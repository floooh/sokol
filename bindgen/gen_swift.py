#-------------------------------------------------------------------------------
#   Generate Swift bindings.
#
#   Swift coding style:
#   - types are PascalCase
#   - functions, properties, and variables are camelCase
#   - constants are SCREAMING_SNAKE_CASE
#-------------------------------------------------------------------------------
# References
# https://github.com/swiftlang/swift/blob/main/docs/HowSwiftImportsCAPIs.md


import gen_ir
import os, shutil, sys

import gen_util as util

module_names = {
    'slog_':    'log',
    'sg_':      'gfx',
    'sapp_':    'app',
    'stm_':     'time',
    'saudio_':  'audio',
    'sgl_':     'gl',
    'sdtx_':    'debugtext',
    'sshape_':  'shape',
    'sglue_':   'glue',
    'sfetch_':  'fetch',
    'simgui_':  'imgui',
}

c_source_paths = {
    'slog_':    'sokol-swift/Sources/sokol/c/sokol_log.c',
    'sg_':      'sokol-swift/Sources/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-swift/Sources/sokol/c/sokol_app.c',
    'stm_':     'sokol-swift/Sources/sokol/c/sokol_time.c',
    'saudio_':  'sokol-swift/Sources/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-swift/Sources/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-swift/Sources/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-swift/Sources/sokol/c/sokol_shape.c',
    'sglue_':   'sokol-swift/Sources/sokol/c/sokol_glue.c',
    'sfetch_':  'sokol-swift/Sources/sokol/c/sokol_fetch.c',
    'simgui_':  'sokol-swift/Sources/sokol/c/sokol_imgui.c',
}

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
    'sg_install_trace_hooks',
    'sg_trace_hooks',
]

# functions that need to be exposed as 'raw' C callbacks without a Swift wrapper function
c_callbacks = [
    'slog_func'
]

# NOTE: syntax for function results: "func_name.RESULT"
overrides = {
    'func':                                  'func_',
    'sgl_error':                            'sgl_get_error',   # 'error' is reserved in Swift
    'sgl_deg':                              'sgl_as_degrees',
    'sgl_rad':                              'sgl_as_radians',
    'sg_apply_uniforms.ub_index':           'uint32_t',
    'sg_draw.base_element':                 'uint32_t',
    'sg_draw.num_elements':                 'uint32_t',
    'sg_draw.num_instances':                'uint32_t',
    'sshape_element_range_t.base_element':  'uint32_t',
    'sshape_element_range_t.num_elements':  'uint32_t',
    'sdtx_font.font_index':                 'uint32_t',
    'SGL_NO_ERROR':                         'SGL_ERROR_NO_ERROR',
    'sfetch_continue':                      'continue_fetching',  # 'continue' is reserved in Swift
    'sfetch_desc':                          'sfetch_get_desc'     # 'desc' shadowed by earlier definition
}

