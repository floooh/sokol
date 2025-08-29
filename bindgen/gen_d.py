#-------------------------------------------------------------------------------
#   Generate D bindings for Sokol library.
#
#   D coding style:
#   - Types: PascalCase
#   - Functions: camelCase
#   - Variables: snake_case
#   - Doc-comments: /++ ... +/ for declarations, /// for fields, with proper wrapping
#-------------------------------------------------------------------------------
import gen_ir
import os
import shutil
import sys
import textwrap
import logging

import gen_util as util

# Configure logging for debugging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

module_names = {
    'slog_':    'log',
    'sg_':      'gfx',
    'sapp_':    'app',
    'sargs_':   'args',
    'stm_':     'time',
    'saudio_':  'audio',
    'sgl_':     'gl',
    'sdtx_':    'debugtext',
    'sshape_':  'shape',
    'sglue_':   'glue',
    'sfetch_':  'fetch',
    'simgui_':  'imgui',
    'sgimgui_': 'gfximgui',
    'snk_':     'nuklear',
    'smemtrack_': 'memtrack',
}

c_source_paths = {
    'slog_':    'sokol-d/src/sokol/c/sokol_log.c',
    'sg_':      'sokol-d/src/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-d/src/sokol/c/sokol_app.c',
    'sargs_':   'sokol-d/src/sokol/c/sokol_args.c',
    'stm_':     'sokol-d/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-d/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-d/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-d/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-d/src/sokol/c/sokol_shape.c',
    'sglue_':   'sokol-d/src/sokol/c/sokol_glue.c',
    'sfetch_':  'sokol-d/src/sokol/c/sokol_fetch.c',
    'simgui_':  'sokol-d/src/sokol/c/sokol_imgui.c',
    'sgimgui_': 'sokol-d/src/sokol/c/sokol_gfx_imgui.c',
    'snk_':     'sokol-d/src/sokol/c/sokol_nuklear.c',
    'smemtrack_': 'sokol-d/src/sokol/c/sokol_memtrack.c',
}

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
]

c_callbacks = [
    'slog_func',
    'nk_plugin_filter',
]

overrides = {
    'ref':                                  '_ref',
    'immutable':                            '_immutable',
    'sgl_error':                            'sgl_get_error',
    'sgl_deg':                              'sgl_as_degrees',
    'sgl_rad':                              'sgl_as_radians',
    'sg_apply_uniforms.ub_slot':            'uint32_t',
    'sg_draw.base_element':                 'uint32_t',
    'sg_draw.num_elements':                 'uint32_t',
    'sg_draw.num_instances':                'uint32_t',
    'sg_dispatch.num_groups_x':             'uint32_t',
    'sg_dispatch.num_groups_y':             'uint32_t',
    'sg_dispatch.num_groups_z':             'uint32_t',
    'sshape_element_range_t.base_element':  'uint32_t',
    'sshape_element_range_t.num_elements':  'uint32_t',
    'sdtx_font.font_index':                 'uint32_t',
    'SGL_NO_ERROR':                         'SGL_ERROR_NO_ERROR',
    'sfetch_continue':                      'continue_fetching',
    'struct nk_context':                    'NkContext',
    'nk_handle':                            'NkHandle',
    'nk_flags':                             'NkFlags',
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
    'char':         '0',
    'int8_t':       '0',
    'uint8_t':      '0',
    'int16_t':      '0',
    'uint16_t':      '0',
    'int32_t':      '0',
    'uint32_t':      '0',
    'int64_t':      '0',
    'uint64_t':      '0',
    'float':        '0.0f',
    'double':       '0.0',
    'uintptr_t':    '0',
    'intptr_t':     '0',
    'size_t':       '0'
}

