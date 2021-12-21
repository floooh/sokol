#-------------------------------------------------------------------------------
#   Read output of gen_json.py and generate Zig language bindings.
#
#   Zig coding style:
#   - types are PascalCase
#   - functions are camelCase
#   - otherwise snake_case
#-------------------------------------------------------------------------------
import gen_ir
import json, re, os, shutil

module_names = {
    'sg_':      'gfx',
    'sapp_':    'app',
    'stm_':     'time',
    'saudio_':  'audio',
    'sgl_':     'gl',
    'sdtx_':    'debugtext',
    'sshape_':  'shape',
}

c_source_paths = {
    'sg_':      'sokol-zig/src/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-zig/src/sokol/c/sokol_app.c',
    'stm_':     'sokol-zig/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-zig/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-zig/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-zig/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-zig/src/sokol/c/sokol_shape.c',
}

name_ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
    'sg_install_trace_hooks',
    'sg_trace_hooks',
]

name_overrides = {
    'sgl_error':    'sgl_get_error',   # 'error' is reserved in Zig
    'sgl_deg':      'sgl_as_degrees',
    'sgl_rad':      'sgl_as_radians'
}

# NOTE: syntax for function results: "func_name.RESULT"
type_overrides = {
    'sg_context_desc.color_format':         'int',
    'sg_context_desc.depth_format':         'int',
    'sg_apply_uniforms.ub_index':           'uint32_t',
    'sg_draw.base_element':                 'uint32_t',
    'sg_draw.num_elements':                 'uint32_t',
    'sg_draw.num_instances':                'uint32_t',
    'sshape_element_range_t.base_element':  'uint32_t',
    'sshape_element_range_t.num_elements':  'uint32_t',
    'sdtx_font.font_index':                 'uint32_t',
}

prim_types = {
    'int':          'i32',
    'bool':         'bool',
    'char':         'u8',
    'int8_t':       'i8',
    'uint8_t':      'u8',
    'int16_t':      'i16',
    'uint16_t':     'u16',
    'int32_t':      'i32',
    'uint32_t':     'u32',
    'int64_t':      'i64',
    'uint64_t':     'u64',
    'float':        'f32',
    'double':       'f64',
    'uintptr_t':    'usize',
    'intptr_t':     'isize',
    'size_t':       'usize'
}

prim_defaults = {
    'int':          '0',
    'bool':         'false',
    'int8_t':       '0',
    'uint8_t':      '0',
    'int16_t':      '0',
    'uint16_t':     '0',
    'int32_t':      '0',
    'uint32_t':     '0',
    'int64_t':      '0',
    'uint64_t':     '0',
    'float':        '0.0',
    'double':       '0.0',
    'uintptr_t':    '0',
    'intptr_t':     '0',
    'size_t':       '0'
}

struct_types = []
enum_types = []
enum_items = {}
out_lines = ''

def reset_globals():
    global struct_types
    global enum_types
    global enum_items
    global out_lines
    struct_types = []
    enum_types = []
    enum_items = {}
    out_lines = ''

re_1d_array = re.compile("^(?:const )?\w*\s\*?\[\d*\]$")
re_2d_array = re.compile("^(?:const )?\w*\s\*?\[\d*\]\[\d*\]$")

def l(s):
    global out_lines
    out_lines += s + '\n'

def as_zig_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_zig_struct_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_zig_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

def check_type_override(func_or_struct_name, field_or_arg_name, orig_type):
    s = f"{func_or_struct_name}.{field_or_arg_name}"
    if s in type_overrides:
        return type_overrides[s]
    else:
        return orig_type

def check_name_override(name):
    if name in name_overrides:
        return name_overrides[name]
    else:
        return name

def check_name_ignore(name):
    return name in name_ignores

# PREFIX_BLA_BLUB to bla_blub
def as_snake_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    return outp

# prefix_bla_blub => blaBlub
def as_camel_case(s):
    parts = s.lower().split('_')[1:]
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    return outp

# PREFIX_ENUM_BLA => Bla, _PREFIX_ENUM_BLA => Bla
def as_enum_item_name(s):
    outp = s
    if outp.startswith('_'):
        outp = outp[1:]
    parts = outp.split('_')[2:]
    outp = '_'.join(parts)
    if outp[0].isdigit():
        outp = '_' + outp
    return outp

