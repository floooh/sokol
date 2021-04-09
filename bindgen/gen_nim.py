#-------------------------------------------------------------------------------
#   Read output of gen_json.py and generate Zig language bindings.
#
#   Nim coding style:
#   - types and constants are PascalCase
#   - functions, parameters, and fields are camelCase
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
    'sg_':      'sokol-nim/src/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-nim/src/sokol/c/sokol_app.c',
    'stm_':     'sokol-nim/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-nim/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-nim/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-nim/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-nim/src/sokol/c/sokol_shape.c',
}

func_name_ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
]

func_name_overrides = {
    'sgl_error': 'sgl_get_error',   # 'error' is reserved in Zig
    'sgl_deg': 'sgl_as_degrees',
    'sgl_rad': 'sgl_as_radians',
}

struct_field_type_overrides = {
    'sg_context_desc.color_format': 'int',
    'sg_context_desc.depth_format': 'int',
}

prim_types = {
    'int':          'int32',
    'bool':         'bool',
    'char':         'char',
    'int8_t':       'int8',
    'uint8_t':      'uint8',
    'int16_t':      'int16',
    'uint16_t':     'uint16',
    'int32_t':      'int32',
    'uint32_t':     'uint32',
    'int64_t':      'int64',
    'uint64_t':     'uint64',
    'float':        'float32',
    'double':       'float64',
    'uintptr_t':    'uint',
    'intptr_t':     'int',
    'size_t':       'int',
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

def as_nim_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_nim_struct_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_nim_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_nim_const_type(s, prefix):
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

def check_func_name_ignore(func_name):
    return func_name in func_name_ignores

def check_func_name_override(func_name):
    if func_name in func_name_overrides:
        return func_name_overrides[func_name]
    else:
        return func_name

def trim_prefix(s, prefix):
    outp = s;
    if outp.lower().startswith(prefix.lower()):
        outp = outp[len(prefix):]
    return outp

# PREFIX_BLA_BLUB to bla_blub
def as_snake_case(s, prefix = ""):
    return trim_prefix(s, prefix).lower()

# prefix_bla_blub => blaBlub
def as_camel_case(s, prefix = ""):
    parts = trim_prefix(s, prefix).lower().split('_')
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    return outp

# prefix_bla_blub => BlaBlub
def as_pascal_case(s, prefix):
    parts = trim_prefix(s, prefix).lower().split('_')
    outp = ""
    for part in parts:
        outp += part.capitalize()
    return outp

# PREFIX_ENUM_BLA => Bla, _PREFIX_ENUM_BLA => Bla
def as_enum_item_name(s):
    outp = s
    if outp.startswith('_'):
        outp = outp[1:]
    parts = outp.lower().split('_')[2:]
    outp = ""
    for part in parts:
        outp += part.capitalize()
    if outp[0].isdigit():
        outp = 'N' + outp
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
        return as_nim_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_nim_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_nim_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return "pointer"
    elif is_const_void_ptr(arg_type):
        return "pointer"
    elif is_string_ptr(arg_type):
        return "cstring"
    elif is_const_struct_ptr(arg_type):
        return f"ptr {as_nim_struct_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return f"[*c] {as_nim_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return f"ptr {as_nim_prim_type(extract_ptr_type(arg_type))}"
    else:
        return '??? (as_extern_c_arg_type)'

def as_nim_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return "void"
        else:
            return ""
    elif is_prim_type(arg_type):
        return pre + as_nim_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_nim_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_nim_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return pre + "pointer"
    elif is_const_void_ptr(arg_type):
        return pre + "pointer"
    elif is_string_ptr(arg_type):
        return pre + "cstring"
    elif is_const_struct_ptr(arg_type):
        return pre + f"ptr {as_nim_struct_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"ptr {as_nim_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return pre + f"ptr {as_nim_prim_type(extract_ptr_type(arg_type))}"
    else:
        return arg_prefix + "??? (as_nim_arg_type)"

# get C-style arguments of a function pointer as string
def funcptr_args_c(field_type, prefix):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ""
    n = 0
    for token in tokens:
        n += 1
        arg_type = token.strip()
        if s != "":
            s += ", "
        c_arg = f"a{n}:" + as_extern_c_arg_type(arg_type, prefix)
        if (c_arg == "void"):
            return ""
        else:
            s += c_arg
    if s == "a1:void":
        s = ""
    return s

# get C-style result of a function pointer as string
def funcptr_res_c(field_type):
    res_type = field_type[:field_type.index('(*)')].strip()
    if res_type == 'void':
        return ''
    elif is_const_void_ptr(res_type):
        return ':pointer'
    else:
        return '???'

def funcdecl_args_c(decl, prefix):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_type = param_decl['type']
        s += as_extern_c_arg_type(arg_type, prefix)
    return s

def funcdecl_args_nim(decl, prefix):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        s += f"{as_nim_arg_type(f'{arg_name}:', arg_type, prefix)}"
    return s

def funcdecl_res_c(decl, prefix):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    return as_extern_c_arg_type(res_type, prefix)

def funcdecl_res_nim(decl, prefix):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    nim_res_type = as_nim_arg_type(None, res_type, prefix)
    if nim_res_type == "":
        nim_res_type = "void"
    return nim_res_type

def gen_struct(decl, prefix, use_raw_name=False):
    struct_name = decl['name']
    nim_type = struct_name if use_raw_name else as_nim_struct_type(struct_name, prefix)
    l(f"type {nim_type}* = object")
    isPublic = True
    for field in decl['fields']:
        field_name = field['name']
        if field_name == "__pad":
            # FIXME: these should be guarded by SOKOL_ZIG_BINDINGS, but aren't?
            continue
        isPublic = not field_name.startswith("_")
        field_name = as_camel_case(field_name, "_")
        if field_name == "ptr":
            field_name = "source"
        if field_name == "ref":
            field_name = "`ref`"
        if field_name == "type":
            field_name = "`type`"
        if isPublic:
            field_name += "*"
        field_type = field['type']
        field_type = check_struct_field_type_override(struct_name, field_name, field_type)
        if is_prim_type(field_type):
            l(f"  {field_name}:{as_nim_prim_type(field_type)}")
        elif is_struct_type(field_type):
            l(f"  {field_name}:{as_nim_struct_type(field_type, prefix)}")
        elif is_enum_type(field_type):
            l(f"  {field_name}:{as_nim_enum_type(field_type, prefix)}")
        elif is_string_ptr(field_type):
            l(f"  {field_name}:cstring")
        elif is_const_void_ptr(field_type):
            l(f"  {field_name}:pointer")
        elif is_void_ptr(field_type):
            l(f"  {field_name}:pointer")
        elif is_const_prim_ptr(field_type):
            l(f"  {field_name}:ptr {as_nim_prim_type(extract_ptr_type(field_type))}")
        elif is_func_ptr(field_type):
            l(f"  {field_name}:proc({funcptr_args_c(field_type, prefix)}){funcptr_res_c(field_type)} {{.cdecl.}}")
        elif is_1d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    nim_type = as_nim_prim_type(array_type)
                elif is_struct_type(array_type):
                    nim_type = as_nim_struct_type(array_type, prefix)
                elif is_enum_type(array_type):
                    nim_type = as_nim_enum_type(array_type, prefix)
                else:
                    nim_type = '??? (array type)'
                t0 = f"array[{array_nums[0]}, {nim_type}]"
                t0_slice = f"[]const {nim_type}"
                t1 = f"[_]{nim_type}"
                l(f"  {field_name}:{t0}")
            elif is_const_void_ptr(array_type):
                l(f"  {field_name}:array[{array_nums[0]}, pointer]")
            else:
                l(f"//    FIXME: ??? array {field_name}:{field_type} => {array_type} [{array_nums[0]}]")
        elif is_2d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type):
                nim_type = as_nim_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                nim_type = as_nim_struct_type(array_type, prefix)
                def_val = ".{ }"
            else:
                nim_type = "???"
                def_val = "???"
            t0 = f"array[{array_nums[0]}, array[{array_nums[1]}, {nim_type}]]"
            l(f"  {field_name}:{t0}")
        else:
            l(f"// FIXME: {field_name}:{field_type};")
    l("")

def gen_consts(decl, prefix):
    l("const")
    for item in decl['items']:
        l(f"  {trim_prefix(item['name'], prefix)}* = {item['value']}")
    l("")

def gen_enum(decl, prefix):
    item_names_by_value = {}
    value = -1
    hasForceU32 = False
    hasExplicitValues = False
    for item in decl['items']:
        itemName = item['name']
        if itemName.endswith("_FORCE_U32"):
            hasForceU32 = True
        elif itemName.endswith("_NUM"):
            continue
        else:
            if 'value' in item:
                hasExplicitValues = True
                value = int(item['value'])
            else:
                value += 1
            item_names_by_value[value] = as_enum_item_name(item['name']);
    if hasForceU32:
        l(f"type {as_nim_enum_type(decl['name'], prefix)}* {{.pure, size:4.}} = enum")
    else:
        l(f"type {as_nim_enum_type(decl['name'], prefix)}* {{.pure.}} = enum")
    if hasExplicitValues:
        # Nim requires explicit enum values to be declared in ascending order
        for value in sorted(item_names_by_value):
            name = item_names_by_value[value]
            l(f"  {name} = {value},")
    else:
        for name in item_names_by_value.values():
            l(f"  {name},")
    l("")

def gen_func_nim(decl, prefix):
    c_func_name = decl['name']
    nim_func_name = as_camel_case(decl['name'], prefix)
    nim_res_type = funcdecl_res_nim(decl, prefix)
    l(f"proc {nim_func_name}*({funcdecl_args_nim(decl, prefix)}):{funcdecl_res_nim(decl, prefix)} {{.cdecl, importc:\"{decl['name']}\".}}")
    l("")

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
        l(f'import {dep_module_name}')
        l('')

def gen_module(inp, dep_prefixes):
    l('## machine generated, do not edit')
    l('')
    gen_imports(inp, dep_prefixes)
    pre_parse(inp)
    prefix = inp['prefix']
    for decl in inp['decls']:
        if not decl['is_dep']:
            kind = decl['kind']
            if kind == 'consts':
                gen_consts(decl, prefix)
            elif kind == 'enum':
                gen_enum(decl, prefix)
            elif kind == 'struct':
                gen_struct(decl, prefix)
            elif kind == 'func':
                if not check_func_name_ignore(decl['name']):
                    gen_func_nim(decl, prefix)

def prepare():
    print('Generating nim bindings:')
    if not os.path.isdir('sokol-nim/src/sokol'):
        os.makedirs('sokol-nim/src/sokol')
    if not os.path.isdir('sokol-nim/src/sokol/c'):
        os.makedirs('sokol-nim/src/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    global out_lines
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-nim/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-nim/src/sokol/{ir['module']}.nim"

    ## some changes for readability
    out_lines = out_lines.replace("PixelformatInfo", "PixelFormatInfo")
    out_lines = out_lines.replace(" Dontcare,", " DontCare,")
    out_lines = out_lines.replace(" Vertexbuffer,", " VertexBuffer,")
    out_lines = out_lines.replace(" Indexbuffer,", " IndexBuffer,")
    out_lines = out_lines.replace(" N2d,", " Plane,")
    out_lines = out_lines.replace(" N3d,", " Volume,")
    out_lines = out_lines.replace(" Vs,", " Vertex,")
    out_lines = out_lines.replace(" Fs,", " Fragment,")

    ## include extensions in generated code
    l("# Nim-specific API extensions")
    l(f"include nim/{ir['module']}")

    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