class TypeConverter:
    def __init__(self, prefix, struct_types, enum_types):
        self.prefix = prefix
        self.struct_types = struct_types
        self.enum_types = enum_types

    def as_d_type(self, c_type, is_param=False, decl_name=None):
        c_type = c_type.strip()
        # Check for override first
        if c_type in overrides:
            return overrides[c_type]

        d_to_c_types = {v: k for k, v in prim_types.items()}
        if c_type in d_to_c_types:
            c_type = d_to_c_types[c_type]

        # Handle struct keyword in type (e.g., "struct nk_context *")
        if c_type.startswith('struct '):
            c_type = c_type[7:].strip()  # Remove 'struct' prefix
            if c_type in overrides:
                return overrides[c_type]
            if c_type.endswith('*'):
                # External struct pointer, treat as opaque
                return 'void*' if is_param else 'void*'
            # External struct by value, treat as named type if overridden
            return overrides.get(c_type, c_type)

        if util.is_func_ptr(c_type):
            return self.as_func_ptr_type(c_type, decl_name)
        
        if c_type == "void":
            return "" if is_param else "void"
        
        if c_type in prim_types:
            return prim_types[c_type]
        
        if c_type in self.struct_types:
            return self.as_d_struct_type(c_type)
        
        if c_type in self.enum_types:
            return self.as_d_enum_type(c_type)
        
        if util.is_void_ptr(c_type):
            return "void*"
        
        if util.is_const_void_ptr(c_type):
            return "const(void)*"
        
        if util.is_string_ptr(c_type):
            return "const(char)*"
        
        if self.is_struct_ptr(c_type) or self.is_const_struct_ptr(c_type):
            struct_type = util.extract_ptr_type(c_type.strip())
            d_type = self.as_d_struct_type(struct_type)
            return f"const {d_type}*" if is_param else f"{d_type}*"
        
        if self.is_prim_ptr(c_type):
            prim_type = util.extract_ptr_type(c_type.strip())
            return f"{prim_types[prim_type]}*"
        
        if self.is_const_prim_ptr(c_type):
            prim_type = util.extract_ptr_type(c_type.strip())
            return f"const {prim_types[prim_type]}*"
        
        if util.is_array_type(c_type):
            return self.as_array_type(c_type)
        
        # Handle external types (e.g., nk_handle, nk_flags) not in struct_types or prim_types
        if c_type not in self.struct_types and c_type not in prim_types:
            if c_type.endswith('*'):
                # Treat pointer to unknown type as void*
                return 'void*' if is_param else 'void*'
            # Treat unknown type by value as itself (assuming it's defined elsewhere)
            return overrides.get(c_type, c_type)

        raise ValueError(f"Unsupported C type: {c_type} in declaration: {decl_name or 'unknown'}")

    def as_d_struct_type(self, s):
        parts = s.lower().split('_')
        outp = '' if s.startswith(self.prefix) else f'{parts[0]}.'
        name = ''.join(part.capitalize() for part in parts[1:] if part != 't')
        if not name:
            name = parts[0].capitalize()
        return outp + name

    def as_d_enum_type(self, s):
        return self.as_d_struct_type(s)

    def parse_func_ptr_signature(self, c_type, decl, decl_name):
        if '(*)' in c_type:
            return_type = c_type[:c_type.index('(*)')].strip()
            params_str = c_type[c_type.index('(*)')+4:-1].strip()
            params = [p.strip() for p in params_str.split(',')] if params_str else []
        else:
            return_type = decl.get('return_type', 'void')
            params = [param['type'] for param in decl.get('params', [])]
        return return_type, params

    def as_func_ptr_type(self, c_type, decl_name, decl=None):
        return_type, params = self.parse_func_ptr_signature(c_type, decl or {}, decl_name)
        d_return_type = self.as_d_type(return_type, decl_name=decl_name)
        arg_types = []
        for param_type in params:
            if param_type and param_type != "void":
                try:
                    arg_types.append(self.as_d_type(param_type, is_param=True, decl_name=decl_name))
                except ValueError as e:
                    raise ValueError(f"Unsupported function pointer parameter type: {param_type} in {c_type} for declaration: {decl_name or 'unknown'}")
        args_str = ", ".join(arg_types) if arg_types else ""
        return f"extern(C) {d_return_type} function({args_str})"

    def as_array_type(self, c_type):
        array_type = util.extract_array_type(c_type)
        array_sizes = util.extract_array_sizes(c_type)
        base_type = self.as_d_type(array_type)
        dims = ''.join(f"[{size}]" for size in array_sizes)
        return f"{base_type}{dims}"

    def default_value(self, c_type):
        return prim_defaults.get(c_type, "")

    def is_struct_ptr(self, s):
        s = s.strip()
        return any(s == f"{struct_type} *" for struct_type in self.struct_types)

    def is_const_struct_ptr(self, s):
        s = s.strip()
        return any(s == f"const {struct_type} *" for struct_type in self.struct_types)

    def is_prim_ptr(self, s):
        s = s.strip()
        return any(s == f"{prim_type} *" for prim_type in prim_types)

    def is_const_prim_ptr(self, s):
        s = s.strip()
        return any(s == f"const {prim_type} *" for prim_type in prim_types)

# Global state
struct_types = []
enum_types = []
enum_items = {}
out_lines = ''

def reset_globals():
    global struct_types, enum_types, enum_items, out_lines
    struct_types = []
    enum_types = []
    enum_items = {}
    out_lines = ''

def l(s):
    global out_lines
    out_lines += s + '\n'