def enum_default_item(enum_name):
    return enum_items[enum_name][0]

def is_prim_type(s):
    return s in prim_types

def is_struct_type(s):
    return s in struct_types

def is_enum_type(s):
    return s in enum_types

def is_string_ptr(s):
    return s == "const char *"

def is_const_void_ptr(s):
    return s == "const void *"

def is_void_ptr(s):
    return s == "void *"

def is_const_prim_ptr(s):
    for prim_type in prim_types:
        if s == f"const {prim_type} *":
            return True
    return False

def is_prim_ptr(s):
    for prim_type in prim_types:
        if s == f"{prim_type} *":
            return True
    return False

def is_const_struct_ptr(s):
    for struct_type in struct_types:
        if s == f"const {struct_type} *":
            return True
    return False

def is_func_ptr(s):
    return '(*)' in s

def is_1d_array_type(s):
    return re_1d_array.match(s)

def is_2d_array_type(s):
    return re_2d_array.match(s)

def type_default_value(s):
    return prim_defaults[s]

def extract_array_type(s):
    return s[:s.index('[')].strip()

def extract_array_nums(s):
    return s[s.index('['):].replace('[', ' ').replace(']', ' ').split()

def extract_ptr_type(s):
    tokens = s.split()
    if tokens[0] == 'const':
        return tokens[1]
    else:
        return tokens[0]