prim_types = {
    'int':          'CInt',
    'bool':         'Bool',
    'char':         'UInt8',
    'int8_t':       'Int8',
    'uint8_t':      'UInt8',
    'int16_t':      'Int16',
    'uint16_t':     'UInt16',
    'int32_t':      'Int32',
    'uint32_t':     'UInt32',
    'int64_t':      'Int64',
    'uint64_t':     'UInt64',
    'float':        'Float',
    'double':       'Double',
    'uintptr_t':    'UInt',
    'intptr_t':     'Int',
    'size_t':       'UInt'
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

def l(s):
    global out_lines
    out_lines += s + '\n'

def as_swift_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_swift_struct_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        # ignore '_t' type postfix
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_swift_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

def check_override(name, default=None):
    if name in overrides:
        return overrides[name]
    elif default is None:
        return name
    else:
        return default

def check_ignore(name):
    return name in ignores

# PREFIX_ENUM_BLA => Bla, _PREFIX_ENUM_BLA => Bla
def as_enum_item_name(s):
    outp = s.lstrip('_')
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

def type_default_value(s):
    return prim_defaults[s]

def as_c_arg_type(arg_type, prefix):
    if arg_type == "void":
        return "void"
    elif is_prim_type(arg_type):
        return as_swift_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_swift_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_swift_enum_type(arg_type, prefix)
    elif util.is_void_ptr(arg_type):
        return "UnsafeMutableRawPointer?"
    elif util.is_const_void_ptr(arg_type):
        return "UnsafeRawPointer?"
    elif util.is_string_ptr(arg_type):
        return "UnsafePointer<CChar>?"
    elif is_const_struct_ptr(arg_type):
        return f"{as_swift_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return f"{as_swift_prim_type(util.extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return f"let {as_swift_prim_type(util.extract_ptr_type(arg_type))}"
    else:
        sys.exit(f"Error as_c_arg_type(): {arg_type}")

def as_swift_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return ""
        else:
            return ""
    elif is_prim_type(arg_type):
        return pre + as_swift_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_swift_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_swift_enum_type(arg_type, prefix)
    elif util.is_void_ptr(arg_type):
        return pre + "UnsafeMutableRawPointer?"
    elif util.is_const_void_ptr(arg_type):
        return pre + "UnsafeRawPointer?"
    elif util.is_string_ptr(arg_type):
        return pre + "UnsafePointer<CChar>?"
    elif is_const_struct_ptr(arg_type):
        return pre + f"{as_swift_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"UnsafeMutablePointer<{as_swift_prim_type(util.extract_ptr_type(arg_type))}>?"
    elif is_const_prim_ptr(arg_type):
        return pre + f"UnsafePointer<{as_swift_prim_type(util.extract_ptr_type(arg_type))}>?"
    else:
        sys.exit(f"ERROR as_swift_arg_type(): {arg_type}")

def is_swift_string(swift_type):
    return swift_type == "UnsafePointer<CChar>?"

# get C-style arguments of a function pointer as string
def funcptr_args_c(field_type, prefix):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ""
    for token in tokens:
        arg_type = token.strip()
        if s != "":
            s += ", "
        c_arg = as_c_arg_type(arg_type, prefix)
        if c_arg == "void":
            return ""
        else:
            s += c_arg
    return s

# get C-style result of a function pointer as string
def funcptr_result_c(field_type):
    res_type = field_type[:field_type.index('(*)')].strip()
    if res_type == 'void':
        return 'Void'
    elif is_prim_type(res_type):
        return as_swift_prim_type(res_type)
    elif util.is_const_void_ptr(res_type):
        return 'UnsafeRawPointer?'
    elif util.is_void_ptr(res_type):
        return 'UnsafeMutableRawPointer?'
    else:
        sys.exit(f"ERROR funcptr_result_c(): {field_type}")

def funcdecl_args_c(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += as_c_arg_type(param_type, prefix)
    return s

def funcdecl_args_swift(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += f"{as_swift_arg_type(f'_ {param_name}: ', param_type, prefix)}"
    return s

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_override(f'{func_name}.RESULT', default=decl_type[:decl_type.index('(')].strip())
    return as_c_arg_type(result_type, prefix)

def funcdecl_result_swift(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_override(f'{func_name}.RESULT', default=decl_type[:decl_type.index('(')].strip())
    swift_res_type = as_swift_arg_type(None, result_type, prefix)
    return swift_res_type

def gen_struct(decl, prefix):
    struct_name = check_override(decl['name'])
    swift_type = as_swift_struct_type(struct_name, prefix)
    l(f"struct {swift_type} {{")
    for field in decl['fields']:
        field_name = check_override(field['name'])
        field_type = check_override(f'{struct_name}.{field_name}', default=field['type'])
        if is_prim_type(field_type):
            l(f"    var {field_name}: {as_swift_prim_type(field_type)} = {type_default_value(field_type)}")
        elif is_struct_type(field_type):
            l(f"    var {field_name}: {as_swift_struct_type(field_type, prefix)} = {as_swift_struct_type(field_type, prefix)}()")
        elif is_enum_type(field_type):
            l(f"    var {field_name}: {as_swift_enum_type(field_type, prefix)} = .{enum_default_item(field_type)}")
        elif util.is_string_ptr(field_type):
            l(f"    var {field_name}: UnsafePointer<CChar>? = nil")
        elif util.is_const_void_ptr(field_type):
            l(f"    var {field_name}: UnsafeRawPointer? = nil")
        elif util.is_void_ptr(field_type):
            l(f"    var {field_name}: UnsafeMutableRawPointer? = nil")
        elif is_const_prim_ptr(field_type):
            l(f"    let {field_name}: {as_swift_prim_type(util.extract_ptr_type(field_type))} = nil")
        elif util.is_func_ptr(field_type):
            l(f"    var {field_name}: (@convention(c) ({funcptr_args_c(field_type, prefix)}) -> {funcptr_result_c(field_type)})? = nil")
        elif util.is_1d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    swift_type = as_swift_prim_type(array_type)
                    def_val = type_default_value(array_type)
                elif is_struct_type(array_type):
                    swift_type = as_swift_struct_type(array_type, prefix)
                    def_val = ''
                elif is_enum_type(array_type):
                    swift_type = as_swift_enum_type(array_type, prefix)
                    def_val = ''
                else:
                    sys.exit(f"ERROR gen_struct is_1d_array_type: {array_type}")
                t0 = f"[{swift_type}]"
                t1 = f"{swift_type}"
                l(f"    var {field_name}: {t0} = Array(repeating: {t1}({def_val}), count: {array_sizes[0]})")
            elif util.is_const_void_ptr(array_type):
                l(f"    {field_name}: [UnsafeRawPointer?] = Array(repeating: nil, count: {array_sizes[0]})")
            else:
                sys.exit(f"ERROR gen_struct: array {field_name}: {field_type} => {array_type} [{array_sizes[0]}]")
        elif util.is_2d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if is_prim_type(array_type):
                swift_type = as_swift_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                swift_type = as_swift_struct_type(array_type, prefix)
                def_val = ""
            else:
                sys.exit(f"ERROR gen_struct is_2d_array_type: {array_type}")
            t0 = f"[{array_sizes[0]}][{array_sizes[1]}]{swift_type}"
            l(f"    {field_name}: {t0} = [_][{array_sizes[1]}]{swift_type}{{[_]{swift_type}{{{def_val}}} ** {array_sizes[1]}}} ** {array_sizes[0]}")
        else:
            sys.exit(f"ERROR gen_struct: {field_name}: {field_type}")
    l("}")

def gen_consts(decl, prefix):
    for item in decl['items']:
        item_name = check_override(item['name'])
        l(f"let {util.as_lower_snake_case(item_name, prefix)} = {item['value']}")

def gen_enum(decl, prefix):
    enum_name = check_override(decl['name'])
    l(f"enum {as_swift_enum_type(enum_name, prefix)} : Int32 {{")
    for item in decl['items']:
        item_name = as_enum_item_name(check_override(item['name']))
        if item_name != "Force_u32":
            if 'value' in item:
                l(f"    case {item_name} = {item['value']}")
            else:
                l(f"    case {item_name}")
    l("}")

def gen_func_c(decl, prefix):
    l(f"@_extern(c, \"{decl['name']}\")")
    if(funcdecl_result_c(decl, prefix) != "void"):
        l(f"func {decl['name']}({funcdecl_args_c(decl, prefix)}) -> {funcdecl_result_c(decl, prefix)}")
    else:
        l(f"func {decl['name']}({funcdecl_args_c(decl, prefix)})")

def gen_func_swift(decl, prefix):
    c_func_name = decl['name']
    swift_func_name = util.as_lower_camel_case(check_override(decl['name']), prefix)
    if c_func_name in c_callbacks:
        # a simple forwarded C callback function
        l(f"typealias {swift_func_name} = {c_func_name}")
    else:
        swift_res_type = funcdecl_result_swift(decl, prefix)
        l(f"func {swift_func_name}({funcdecl_args_swift(decl, prefix)}) {swift_res_type} {{")
        if is_swift_string(swift_res_type):
            # special case: convert C string to Swift string slice
            s = f"    return cStrToSwift({c_func_name}("
        elif swift_res_type != 'void':
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
            elif util.is_string_ptr(arg_type):
                s += f"{arg_name}"
            else:
                s += arg_name
        if is_swift_string(swift_res_type):
            s += ")"
        s += ")"
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
    # l('const builtin = @import("builtin");')
    for dep_prefix in dep_prefixes:
        dep_module_name = module_names[dep_prefix]
        # l(f'const {dep_prefix[:-1]} = @import("{dep_module_name}.swift");')
    l('')

def gen_helpers(inp):
    l('// helper function to convert a C string to a Swift string slice')
    # l('fn cStrToSwift(c_str: [*c]const u8) [:0]const u8 {')
    # l('    return @import("std").mem.span(c_str);')
    # l('}')
    # if inp['prefix'] in ['sg_', 'sdtx_', 'sshape_', 'sfetch_']:
    #     l('// helper function to convert "anything" to a Range struct')
    #     l('pub fn asRange(val: anytype) Range {')
    #     l('    const type_info = @typeInfo(@TypeOf(val));')
    #     l('    // FIXME: naming convention change between 0.13 and 0.14-dev')
    #     l('    if (@hasField(@TypeOf(type_info), "Pointer")) {')
    #     l('        switch (type_info) {')
    #     l('            .Pointer => {')
    #     l('                switch (type_info.Pointer.size) {')
    #     l('                    .One => return .{ .ptr = val, .size = @sizeOf(type_info.Pointer.child) },')
    #     l('                    .Slice => return .{ .ptr = val.ptr, .size = @sizeOf(type_info.Pointer.child) * val.len },')
    #     l('                    else => @compileError("FIXME: Pointer type!"),')
    #     l('                }')
    #     l('            },')
    #     l('            .Struct, .Array => {')
    #     l('                @compileError("Structs and arrays must be passed as pointers to asRange");')
    #     l('            },')
    #     l('            else => {')
    #     l('                @compileError("Cannot convert to range!");')
    #     l('            },')
    #     l('        }')
    #     l('    } else {')
    #     l('        switch (type_info) {')
    #     l('            .pointer => {')
    #     l('                switch (type_info.pointer.size) {')
    #     l('                    .One => return .{ .ptr = val, .size = @sizeOf(type_info.pointer.child) },')
    #     l('                    .Slice => return .{ .ptr = val.ptr, .size = @sizeOf(type_info.pointer.child) * val.len },')
    #     l('                    else => @compileError("FIXME: Pointer type!"),')
    #     l('                }')
    #     l('            },')
    #     l('            .@"struct", .array => {')
    #     l('                @compileError("Structs and arrays must be passed as pointers to asRange");')
    #     l('            },')
    #     l('            else => {')
    #     l('                @compileError("Cannot convert to range!");')
    #     l('            },')
    #     l('        }')
    #     l('    }')
    #     l('}')
    #     l('')
    # if inp['prefix'] == 'sdtx_':
    #     l('// std.fmt compatible Writer')
    #     l('pub const Writer = struct {')
    #     l('    pub const Error = error{};')
    #     l('    pub fn writeAll(self: Writer, bytes: []const u8) Error!void {')
    #     l('        _ = self;')
    #     l('        for (bytes) |byte| {')
    #     l('            putc(byte);')
    #     l('        }')
    #     l('    }')
    #     l('    pub fn writeByteNTimes(self: Writer, byte: u8, n: usize) Error!void {')
    #     l('        _ = self;')
    #     l('        var i: u64 = 0;')
    #     l('        while (i < n) : (i += 1) {')
    #     l('            putc(byte);')
    #     l('        }')
    #     l('    }')
    #     l('    pub fn writeBytesNTimes(self: Writer, bytes: []const u8, n: usize) Error!void {')
    #     l('        var i: usize = 0;')
    #     l('        while (i < n) : (i += 1) {')
    #     l('            try self.writeAll(bytes);')
    #     l('        }')
    #     l('    }')
    #     l('};')
    #     l('// std.fmt-style formatted print')
    #     l('pub fn print(comptime fmt: anytype, args: anytype) void {')
    #     l('    const writer: Writer = .{};')
    #     l('    @import("std").fmt.format(writer, fmt, args) catch {};')
    #     l('}')
    #     l('')

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
            elif not check_ignore(decl['name']):
                if kind == 'struct':
                    gen_struct(decl, prefix)
                elif kind == 'enum':
                    gen_enum(decl, prefix)
                elif kind == 'func':
                    gen_func_c(decl, prefix)
                    gen_func_swift(decl, prefix)

def prepare():
    print('=== Generating Swift bindings:')
    if not os.path.isdir('sokol-swift/Sources/sokol'):
        os.makedirs('sokol-swift/Sources/sokol')
    if not os.path.isdir('sokol-swift/Sources/sokol/c'):
        os.makedirs('sokol-swift/Sources/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f' >> warning: skipping generation for {c_prefix} prefix...')
        return
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-swift/Sources/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-swift/Sources/sokol/{ir['module']}.swift"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
