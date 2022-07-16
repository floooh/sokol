#-------------------------------------------------------------------------------
#   gen_odin.py
#
#   Generate Odin bindings.
#-------------------------------------------------------------------------------
import gen_ir
import re, os, shutil, sys

module_root = 'sokol-odin/src/sokol'

module_names = {
    'sg_':      'gfx',
    'sapp_':    'app',
    'sapp_sg':  'glue',
    'stm_':     'time',
    'saudio_':  'audio',
    'sgl_':     'gl',
    'sdtx_':    'debugtext',
    'sshape_':  'shape',
}

c_source_names = {
    'sg_':      'sokol_gfx.c',
    'sapp_':    'sokol_app.c',
    'sapp_sg':  'sokol_glue.c',
    'stm_':     'sokol_time.c',
    'saudio_':  'sokol_audio.c',
    'sgl_':     'sokol_gl.c',
    'sdtx_':    'sokol_debugtext.c',
    'sshape_':  'sokol_shape.c',
}

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
    'sg_install_trace_hooks',
    'sg_trace_hooks',
]

# NOTE: syntax for function results: "func_name.RESULT"
overrides = {
    'sgl_deg':                              'sgl_as_degrees',
    'sgl_rad':                              'sgl_as_radians',
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
    'bool':         'b8',
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
    'uintptr_t':    'u64',
    'intptr_t':     'i64',
    'size_t':       'u64'
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

re_1d_array = re.compile("^(?:const )?\w*\s\*?\[\d*\]$")
re_2d_array = re.compile("^(?:const )?\w*\s\*?\[\d*\]\[\d*\]$")

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

def get_odin_module_path(c_prefix):
    return f'{module_root}/{module_names[c_prefix]}'

def get_csource_path(c_prefix):
    return f'{module_root}/c/{c_source_names[c_prefix]}'

def make_odin_module_directory(c_prefix):
    path = get_odin_module_path(c_prefix)
    if not os.path.isdir(path):
        os.makedirs(path)

def as_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)Bla_Blub
def as_struct_or_enum_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        # ignore '_t' type postfix
        if (part != 't'):
            outp += part.capitalize()
            outp += '_'
    outp = outp[:-1]
    return outp

# PREFIX_ENUM_BLA_BLUB => BLA_BLUB, _PREFIX_ENUM_BLA_BLUB => BLA_BLUB
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
    return re_1d_array.match(s) is not None

def is_2d_array_type(s):
    return re_2d_array.match(s) is not None

def type_default_value(s):
    return prim_defaults[s]

def extract_array_type(s):
    return s[:s.index('[')].strip()

def extract_array_sizes(s):
    return s[s.index('['):].replace('[', ' ').replace(']', ' ').split()

def extract_ptr_type(s):
    tokens = s.split()
    if tokens[0] == 'const':
        return tokens[1]
    else:
        return tokens[0]