def format_comment(comment, indent="", multiline=False):
    if not comment:
        return
    comment = comment.strip()
    # Escape nested comment delimiters to ensure valid D code
    comment = comment.replace('/++', '/+ /').replace('+/', '/ +/')
    if multiline:
        # Split by newlines to preserve empty lines
        lines = [line.rstrip() for line in comment.split('\n')]
        l(f"{indent}/++")
        for line in lines:
            l(f"{indent}+ {line}")
        l(f"{indent}+/")
    else:
        for line in comment.split('\n'):
            l(f"{indent}/// {line.strip()}")

def as_enum_item_name(s):
    outp = s.lstrip('_').split('_', 2)[-1].capitalize()
    return '_' + outp if outp[0].isdigit() else outp

def pre_parse(inp):
    global struct_types, enum_types, enum_items
    for decl in inp['decls']:
        if decl['kind'] == 'struct':
            struct_types.append(decl['name'])
        elif decl['kind'] == 'enum':
            enum_name = decl['name']
            enum_types.append(enum_name)
            enum_items[enum_name] = [as_enum_item_name(item['name']) for item in decl['items']]

def gen_nuklear_types():
    """Generate type declarations for Nuklear external types."""
    l("/++ Nuklear external type declarations +/")
    l("extern(C) struct NkContext;")
    l("extern(C) union NkHandle {")
    l("    void* ptr;")
    l("    int id;")
    l("}")
    l("alias NkFlags = uint;")
    l("alias nk_plugin_filter = extern(C) int function(const(NkContext)*, NkHandle, int*, int) @system @nogc nothrow;")

def gen_struct(decl, type_converter):
    struct_name = overrides.get(decl['name'], decl['name'])
    d_type = type_converter.as_d_struct_type(struct_name)
    format_comment(decl.get('comment', ''), multiline=True)
    l(f"extern(C) struct {d_type} {{")
    used_field_names = set()
    for field in decl['fields']:
        field_key = f"{struct_name}.{field['name']}"
        field_name = overrides.get(field['name'], field['name'])
        field_type = overrides.get(field_key, field['type'])

        if field_name in used_field_names or field_name in prim_types.values():
            field_name = f"{field_name}_field"
        used_field_names.add(field_name)

        d_type_str = type_converter.as_d_type(field_type, decl_name=field_key)
        default = type_converter.default_value(field_type)
        
        if default:
            default_value = f" = {default}"
        elif util.is_func_ptr(field_type):
            default_value = " = null"
        elif type_converter.is_struct_ptr(field_type) or type_converter.is_const_struct_ptr(field_type):
            default_value = " = null"
        elif util.is_void_ptr(field_type) or util.is_const_void_ptr(field_type) or util.is_string_ptr(field_type):
            default_value = " = null"
        elif field_type in type_converter.struct_types:
            default_value = " = {}"
        elif field_type in type_converter.enum_types:
            enum_name = field_type
            if enum_name in enum_items and enum_items[enum_name]:
                default_value = f" = {type_converter.as_d_enum_type(enum_name)}.{enum_items[enum_name][0]}"
            else:
                default_value = " = 0"
        elif util.is_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if array_type in prim_types and array_sizes:
                # Handle all primitive arrays with proper defaults
                default_value = f" = [{', '.join([prim_defaults[array_type]] * int(array_sizes[0]))}]"
            elif array_type in type_converter.struct_types or array_type in type_converter.enum_types:
                default_value = " = []"
            else:
                default_value = " = null"
        else:
            default_value = ""

        format_comment(field.get('comment', ''), "    ")
        l(f"    {d_type_str} {field_name}{default_value};")
    l("}")

def gen_consts(decl, prefix):
    format_comment(decl.get('comment', ''), multiline=True)
    for item in decl['items']:
        item_name = overrides.get(item['name'], item['name'])
        format_comment(item.get('comment', ''))
        l(f"enum {util.as_lower_snake_case(item_name, prefix)} = {item['value']};")

def gen_enum(decl, type_converter):
    enum_name = overrides.get(decl['name'], decl['name'])
    format_comment(decl.get('comment', ''), multiline=True)
    l(f"enum {type_converter.as_d_enum_type(enum_name)} {{")
    for item in decl['items']:
        item_name = as_enum_item_name(overrides.get(item['name'], item['name']))
        if item_name != "Force_u32":
            format_comment(item.get('comment', ''))
            l(f"    {item_name}{f' = {item['value']}' if 'value' in item else ''},")
    l("}")