def as_extern_c_arg_type(arg_type, prefix):
    if arg_type == "void":
        return "void"
    elif is_prim_type(arg_type):
        return as_zig_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_zig_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_zig_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return "?*anyopaque"
    elif is_const_void_ptr(arg_type):
        return "?*const anyopaque"
    elif is_string_ptr(arg_type):
        return "[*c]const u8"
    elif is_const_struct_ptr(arg_type):
        return f"[*c]const {as_zig_struct_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return f"[*c] {as_zig_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return f"[*c]const {as_zig_prim_type(extract_ptr_type(arg_type))}"
    else:
        return '??? (as_extern_c_arg_type)'

def as_zig_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return "void"
        else:
            return ""
    elif is_prim_type(arg_type):
        return pre + as_zig_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_zig_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_zig_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return pre + "?*anyopaque"
    elif is_const_void_ptr(arg_type):
        return pre + "?*const anyopaque"
    elif is_string_ptr(arg_type):
        return pre + "[:0]const u8"
    elif is_const_struct_ptr(arg_type):
        # not a bug, pass const structs by value
        return pre + f"{as_zig_struct_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"* {as_zig_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return pre + f"*const {as_zig_prim_type(extract_ptr_type(arg_type))}"
    else:
        return arg_prefix + "??? (as_zig_arg_type)"

# get C-style arguments of a function pointer as string
def funcptr_args_c(field_type, prefix):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ""
    for token in tokens:
        arg_type = token.strip()
        if s != "":
            s += ", "
        c_arg = as_extern_c_arg_type(arg_type, prefix)
        if (c_arg == "void"):
            return ""
        else:
            s += c_arg
    return s

# get C-style result of a function pointer as string
def funcptr_res_c(field_type):
    res_type = field_type[:field_type.index('(*)')].strip()
    if res_type == 'void':
        return 'void'
    elif is_const_void_ptr(res_type):
        return '?*const anyopaque'
    else:
        return '???'

def funcdecl_args_c(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        param_name = param_decl['name']
        param_type = check_type_override(func_name, param_name, param_decl['type'])
        s += as_extern_c_arg_type(param_type, prefix)
    return s

def funcdecl_args_zig(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        param_name = param_decl['name']
        param_type = check_type_override(func_name, param_name, param_decl['type'])
        s += f"{as_zig_arg_type(f'{param_name}: ', param_type, prefix)}"
    return s

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_type_override(func_name, 'RESULT', decl_type[:decl_type.index('(')].strip())
    return as_extern_c_arg_type(result_type, prefix)

def funcdecl_result_zig(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_type_override(func_name, 'RESULT', decl_type[:decl_type.index('(')].strip())
    zig_res_type = as_zig_arg_type(None, result_type, prefix)
    if zig_res_type == "":
        zig_res_type = "void"
    return zig_res_type

def gen_struct(decl, prefix, callconvc_funcptrs = True, use_raw_name=False, use_extern=True):
    struct_name = decl['name']
    zig_type = struct_name if use_raw_name else as_zig_struct_type(struct_name, prefix)
    l(f"pub const {zig_type} = {'extern ' if use_extern else ''}struct {{")
    for field in decl['fields']:
        field_name = field['name']
        field_type = field['type']
        field_type = check_type_override(struct_name, field_name, field_type)
        if is_prim_type(field_type):
            l(f"    {field_name}: {as_zig_prim_type(field_type)} = {type_default_value(field_type)},")
        elif is_struct_type(field_type):
            l(f"    {field_name}: {as_zig_struct_type(field_type, prefix)} = .{{ }},")
        elif is_enum_type(field_type):
            l(f"    {field_name}: {as_zig_enum_type(field_type, prefix)} = .{enum_default_item(field_type)},")
        elif is_string_ptr(field_type):
            l(f"    {field_name}: [*c]const u8 = null,")
        elif is_const_void_ptr(field_type):
            l(f"    {field_name}: ?*const anyopaque = null,")
        elif is_void_ptr(field_type):
            l(f"    {field_name}: ?*anyopaque = null,")
        elif is_const_prim_ptr(field_type):
            l(f"    {field_name}: ?[*]const {as_zig_prim_type(extract_ptr_type(field_type))} = null,")
        elif is_func_ptr(field_type):
            if callconvc_funcptrs:
                l(f"    {field_name}: ?fn({funcptr_args_c(field_type, prefix)}) callconv(.C) {funcptr_res_c(field_type)} = null,")
            else:
                l(f"    {field_name}: ?fn({funcptr_args_c(field_type, prefix)}) {funcptr_res_c(field_type)} = null,")
        elif is_1d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    zig_type = as_zig_prim_type(array_type)
                    def_val = type_default_value(array_type)
                elif is_struct_type(array_type):
                    zig_type = as_zig_struct_type(array_type, prefix)
                    def_val = '.{}'
                elif is_enum_type(array_type):
                    zig_type = as_zig_enum_type(array_type, prefix)
                    def_val = '.{}'
                else:
                    zig_type = '??? (array type)'
                    def_val = '???'
                t0 = f"[{array_nums[0]}]{zig_type}"
                t0_slice = f"[]const {zig_type}"
                t1 = f"[_]{zig_type}"
                l(f"    {field_name}: {t0} = {t1}{{{def_val}}} ** {array_nums[0]},")
            elif is_const_void_ptr(array_type):
                l(f"    {field_name}: [{array_nums[0]}]?*const anyopaque = [_]?*const anyopaque {{ null }} ** {array_nums[0]},")
            else:
                l(f"//    FIXME: ??? array {field_name}: {field_type} => {array_type} [{array_nums[0]}]")
        elif is_2d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type):
                zig_type = as_zig_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                zig_type = as_zig_struct_type(array_type, prefix)
                def_val = ".{ }"
            else:
                zig_type = "???"
                def_val = "???"
            t0 = f"[{array_nums[0]}][{array_nums[1]}]{zig_type}"
            l(f"    {field_name}: {t0} = [_][{array_nums[1]}]{zig_type}{{[_]{zig_type}{{ {def_val} }}**{array_nums[1]}}}**{array_nums[0]},")
        else:
            l(f"// FIXME: {field_name}: {field_type};")
    l("};")

def gen_consts(decl, prefix):
    for item in decl['items']:
        l(f"pub const {as_snake_case(item['name'], prefix)} = {item['value']};")

def gen_enum(decl, prefix):
    l(f"pub const {as_zig_enum_type(decl['name'], prefix)} = enum(i32) {{")
    for item in decl['items']:
        item_name = as_enum_item_name(item['name'])
        if item_name != "FORCE_U32":
            if 'value' in item:
                l(f"    {item_name} = {item['value']},")
            else:
                l(f"    {item_name},")
    l("};")

def gen_func_c(decl, prefix):
    l(f"pub extern fn {decl['name']}({funcdecl_args_c(decl, prefix)}) {funcdecl_result_c(decl, prefix)};")

def gen_func_zig(decl, prefix):
    c_func_name = decl['name']
    zig_func_name = as_camel_case(check_name_override(decl['name']))
    zig_res_type = funcdecl_result_zig(decl, prefix)
    l(f"pub fn {zig_func_name}({funcdecl_args_zig(decl, prefix)}) {zig_res_type} {{")
    if zig_res_type != 'void':
        s = f"    return {c_func_name}("
    else:
        s = f"    {c_func_name}("
    for i, param_decl in enumerate(decl['params']):
        if i > 0:
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        if is_const_struct_ptr(arg_type):
            s += f"&{arg_name}"
        elif is_string_ptr(arg_type):
            s += f"@ptrCast([*c]const u8,{arg_name})"
        else:
            s += arg_name
    s += ");"
    l(s)
    l("}")

def pre_parse(inp):
    global struct_types
    global enum_types
    for decl in inp['decls']:
        kind = decl['kind']
        if kind == 'struct':
            struct_types.append(decl['name'])
        elif kind == 'enum':
            enum_name = decl['name']
            enum_types.append(enum_name)
            enum_items[enum_name] = []
            for item in decl['items']:
                enum_items[enum_name].append(as_enum_item_name(item['name']))

def gen_imports(inp, dep_prefixes):
    for dep_prefix in dep_prefixes:
        dep_module_name = module_names[dep_prefix]
        l(f'const {dep_prefix[:-1]} = @import("{dep_module_name}.zig");')
        l('')

def gen_helpers(inp):
    if inp['prefix'] in ['sg_', 'sdtx_', 'sshape_']:
        l('// helper function to convert "anything" to a Range struct')
        l('pub fn asRange(val: anytype) Range {')
        l('    const type_info = @typeInfo(@TypeOf(val));')
        l('    switch (type_info) {')
        l('        .Pointer => {')
        l('            switch (type_info.Pointer.size) {')
        l('                .One => return .{ .ptr = val, .size = @sizeOf(type_info.Pointer.child) },')
        l('                .Slice => return .{ .ptr = val.ptr, .size = @sizeOf(type_info.Pointer.child) * val.len },')
        l('                else => @compileError("FIXME: Pointer type!"),')
        l('            }')
        l('        },')
        l('        .Struct, .Array => {')
        l('            return .{ .ptr = &val, .size = @sizeOf(@TypeOf(val)) };')
        l('        },')
        l('        else => {')
        l('            @compileError("Cannot convert to range!");')
        l('        }')
        l('    }')
        l('}')
        l('')
    if inp['prefix'] == 'sdtx_':
        l('// std.fmt compatible Writer')
        l('pub const Writer = struct {')
        l('    pub const Error = error { };')
        l('    pub fn writeAll(self: Writer, bytes: []const u8) Error!void {')
        l('        _ = self;')
        l('        for (bytes) |byte| {')
        l('            putc(byte);')
        l('        }')
        l('    }')
        l('    pub fn writeByteNTimes(self: Writer, byte: u8, n: u64) Error!void {')
        l('        _ = self;')
        l('        var i: u64 = 0;')
        l('        while (i < n): (i += 1) {')
        l('            putc(byte);')
        l('        }')
        l('    }')
        l('};')
        l('// std.fmt-style formatted print')
        l('pub fn print(comptime fmt: anytype, args: anytype) void {')
        l('    var writer: Writer = .{};')
        l('    @import("std").fmt.format(writer, fmt, args) catch {};')
        l('}')
        l('')

def gen_module(inp, dep_prefixes):
    l('// machine generated, do not edit')
    l('')
    gen_imports(inp, dep_prefixes)
    gen_helpers(inp)
    pre_parse(inp)
    prefix = inp['prefix']
    for decl in inp['decls']:
        if not decl['is_dep']:
            kind = decl['kind']
            if kind == 'consts':
                gen_consts(decl, prefix)
            elif not check_name_ignore(decl['name']):
                if kind == 'struct':
                    gen_struct(decl, prefix)
                elif kind == 'enum':
                    gen_enum(decl, prefix)
                elif kind == 'func':
                    gen_func_c(decl, prefix)
                    gen_func_zig(decl, prefix)

def prepare():
    print('Generating zig bindings:')
    if not os.path.isdir('sokol-zig/src/sokol'):
        os.makedirs('sokol-zig/src/sokol')
    if not os.path.isdir('sokol-zig/src/sokol/c'):
        os.makedirs('sokol-zig/src/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-zig/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-zig/src/sokol/{ir['module']}.zig"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
