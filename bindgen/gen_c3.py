#-------------------------------------------------------------------------------
#   gen_c3.py
#
#   Generate C3 bindings.
#-------------------------------------------------------------------------------
import gen_ir
import gen_util as util
import os, shutil, sys

bindings_root = 'sokol-c3'
c_root = f'{bindings_root}/sokol.c3l/c'
module_root = f'{bindings_root}/sokol.c3l'

# TODO: Consider changing module names to something shorter.
#       For example we could C prefixes, for example `sg` instead of current `gfx`.
module_names = {
    'slog_':    'slog',
    'sg_':      'sg',
    'sapp_':    'sapp',
    'stm_':     'stm',
    'saudio_':  'saudio',
    'sgl_':     'sgl',
    'sdtx_':    'sdtx',
    'sshape_':  'sshape',
    'sglue_':   'sglue',
}

c_source_names = {
    'slog_':    'sokol_log.c',
    'sg_':      'sokol_gfx.c',
    'sapp_':    'sokol_app.c',
    'sapp_sg':  'sokol_glue.c',
    'stm_':     'sokol_time.c',
    'saudio_':  'sokol_audio.c',
    'sgl_':     'sokol_gl.c',
    'sdtx_':    'sokol_debugtext.c',
    'sshape_':  'sokol_shape.c',
    'sglue_':   'sokol_glue.c',
}

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
    'sg_install_trace_hooks',
    'sg_trace_hooks',
]

overrides = {
    # `any` is treated specially in C3.
    'any': '_any',
    # Constants must be uppercase - lowercase `x` is not allowed.
    'SG_PIXELFORMAT_ASTC_4x4_RGBA': 'SG_PIXELFORMAT_ASTC_4X4_RGBA',
    'SG_PIXELFORMAT_ASTC_4x4_SRGBA': 'SG_PIXELFORMAT_ASTC_4X4_SRGBA',
}

