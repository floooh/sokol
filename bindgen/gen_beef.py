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
    'sg_':      'sokol-beef/src/sokol/c/sokol_app_gfx.c',
    'sapp_':    'sokol-beef/src/sokol/c/sokol_app_gfx.c',
    'stm_':     'sokol-beef/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-beef/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-beef/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-beef/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-beef/src/sokol/c/sokol_shape.c',
}

func_name_ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
]

func_name_overrides = {
    'sgl_error': 'sgl_get_error',   # 'error' is reserved in Zig
    'sgl_deg': 'sgl_as_degrees',
    'sgl_rad': 'sgl_as_radians'
}

struct_field_type_overrides = {
    'sg_context_desc.color_format': 'int',
    'sg_context_desc.depth_format': 'int',
}

struct_field_name_overrides = {
    'ref': 'Ref',
    'params': '_params'
}

prim_types = {
    'int':          'int32',
    'bool':         'bool',
    'char':         'char8',
    'int8_t':       'int8',
    'uint8_t':      'uint8',
    'int16_t':      'int16',
    'uint16_t':     'uint16',
    'int32_t':      'int32',
    'uint32_t':     'uint32',
    'int64_t':      'int64',
    'uint64_t':     'uint64',
    'float':        'float',
    'double':       'double',
    'uintptr_t':    'uint',
    'intptr_t':     'int',
    'size_t':       'uint'
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
    'float':        '0.0f',
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

def as_beef_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_beef_struct_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_beef_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

def check_struct_field_type_override(struct_name, field_name, orig_type):
    s = f"{struct_name}.{field_name}"
    if s in struct_field_type_overrides:
        return struct_field_type_overrides[s]
    else:
        return orig_type

def check_struct_field_name_override(field_name):
    if field_name in struct_field_name_overrides:
        return struct_field_name_overrides[field_name]
    else:
        return field_name

def check_func_name_ignore(func_name):
    return func_name in func_name_ignores

def check_func_name_override(func_name):
    if func_name in func_name_overrides:
        return func_name_overrides[func_name]
    else:
        return func_name

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
        return as_beef_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_beef_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_beef_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return "void*"
    elif is_const_void_ptr(arg_type):
        return "void*"
    elif is_string_ptr(arg_type):
        return "char8*"
    elif is_const_struct_ptr(arg_type):
        return f"{as_beef_struct_type(extract_ptr_type(arg_type), prefix)}*"
    elif is_prim_ptr(arg_type):
        return f"{as_beef_prim_type(extract_ptr_type(arg_type))}*"
    elif is_const_prim_ptr(arg_type):
        return f"{as_beef_prim_type(extract_ptr_type(arg_type))}*"
    else:
        return '??? (as_extern_c_arg_type)'

def as_beef_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return "void"
        else:
            return ""
    elif is_prim_type(arg_type):
        return pre + as_beef_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_beef_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_beef_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return pre + "void*"
    elif is_const_void_ptr(arg_type):
        return pre + "const void*"
    elif is_string_ptr(arg_type):
        return pre + "const char8*"
    elif is_const_struct_ptr(arg_type):
        # not a bug, pass const structs by value
        return pre + f"{as_beef_struct_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"* {as_beef_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return pre + f"*const {as_beef_prim_type(extract_ptr_type(arg_type))}"
    else:
        return arg_prefix + "??? (as_beef_arg_type)"

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
        return 'void*'
    else:
        return '???'

def funcdecl_args_c(decl, prefix):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_type = param_decl['type']
        s += as_extern_c_arg_type(arg_type, prefix) + " " + check_struct_field_name_override(param_decl['name'])
    return s

def funcdecl_args_beef(decl, prefix):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        s += f"{as_beef_arg_type(f'{arg_name}: ', arg_type, prefix)}"
    return s

def funcdecl_res_c(decl, prefix):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    return as_extern_c_arg_type(res_type, prefix)

def funcdecl_res_beef(decl, prefix):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    beef_res_type = as_beef_arg_type(None, res_type, prefix)
    if beef_res_type == "":
        beef_res_type = "void"
    return beef_res_type

def gen_struct(decl, prefix, callconvc_funcptrs = True, use_raw_name=False, use_extern=True):
    struct_name = decl['name']
    beef_type = struct_name if use_raw_name else as_beef_struct_type(struct_name, prefix)
    l('')
    l('\t\t[CRepr]')
    l(f"\t\tpublic struct {beef_type}")
    l('\t\t{')
    for field in decl['fields']:
        field_name = field['name']
        field_name = check_struct_field_name_override(field_name)
        field_type = field['type']
        field_type = check_struct_field_type_override(struct_name, field_name, field_type)
        if is_prim_type(field_type):
            l(f"\t\t\tpublic {as_beef_prim_type(field_type)} {field_name}  = {type_default_value(field_type)};")
        elif is_struct_type(field_type):
            l(f"\t\t\tpublic {as_beef_struct_type(field_type, prefix)} {field_name} = .();")
        elif is_enum_type(field_type):
            l(f"\t\t\tpublic {as_beef_enum_type(field_type, prefix)} {field_name} = .{enum_default_item(field_type)};")
        elif is_string_ptr(field_type):
            l(f"\t\t\tpublic char8* {field_name} = null;")
        elif is_const_void_ptr(field_type):
            l(f"\t\t\tpublic void* {field_name} = null;")
        elif is_void_ptr(field_type):
            l(f"\t\t\tpublic void* {field_name} = null;")
        elif is_const_prim_ptr(field_type):
            l(f"\t\t\tpublic {as_beef_prim_type(extract_ptr_type(field_type))}* {field_name} = null;")
        elif is_func_ptr(field_type):
            if callconvc_funcptrs:
                l(f"\t\t\tpublic function {funcptr_res_c(field_type)}({funcptr_args_c(field_type, prefix)}) {field_name} = null;")
            else:
                l(f"\t\t\tpublic {field_name}: ?fn({funcptr_args_c(field_type, prefix)}) {funcptr_res_c(field_type)} = null,")
        elif is_1d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    beef_type = as_beef_prim_type(array_type)
                    def_val = 'default'
                elif is_struct_type(array_type):
                    beef_type = as_beef_struct_type(array_type, prefix)
                    def_val = '.()'
                elif is_enum_type(array_type):
                    beef_type = as_beef_enum_type(array_type, prefix)
                    def_val = '.()'
                else:
                    beef_type = '??? (array type)'
                    def_val = '???'
                t0 = f"{beef_type}[{array_nums[0]}]"
                t0_slice = f"[]const {beef_type}"
                t1 = f"[_]{beef_type}"
                l(f"\t\t\tpublic {t0} {field_name} = {def_val};")
            elif is_const_void_ptr(array_type):
                l(f"\t\t\tpublic void* [{array_nums[0]}] {field_name} = default;")
            else:
                l(f"//    FIXME: ??? array {field_name}: {field_type} => {array_type} [{array_nums[0]}]")
        elif is_2d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type):
                beef_type = as_beef_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                beef_type = as_beef_struct_type(array_type, prefix)
                def_val = ".{ }"
            else:
                beef_type = "???"
                def_val = "???"
            t0 = f"{beef_type}[{array_nums[0]}][{array_nums[1]}]"
            l(f"\t\t\tpublic {t0} {field_name} = default;")
        else:
            l(f"// FIXME: {field_name}: {field_type};")
    l("\t\t}")

def gen_consts(decl, prefix):
    for item in decl['items']:
        l(f"\t\tpublic const int {as_snake_case(item['name'], prefix)} = {item['value']};")

def gen_enum(decl, prefix):
    l('')
    l(f"\t\tpublic enum {as_beef_enum_type(decl['name'], prefix)} : int32")
    l('\t\t{')
    for item in decl['items']:
        item_name = as_enum_item_name(item['name'])
        if item_name != "FORCE_U32":
            if 'value' in item:
                l(f"\t\t\t{item_name} = {item['value']},")
            else:
                l(f"\t\t\t{item_name},")
    l("\t\t}")

def gen_func_c(decl, prefix):
    beef_func_name = as_camel_case(check_func_name_override(decl['name']))
    l('')
    l(f'\t\t[LinkName("{decl["name"]}")]')
    l(f"\t\tpublic static extern {funcdecl_res_c(decl, prefix)} {beef_func_name}({funcdecl_args_c(decl, prefix)});")

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

def gen_namespace(inp):
    l('using System;')
    l('')
    l('namespace sokol')
    l('{')
    l('\tpublic static class {}'.format(inp['module'].title()))
    l('\t{')

def gen_end_namespace():
    l('\t}')
    l('}')

def gen_helpers(inp):
    if inp['prefix'] in ['sg_', 'sdtx_', 'sshape_']:
        l('\t\t// helper function to convert "anything" to a Range struct')
        l('\t\tpublic static Range asRange<T>(T anytype)')
        l('\t\t{')
        l('\t\t\tvar r = Range();')
        l('\t\t\tr.ptr = Internal.UnsafeCastToPtr(anytype);')
        l('\t\t\tr.size = (uint)anytype.GetType().InstanceSize;')
        l('\t\t\treturn r;')
        l('\t\t}')
        l('')
    if inp['prefix'] == 'sdtx_':
        l('// std.fmt compatible Writer')
        l('pub const Writer = struct {')
        l('    pub const Error = error { };')
        l('    pub fn writeAll(self: Writer, bytes: []const u8) Error!void {')
        l('        for (bytes) |byte| {')
        l('            putc(byte);')
        l('        }')
        l('    }')
        l('    pub fn writeByteNTimes(self: Writer, byte: u8, n: u64) Error!void {')
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
    gen_namespace(inp)
    gen_helpers(inp)
    pre_parse(inp)
    prefix = inp['prefix']
    for decl in inp['decls']:
        if not decl['is_dep']:
            kind = decl['kind']
            if kind == 'struct':
                gen_struct(decl, prefix)
            elif kind == 'consts':
                gen_consts(decl, prefix)
            elif kind == 'enum':
                gen_enum(decl, prefix)
            elif kind == 'func':
                if not check_func_name_ignore(decl['name']):
                    gen_func_c(decl, prefix)
    gen_end_namespace()

def prepare():
    print('Generating beef bindings:')
    if not os.path.isdir('sokol-beef/src/sokol'):
        os.makedirs('sokol-beef/src/sokol')
    if not os.path.isdir('sokol-beef/src/sokol/c'):
        os.makedirs('sokol-beef/src/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-beef/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-beef/src/sokol/{ir['module']}.bf"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