def as_c_arg_type(arg_type, prefix):
    if arg_type == "void":
        return ""
    elif is_prim_type(arg_type):
        return as_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_struct_or_enum_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_struct_or_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return "rawptr"
    elif is_const_void_ptr(arg_type):
        return "rawptr"
    elif is_string_ptr(arg_type):
        return "cstring"
    elif is_const_struct_ptr(arg_type):
        return f"^{as_struct_or_enum_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return f"^{as_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return f"^{as_prim_type(extract_ptr_type(arg_type))}"
    else:
        sys.exit(f"Error as_c_arg_type(): {arg_type}")

def as_odin_arg_type(arg_type, prefix):
    if arg_type == "void":
        return ""
    elif is_prim_type(arg_type):
        # for args and return values we'll map the C int type (32-bit) to Odin's pointer-sized int type,
        # and the C bool type to Odin's 'unsized' bool type
        if arg_type == 'int':
            return 'int'
        elif arg_type == 'bool':
            return 'bool'
        else:
            return as_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_struct_or_enum_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_struct_or_enum_type(arg_type, prefix)
    elif is_void_ptr(arg_type):
        return "rawptr"
    elif is_const_void_ptr(arg_type):
        return "rawptr"
    elif is_string_ptr(arg_type):
        return "cstring"
    elif is_const_struct_ptr(arg_type):
        # not a bug, pass structs by value
        return f"{as_struct_or_enum_type(extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return f"^{as_prim_type(extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return f"^{as_prim_type(extract_ptr_type(arg_type))}"
    else:
        sys.exit(f"Error as_odin_arg_type(): {arg_type}")

def funcdecl_args_c(decl, prefix):
    s = ''
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != '':
            s += ', '
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += f"{param_name}: {as_c_arg_type(param_type, prefix)}"
    return s

def funcdecl_args_odin(decl, prefix):
    s = ''
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != '':
            s += ', '
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += f"{param_name}: {as_odin_arg_type(param_type, prefix)}"
    return s

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    res_c_type = decl_type[:decl_type.index('(')].strip()
    return as_c_arg_type(check_override(f'{func_name}.RESULT', default=res_c_type), prefix)

def funcdecl_result_odin(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    res_c_type = decl_type[:decl_type.index('(')].strip()
    return as_odin_arg_type(check_override(f'{func_name}.RESULT', default=res_c_type), prefix)

def gen_c_imports(inp):
    l(f'// FIXME: foreign import...\n')
    l('@(default_calling_convention="c")')
    l(f"foreign sokol_{inp['module']}_clib {{")
    prefix = inp['prefix']
    for decl in inp['decls']:
        if decl['kind'] == 'func' and not decl['is_dep'] and not check_ignore(decl['name']):
            args = funcdecl_args_c(decl, prefix)
            ret_type = funcdecl_result_c(decl, prefix)
            ret_str = '' if ret_type == '' else f'-> {ret_type}'
            l(f"    {decl['name']} :: proc({args}) {ret_str} ---")
    l('}')

def gen_consts(decl, prefix):
    for item in decl['items']:
        item_name = check_override(item['name'])
        l(f"{as_snake_case(item_name, prefix)} :: {item['value']};")

def gen_struct(decl, prefix):
    # FIXME
    l(f'// FIXME: struct {decl["name"]}')

def gen_enum(decl, prefix):
    enum_name = check_override(decl['name'])
    l(f'{as_struct_or_enum_type(enum_name, prefix)} :: enum i32 {{')
    for item in decl['items']:
        item_name = as_enum_item_name(check_override(item['name']))
        if item_name != 'FORCE_U32':
            if 'value' in item:
                l(f"    {item_name} = {item['value']},")
            else:
                l(f"    {item_name},")
    l('};')

def gen_func(decl, prefix):
    c_func_name = decl['name']
    args = funcdecl_args_odin(decl, prefix)
    ret_type = funcdecl_result_odin(decl, prefix)
    ret_str = '' if ret_type == '' else f'-> {ret_type}'
    if ret_type != funcdecl_result_c(decl, prefix):
        # cast needed for return type
        ret_cast = f'cast({ret_type})'
    else:
        ret_cast = ''
    l(f"{as_snake_case(decl['name'], prefix)} :: proc({args}) {ret_str} {{")
    s = '    '
    if ret_type == '':
        # void result
        s += f"{c_func_name}("
    else:
        s += f"return {ret_cast}{c_func_name}("
    for i, param_decl in enumerate(decl['params']):
        if i > 0:
            s += ', '
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        if is_const_struct_ptr(arg_type):
            s += f"&{arg_name}"
        else:
            odin_arg_type = as_odin_arg_type(arg_type, prefix)
            c_arg_type = as_c_arg_type(arg_type, prefix)
            if odin_arg_type != c_arg_type:
                cast = f'cast({c_arg_type})'
            else:
                cast = ''
            s += f'{cast}{arg_name}'
    s += ');'
    l(s)
    l('}')

def gen_module(inp, dep_prefixes):
    pre_parse(inp)
    l('// machine generated, do not edit')
    l('')
    l(f"package sokol_{inp['module']}\n")
    gen_c_imports(inp)
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
                    gen_func(decl, prefix)

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
    print('=== Generating Odin bindings:')
    if not os.path.isdir(f'{module_root}/c'):
        os.makedirs(f'{module_root}/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'  >> warning: skipping generation for {c_prefix} prefix...')
        return
    reset_globals()
    make_odin_module_directory(c_prefix)
    print(f'  {c_header_path} => {module_names[c_prefix]}')
    shutil.copyfile(c_header_path, f'{module_root}/c/{os.path.basename(c_header_path)}')
    csource_path = get_csource_path(c_prefix)
    module_name = module_names[c_prefix]
    ir = gen_ir.gen(c_header_path, csource_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    with open(f"{module_root}/{ir['module']}/{ir['module']}.odin", 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)