prim_types = {
    'int':          'CInt',
    # TODO: Check whether we should translate to `CBool` instead?
    'bool':         'bool',
    'char':         'char',
    'int8_t':       'ichar',
    'uint8_t':      'char',
    'int16_t':      'short',
    'uint16_t':     'ushort',
    'int32_t':      'int',
    'uint32_t':     'uint',
    'int64_t':      'long',
    'uint64_t':     'ulong',
    'float':        'float',
    'double':       'double',
    'uintptr_t':    'uptr',
    'intptr_t':     'iptr',
    'size_t':       'usz'
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

special_constant_types = {
    "SG_INVALID_ID": "uint",
    "SAPP_MODIFIER_SHIFT": "uint",
    "SAPP_MODIFIER_CTRL": "uint",
    "SAPP_MODIFIER_ALT": "uint",
    "SAPP_MODIFIER_SUPER": "uint",
    "SAPP_MODIFIER_LMB": "uint",
    "SAPP_MODIFIER_RMB": "uint",
    "SAPP_MODIFIER_MMB": "uint",
}

# Aliases for function pointers.
# Function pointers must be aliased - we can't use them directly inside structs,
# instead we have to create an alias and use the alias in the struct.
aliases = {
    # C type -> (Alias name, Right hand side of alias).
    "void (*)(void *)":
        ("DataCb", "fn void(void*)"),
    "void *(*)(size_t, void *)":
        ("AllocCb", "fn void*(usz, void*)"),
    "void (*)(void *, void *)":
        ("FreeCb", "fn void*(usz, void*)"),
    "void (*)(const char *, uint32_t, uint32_t, const char *, uint32_t, const char *, void *)":
        ("LogCb", "fn void(ZString, uint, uint, ZString, uint, ZString, void*)"),
    "void (*)(void)":
        ("Cb", "fn void()"),
    "void (*)(const sapp_event *)":
        ("EventCb", "fn void(SappEvent*)"),
    "void (*)(const sapp_event *, void *)":
        ("EventDataCb", "fn void(SappEvent*, void*)"),
    "void (*)(const sapp_html5_fetch_response *)":
        ("ResponseCb", "fn void(SappHtml5FetchResponse*)"),
    "void (*)(float *, int, int)":
        ("StreamCb", "fn void(float*, CInt, CInt)"),
    "void (*)(float *, int, int, void *)":
        ("StreamDataCb", "fn void(float*, CInt, CInt, void*)"),
}

struct_types = []
enum_types = []
enum_items = {}
# Which alias were used in current module.
# At the end of module we emit only used aliases.
used_aliases = []  # We shouldn't use `set()` because the order differs among runs.
out_lines = ''

def reset_globals():
    global struct_types
    global enum_types
    global enum_items
    global used_aliases
    global out_lines
    struct_types = []
    enum_types = []
    enum_items = {}
    used_aliases = []
    out_lines = ''

def l(s):
    global out_lines
    out_lines += s + '\n'

def check_override(name, default=None):
    if name in overrides:
        return overrides[name]
    elif default is None:
        return name
    else:
        return default

def check_ignore(name):
    return name in ignores

# PREFIX_BLA_BLUB to BLA_BLUB, prefix_bla_blub to bla_blub
def as_snake_case(s, prefix):
    outp = s
    if outp.lower().startswith(prefix):
        outp = outp[len(prefix):]
    return outp

def get_c3_module_path(c_prefix):
    return f'{module_root}'

def get_csource_path(c_prefix):
    return f'{c_root}/{c_source_names[c_prefix]}'

def make_c3_module_directory(c_prefix):
    path = get_c3_module_path(c_prefix)
    if not os.path.isdir(path):
        os.makedirs(path)

def as_prim_type(s):
    return prim_types[s]

def as_upper_snake_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    return outp.upper()

def as_module_name_for_enum_type(enum_name, prefix):
    parts = enum_name.lower().split('_')
    parent_module = module_names[prefix]
    if parts[-1] == 't':
        # Ignore '_t' suffix.
        module = "_".join(parts[1:-1])
    else:
        module = "_".join(parts[1:])
    return f"sokol::{parent_module}::{module}"

def as_parent_module_name_for_enum_type(enum_name, prefix):
    parent_module = module_names[prefix]
    return f"sokol::{parent_module}"

# prefix_bla_blub(_t) => (dep::)BlaBlub
def as_struct_or_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = ''
    for part in parts:
        # ignore '_t' type postfix
        if part != 't':
            outp += part.capitalize()
    return outp

# PREFIX_ENUM_BLA_BLUB => BLA_BLUB, _PREFIX_ENUM_BLA_BLUB => BLA_BLUB
def as_enum_item_name(s):
    outp = s.lstrip('_')
    parts = outp.split('_')[2:]
    outp = '_'.join(parts)
    if outp[0].isdigit():
        if outp in ["2D", "3D"]:
            outp = "TYPE_" + outp
        else:
            outp = 'NUM_' + outp
    return outp

def is_prim_type(s):
    return s in prim_types

def is_int_type(s):
    return s == "int"

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

def map_type(type, prefix, sub_type):
    if sub_type not in ['c_arg', 'struct_field']:
        sys.exit(f"Error: map_type(): unknown sub_type '{sub_type}")
    if type == "void":
        return ""
    elif is_prim_type(type):
        return as_prim_type(type)
    elif is_struct_type(type):
        return as_struct_or_enum_type(type, prefix)
    elif is_enum_type(type):
        return as_struct_or_enum_type(type, prefix)
    elif util.is_void_ptr(type):
        return "void*"
    elif util.is_const_void_ptr(type):
        return "void*"
    elif util.is_string_ptr(type):
        return "ZString"
    elif is_const_struct_ptr(type):
        return f"{as_struct_or_enum_type(util.extract_ptr_type(type), prefix)}*"
    elif is_prim_ptr(type):
        return f"{as_prim_type(util.extract_ptr_type(type))}*"
    elif is_const_prim_ptr(type):
        return f"{as_prim_type(util.extract_ptr_type(type))}*"
    elif util.is_1d_array_type(type):
        array_type = util.extract_array_type(type)
        array_sizes = util.extract_array_sizes(type)
        return f"{map_type(array_type, prefix, sub_type)}[{array_sizes[0]}]"
    elif util.is_2d_array_type(type):
        array_type = util.extract_array_type(type)
        array_sizes = util.extract_array_sizes(type)
        # TODO: Check if the dimensions are in correct order.
        return f"{map_type(array_type, prefix, sub_type)}[{array_sizes[0]}][{array_sizes[1]}]"
    elif util.is_func_ptr(type):
        if type in aliases:
            alias_name, _ = aliases[type]
            if type not in used_aliases:
                used_aliases.append(type)
            return alias_name
        else:
            sys.exit(f"Error map_type(): missing alias for function pointer '{type}'")
    else:
        sys.exit(f"Error map_type(): unknown type '{type}'")

def funcdecl_args_c(decl, prefix):
    s = ''
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != '':
            s += ', '
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += f"{map_type(param_type, prefix, 'c_arg')} {param_name}"
    return s

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    res_c_type = decl_type[:decl_type.index('(')].strip()
    return map_type(check_override(f'{func_name}.RESULT', default=res_c_type), prefix, 'c_arg')

def gen_c_imports(inp, c_prefix, prefix):
    prefix = inp['prefix']
    for decl in inp['decls']:
        if decl['kind'] == 'func' and not decl['is_dep'] and not check_ignore(decl['name']):
            args = funcdecl_args_c(decl, prefix)
            res_type = funcdecl_result_c(decl, prefix)
            res_str = 'void' if res_type == '' else res_type
            l(f'extern fn {res_str} {check_override(as_snake_case(decl["name"], c_prefix))}({args}) @cname("{decl["name"]}");')
    l('')

def gen_consts(decl, prefix):
    for item in decl["items"]:
        #
        # TODO: What type should these constants have? Currently giving all `usz`
        #       unless specifically overridden by `special_constant_types`
        #

        item_name = check_override(item["name"])
        tpe = "usz"
        if item_name in special_constant_types:
            tpe = special_constant_types[item_name]
        l(f"const {tpe} {as_upper_snake_case(item_name, prefix)} = {item['value']};")
    l('')

def gen_struct(decl, prefix):
    c_struct_name = check_override(decl['name'])
    struct_name = as_struct_or_enum_type(c_struct_name, prefix)
    l(f'struct {struct_name}')
    l('{')
    for field in decl['fields']:
        field_name = check_override(field['name'])
        field_type = map_type(check_override(f'{c_struct_name}.{field_name}', default=field['type']), prefix, 'struct_field')
        l(f'    {field_type} {field_name};')
    l('}')
    l('')

def gen_enum(decl, prefix):
    enum_name = check_override(decl['name'])
    tpe = "int"
    if any(as_enum_item_name(check_override(item['name'])) == 'FORCE_U32' for item in decl['items']):
        tpe = "uint"
    l(f'enum {as_struct_or_enum_type(enum_name, prefix)} : const {tpe}')
    l('{')
    value = "-1"
    for item in decl['items']:
        item_name = as_enum_item_name(check_override(item['name']))
        if item_name != 'FORCE_U32':
            if 'value' in item:
                value = item['value']
            else:
                value = str(int(value) + 1)
            l(f"    {item_name} = {value},")
    l('}')
    l('')

def gen_imports(dep_prefixes):
    l(f'import sokol;')
    l('')

def gen_function_pointer_aliases():
    for type in used_aliases:
        alias_name, right_hand_side = aliases[type]
        l(f'alias {alias_name} = {right_hand_side};')
    l('')

def gen_module(inp, c_prefix, dep_prefixes):
    pre_parse(inp)
    l('// machine generated, do not edit')
    l('')
    l(f"module sokol::{module_names[c_prefix]};")
    gen_imports(dep_prefixes)
    prefix = inp['prefix']
    gen_c_imports(inp, c_prefix, prefix)
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
    gen_function_pointer_aliases()

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

def prepare():
    print('=== Generating C3 bindings:')
    if not os.path.isdir(module_root):
        os.makedirs(module_root)
    if not os.path.isdir(c_root):
        os.makedirs(c_root)

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'  >> warning: skipping generation for {c_prefix} prefix...')
        return
    reset_globals()
    make_c3_module_directory(c_prefix)
    print(f'  {c_header_path} => {module_names[c_prefix]}')
    shutil.copyfile(c_header_path, f'{c_root}/{os.path.basename(c_header_path)}')
    csource_path = get_csource_path(c_prefix)
    module_name = module_names[c_prefix]
    ir = gen_ir.gen(c_header_path, csource_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, c_prefix, dep_c_prefixes)
    with open(f"{module_root}/{ir['module']}.c3", 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