def gen_func(decl, type_converter, prefix):
    c_func_name = decl['name']
    d_func_name = util.as_lower_camel_case(overrides.get(c_func_name, c_func_name), prefix)
    format_comment(decl.get('comment', ''), multiline=True)

    if c_func_name == 'slog_func':
        params = []
        for param in decl['params']:
            param_name = param['name']
            param_key = f"{c_func_name}.{param_name}"
            param_type = overrides.get(param_key, param['type'])
            param_d_type = type_converter.as_d_type(param_type, is_param=True, decl_name=param_key)
            params.append(f"{param_d_type} {param_name}")
        params_str = ", ".join(params) if params else ""
        result_type = type_converter.as_d_type(decl['type'][:decl['type'].index('(')].strip(), decl_name=c_func_name)
        l(f"extern(C) {result_type} {c_func_name}({params_str}) @system @nogc nothrow pure;")
        format_comment(decl.get('comment', ''), multiline=True)
        l(f"alias func = {c_func_name};")
        return

    if c_func_name in c_callbacks:
        l(f"alias {d_func_name} = {type_converter.as_func_ptr_type(decl['type'], c_func_name, decl)};")
        return

    params = []
    wrapper_params = []
    call_args = []
    for param in decl['params']:
        param_name = param['name']
        param_key = f"{c_func_name}.{param_name}"
        param_type = overrides.get(param_key, param['type'])
        param_d_type = type_converter.as_d_type(param_type, is_param=True, decl_name=param_key)
        wrapper_d_type = param_d_type
        if type_converter.is_struct_ptr(param_type) or type_converter.is_const_struct_ptr(param_type):
            wrapper_d_type = f"scope ref {type_converter.as_d_struct_type(util.extract_ptr_type(param_type.strip()))}"
        params.append(f"{param_d_type} {param_name}")
        wrapper_params.append(f"{wrapper_d_type} {param_name}")
        if 'scope ref' in wrapper_d_type:
            call_args.append(f"&{param_name}")
        else:
            call_args.append(param_name)

    result_type = type_converter.as_d_type(decl['type'][:decl['type'].index('(')].strip(), decl_name=c_func_name)
    params_str = ", ".join(params) if params else ""
    wrapper_params_str = ", ".join(wrapper_params) if wrapper_params else ""
    call_args_str = ", ".join(call_args) if call_args else ""

    l(f"extern(C) {result_type} {c_func_name}({params_str}) @system @nogc nothrow pure;")
    l(f"{result_type} {d_func_name}({wrapper_params_str}) @trusted @nogc nothrow pure {{")
    l(f"    {'return ' if result_type != 'void' else ''}{c_func_name}({call_args_str});")
    l("}")

def gen_imports(inp, dep_prefixes):
    for dep_prefix in dep_prefixes:
        if dep_prefix in module_names:
            l(f'import {dep_prefix[:-1]} = sokol.{module_names[dep_prefix]};')
    l('')

def gen_module(inp, dep_prefixes, c_header_path):
    reset_globals()
    header_comment = f"""
    Machine generated D bindings for Sokol library.
        
    Source header: {os.path.basename(c_header_path)}
    Module: sokol.{inp['module']}
    
    Do not edit manually; regenerate using gen_d.py.
    """
    format_comment(header_comment, multiline=True)
    l(f'module sokol.{inp["module"]};')
    logging.info(f"Generating imports for module {inp['module']}")
    gen_imports(inp, dep_prefixes)
    # Add Nuklear types for the nuklear module
    if inp['module'] == 'nuklear':
        gen_nuklear_types()
    pre_parse(inp)
    type_converter = TypeConverter(inp['prefix'], struct_types, enum_types)
    for decl in inp['decls']:
        if not decl.get('is_dep', False) and (decl.get('kind', '') != 'func' or decl['name'] not in ignores):
            if decl['kind'] == 'consts':
                gen_consts(decl, inp['prefix'])
            elif decl['kind'] == 'struct':
                gen_struct(decl, type_converter)
            elif decl['kind'] == 'enum':
                gen_enum(decl, type_converter)
            elif decl['kind'] == 'func':
                gen_func(decl, type_converter, inp['prefix'])

def prepare():
    logging.info("Preparing directories for D bindings generation")
    print('=== Generating D bindings:')
    os.makedirs('sokol-d/src/sokol/c', exist_ok=True)

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not os.path.isfile(c_header_path):
        raise FileNotFoundError(f"Header file not found: {c_header_path}")
    if c_prefix not in module_names:
        logging.warning(f"Skipping generation for prefix {c_prefix}")
        print(f' >> warning: skipping generation for {c_prefix} prefix...')
        return
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    logging.info(f"Generating bindings for {c_header_path} => {module_name}")
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-d/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes, with_comments=True)
    gen_module(ir, dep_c_prefixes, c_header_path)
    output_path = f"sokol-d/src/sokol/{ir['module']}.d"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)