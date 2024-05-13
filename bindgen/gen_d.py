#-------------------------------------------------------------------------------
#   Generate D bindings.
#
#   D coding style:
#   - types are PascalCase
#   - functions are camelCase
#   - otherwise snake_case
#-------------------------------------------------------------------------------
import gen_ir
import os
import shutil
import sys

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
}

c_source_paths = {
    'slog_':    'sokol-d/src/sokol/c/sokol_log.c',
    'sg_':      'sokol-d/src/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-d/src/sokol/c/sokol_app.c',
    'stm_':     'sokol-d/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-d/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-d/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-d/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-d/src/sokol/c/sokol_shape.c',
    'sglue_':   'sokol-d/src/sokol/c/sokol_glue.c',
}

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
]

# functions that need to be exposed as 'raw' C callbacks without a Dlang wrapper function
c_callbacks = [
    'slog_func'
]

# NOTE: syntax for function results: "func_name.RESULT"
overrides = {
    'ref':                                  '_ref',
    'sgl_error':                            'sgl_get_error',   # 'error' is reserved in Dlang
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
    'SGL_NO_ERROR':                         'SGL_ERROR_NO_ERROR',
}

prim_types = {
    "int":          "int",
    "bool":         "bool",
    "char":         "char",
    "int8_t":       "byte",
    "uint8_t":      "ubyte",
    "int16_t":      "short",
    "uint16_t":     "ushort",
    "int32_t":      "int",
    "uint32_t":     "uint",
    "int64_t":      "long",
    "uint64_t":     "ulong",
    "float":        "float",
    "double":       "double",
    "uintptr_t":    "ulong",
    "intptr_t":     "long",
    "size_t":       "size_t",
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

def l(s):
    global out_lines
    out_lines += s + '\n'

def as_d_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_d_struct_type(s, prefix):
    parts = s.lower().split('_')
    outp = '' if s.startswith(prefix) else f'{parts[0]}.'
    for part in parts[1:]:
        # ignore '_t' type postfix
        if (part != 't'):
            outp += part.capitalize()
    return outp

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_d_enum_type(s, prefix):
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
    outp = outp.capitalize()
    if outp[0].isdigit():
        outp = '_' + outp.capitalize()
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

def is_struct_ptr(s):
    for struct_type in struct_types:
        if s == f"{struct_type} *":
            return True
    return False

def type_default_value(s):
    return prim_defaults[s]

def as_c_arg_type(arg_type, prefix):
    if arg_type == "void":
        return "void"
    elif is_prim_type(arg_type):
        return as_d_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_d_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return as_d_enum_type(arg_type, prefix)
    elif util.is_void_ptr(arg_type):
        return "void*"
    elif util.is_const_void_ptr(arg_type):
        return "const(void)*"
    elif util.is_string_ptr(arg_type):
        return "const(char)*"
    elif is_const_struct_ptr(arg_type):
        return f"const {as_d_struct_type(util.extract_ptr_type(arg_type), prefix)} *"
    elif is_prim_ptr(arg_type):
        return f"{as_d_prim_type(util.extract_ptr_type(arg_type))} *"
    elif is_const_prim_ptr(arg_type):
        return f"const {as_d_prim_type(util.extract_ptr_type(arg_type))} *"
    else:
        sys.exit(f"Error as_c_arg_type(): {arg_type}")

def as_d_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return "void"
        else:
            return ""
    elif is_prim_type(arg_type):
        return as_d_prim_type(arg_type) + pre
    elif is_struct_type(arg_type):
        return as_d_struct_type(arg_type, prefix) + pre
    elif is_enum_type(arg_type):
        return as_d_enum_type(arg_type, prefix) + pre
    elif util.is_void_ptr(arg_type):
        return "scope void*" + pre
    elif util.is_const_void_ptr(arg_type):
        return "scope const(void)*" + pre
    elif util.is_string_ptr(arg_type):
        return "scope const(char)*" + pre
    elif is_struct_ptr(arg_type):
        return f"scope ref {as_d_struct_type(util.extract_ptr_type(arg_type), prefix)}" + pre
    elif is_const_struct_ptr(arg_type):
        return f"scope ref {as_d_struct_type(util.extract_ptr_type(arg_type), prefix)}" + pre
    elif is_prim_ptr(arg_type):
        return f"scope {as_d_prim_type(util.extract_ptr_type(arg_type))} *" + pre
    elif is_const_prim_ptr(arg_type):
        return f"scope const {as_d_prim_type(util.extract_ptr_type(arg_type))} *" + pre
    else:
        sys.exit(f"ERROR as_d_arg_type(): {arg_type}")

def is_d_string(d_type):
    return d_type == "string"

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
        return 'void'
    elif is_prim_type(res_type):
        return as_d_prim_type(res_type)
    elif util.is_const_void_ptr(res_type):
        return 'const(void)*'
    elif util.is_void_ptr(res_type):
        return 'void*'
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

def funcdecl_args_d(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        param_name = param_decl['name']
        param_type = check_override(f'{func_name}.{param_name}', default=param_decl['type'])
        s += f"{as_d_arg_type(f' {param_name}', param_type, prefix)}"
    return s

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_override(f'{func_name}.RESULT', default=decl_type[:decl_type.index('(')].strip())
    return as_c_arg_type(result_type, prefix)

def funcdecl_result_d(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_override(f'{func_name}.RESULT', default=decl_type[:decl_type.index('(')].strip())
    d_res_type = as_d_arg_type(None, result_type, prefix)
    if is_d_string(d_res_type):
        d_res_type = "string"
    return d_res_type

def gen_struct(decl, prefix):
    struct_name = check_override(decl['name'])
    d_type = as_d_struct_type(struct_name, prefix)
    l(f"extern(C)\nstruct {d_type} {{")
    for field in decl['fields']:
        field_name = check_override(field['name'])
        field_type = check_override(f'{struct_name}.{field_name}', default=field['type'])
        if is_prim_type(field_type):
            l(f"    {as_d_prim_type(field_type)} {field_name} = {type_default_value(field_type)};")
        elif is_struct_type(field_type):
            l(f"    {as_d_struct_type(field_type, prefix)} {field_name};")
        elif is_enum_type(field_type):
            l(f"    {as_d_enum_type(field_type, prefix)} {field_name};")
        elif util.is_string_ptr(field_type):
            l(f"    const(char)* {field_name} = null;")
        elif util.is_const_void_ptr(field_type):
            l(f"    const(void)* {field_name} = null;")
        elif util.is_void_ptr(field_type):
            l(f"    void* {field_name} = null;")
        elif is_const_prim_ptr(field_type):
            l(f"    const {as_d_prim_type(util.extract_ptr_type(field_type))} = null;")
        elif util.is_func_ptr(field_type):
            l(f"    extern(C) {funcptr_result_c(field_type)} function({funcptr_args_c(field_type, prefix)}) {field_name} = null;")
        elif util.is_1d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    d_type = as_d_prim_type(array_type)
                    def_val = type_default_value(array_type)
                elif is_struct_type(array_type):
                    d_type = as_d_struct_type(array_type, prefix)
                    def_val = ''
                elif is_enum_type(array_type):
                    d_type = as_d_enum_type(array_type, prefix)
                    def_val = ''
                else:
                    sys.exit(f"ERROR gen_struct is_1d_array_type: {array_type}")
                t0 = f"{d_type}[{array_sizes[0]}]"
                t1 = f"{d_type}[]"
                if def_val != '':
                    l(f"    {t0} {field_name} = {def_val};")
                else:
                    l(f"    {t0} {field_name};")
            elif util.is_const_void_ptr(array_type):
                l(f"    const(void)*[{array_sizes[0]}] {field_name} = null;")
            else:
                sys.exit(f"ERROR gen_struct: array {field_name}: {field_type} => {array_type} [{array_sizes[0]}]")
        elif util.is_2d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if is_prim_type(array_type):
                d_type = as_d_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                d_type = as_d_struct_type(array_type, prefix)
                def_val = ''
            else:
                sys.exit(f"ERROR gen_struct is_2d_array_type: {array_type}")
            t0 = f"{d_type}[{array_sizes[0]}][{array_sizes[1]}]"
            if def_val != '':
                l(f"    {t0} {field_name} = {def_val};")
            else:
                l(f"    {t0} {field_name};")
        else:
            sys.exit(f"ERROR gen_struct: {field_type} {field_name};")
    l("}")

def gen_consts(decl, prefix):
    for item in decl['items']:
        item_name = check_override(item['name'])
        l(f"enum {util.as_lower_snake_case(item_name, prefix)} = {item['value']};")

def gen_enum(decl, prefix):
    enum_name = check_override(decl['name'])
    l(f"enum {as_d_enum_type(enum_name, prefix)} {{")
    for item in decl['items']:
        item_name = as_enum_item_name(check_override(item['name']))
        if item_name != "Force_u32":
            if 'value' in item:
                l(f"    {item_name} = {item['value']},")
            else:
                l(f"    {item_name},")
    l("}")

def gen_func_c(decl, prefix):
    l(f"extern(C) {funcdecl_result_c(decl, prefix)} {decl['name']}({funcdecl_args_c(decl, prefix)}) @system @nogc nothrow;")

def gen_func_d(decl, prefix):
    c_func_name = decl['name']
    d_func_name = util.as_lower_camel_case(check_override(decl['name']), prefix)
    if c_func_name in c_callbacks:
        # a simple forwarded C callback function
        l(f"alias {d_func_name} = {c_func_name};")
    else:
        d_res_type = funcdecl_result_d(decl, prefix)
        l(f"{d_res_type} {d_func_name}({funcdecl_args_d(decl, prefix)}) @trusted @nogc nothrow {{")
        if d_res_type != 'void':
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
        if is_d_string(d_res_type):
            s += ")"
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
        l(f'import {dep_prefix[:-1]} = sokol.{dep_module_name};')
    l('')

def gen_module(inp, dep_prefixes):
    l('// machine generated, do not edit')
    l('')
    l(f'module sokol.{inp["module"]};')
    gen_imports(inp, dep_prefixes)
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
                    gen_func_d(decl, prefix)

def prepare():
    print('=== Generating d bindings:')
    if not os.path.isdir('sokol-d/src/sokol'):
        os.makedirs('sokol-d/src/sokol')
    if not os.path.isdir('sokol-d/src/sokol/c'):
        os.makedirs('sokol-d/src/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f' >> warning: skipping generation for {c_prefix} prefix...')
        return
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-d/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-d/src/sokol/{ir['module']}.d"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
