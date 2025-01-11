#-------------------------------------------------------------------------------
#   gen_jai.py
#
#   Generate Jai bindings.
#-------------------------------------------------------------------------------
import textwrap
import gen_ir
import gen_util as util
import os, shutil, sys

bindings_root = 'sokol-jai'
c_root = f'{bindings_root}/sokol/c'
module_root = f'{bindings_root}/sokol'

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

system_libs = {
    'sg_': {
        'windows': {
            'd3d11': '#system_library,link_always "gdi32"; #system_library,link_always "dxguid"; #system_library,link_always "user32"; #system_library,link_always "shell32"; #system_library,link_always "d3d11";',
            'gl': '#system_library,link_always "gdi32"; #system_library,link_always "dxguid"; #system_library,link_always "user32"; #system_library,link_always "shell32";',
        },
        'macos': {
            'metal': '#library,link_always "../../libclang_rt.osx"; #system_library,link_always "Cocoa"; #system_library,link_always "QuartzCore"; #system_library,link_always "Metal"; #system_library,link_always "MetalKit";',
            'gl': '#system_library,link_always "Cocoa"; #system_library,link_always "QuartzCore"; #system_library,link_always "OpenGL";',
        },
        'linux': {
            'gl': '#system_library,link_always "libXcursor"; #system_library,link_always "libX11"; #system_library,link_always "libXi"; #system_library,link_always "libGL";',
        }
    },
    'sapp_': {
        'windows': {
            'd3d11': '#system_library,link_always "gdi32"; #system_library,link_always "dxguid"; #system_library,link_always "user32"; #system_library,link_always "shell32";',
            'gl': '#system_library,link_always "gdi32"; #system_library,link_always "dxguid"; #system_library,link_always "user32"; #system_library,link_always "shell32";',
        },
        'macos': {
            'metal': '#library "../../libclang_rt.osx"; #system_library,link_always "Cocoa"; #system_library,link_always "QuartzCore";',
            'gl': '#system_library,link_always "Cocoa"; #system_library,link_always "QuartzCore";',
        },
        'linux': {
            'gl': '#system_library,link_always "libXcursor"; #system_library,link_always "libX11"; #system_library,link_always "libXi"; #system_library,link_always "libGL";',
        }
    },
    'saudio_': {
        'windows': {
            'd3d11': '#system_library,link_always "ole32";',
            'gl': '#system_library,link_always "ole32";',
        },
        'macos': {
            'metal': '#system_library,link_always "AudioToolbox";',
            'gl': '#system_library,link_always "AudioToolbox";',
        },
        'linux': {
            'gl': '#system_library,link_always "asound"; #system_library,link_always "dl"; #system_library,link_always "pthread";',
        }
    }
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
]

# NOTE: syntax for function results: "func_name.RESULT"
overrides = {
    'context':                              'ctx',  # reserved keyword
    'SGL_NO_ERROR':                         'SGL_ERROR_NO_ERROR',
}

prim_types = {
    'int':          's32',
    'bool':         'bool',
    'char':         'u8',
    'int8_t':       's8',
    'uint8_t':      'u8',
    'int16_t':      's16',
    'uint16_t':     'u16',
    'int32_t':      's32',
    'uint32_t':     'u32',
    'int64_t':      's64',
    'uint64_t':     'u64',
    'float':        'float',
    'double':       'float64',
    'uintptr_t':    'u64',
    'intptr_t':     's64',
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

def c(s, indent=""):
    if not s:
        return
    if '\n' in s:
        l(f'{indent}/*')
        l(textwrap.indent(textwrap.dedent(s), prefix=f"    {indent}"))
        l(f'{indent}*/')
    else:
        l(f'{indent}// {s.strip()}')

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

def get_jai_module_path(c_prefix):
    return f'{module_root}/{module_names[c_prefix]}'

def get_csource_path(c_prefix):
    return f'{c_root}/{c_source_names[c_prefix]}'

def make_jai_module_directory(c_prefix):
    path = get_jai_module_path(c_prefix)
    if not os.path.isdir(path):
        os.makedirs(path)

def as_prim_type(s):
    return prim_types[s]

def as_struct_or_enum_type(s, prefix):
    return s

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
    elif is_struct_type(type):
        return as_struct_or_enum_type(type, prefix)
    elif is_enum_type(type):
        return as_struct_or_enum_type(type, prefix)
    elif util.is_void_ptr(type):
        return "*void"
    elif util.is_const_void_ptr(type):
        return "*void"
    elif util.is_string_ptr(type):
        return "*u8"
    elif is_const_struct_ptr(type):
        return f"*{as_struct_or_enum_type(util.extract_ptr_type(type), prefix)}"
    elif is_prim_ptr(type):
        return f"*{as_prim_type(util.extract_ptr_type(type))}"
    elif is_const_prim_ptr(type):
        return f"*{as_prim_type(util.extract_ptr_type(type))}"
    elif util.is_1d_array_type(type):
        array_type = util.extract_array_type(type)
        array_sizes = util.extract_array_sizes(type)
        return f"[{array_sizes[0]}]{map_type(array_type, prefix, sub_type)}"
    elif util.is_2d_array_type(type):
        array_type = util.extract_array_type(type)
        array_sizes = util.extract_array_sizes(type)
        return f"[{array_sizes[0]}][{array_sizes[1]}]{map_type(array_type, prefix, sub_type)}"
    elif util.is_func_ptr(type):
        res_type = funcptr_result_c(type, prefix)
        res_str = '' if res_type == '' else f' -> {res_type}'
        return f'({funcptr_args_c(type, prefix)}){res_str} #c_call'
    elif is_prim_type(type):
        return as_prim_type(type)
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
        s += f"{param_name}: {map_type(param_type, prefix, 'c_arg')}"
    return s

def funcptr_args_c(field_type, prefix):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ''
    arg_index = 0
    for token in tokens:
        arg_type = token.strip()
        if s != '':
            s += ', '
        c_arg = map_type(arg_type, prefix, 'c_arg')
        if c_arg == '':
            return ''
        else:
            s += f'a{arg_index}: {c_arg}'
        arg_index += 1
    return s

def funcptr_result_c(field_type, prefix):
    res_type = field_type[:field_type.index('(*)')].strip()
    return map_type(res_type, prefix, 'c_arg')

def funcdecl_result_c(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    res_c_type = decl_type[:decl_type.index('(')].strip()
    return map_type(check_override(f'{func_name}.RESULT', default=res_c_type), prefix, 'c_arg')

def get_system_libs(module, platform, backend):
    if module in system_libs:
        if platform in system_libs[module]:
            if backend in system_libs[module][platform]:
                libs = system_libs[module][platform][backend]
                if libs != '':
                    return f"{libs}"
    return ''

def gen_c_imports(inp, c_prefix, prefix):
    module_name = inp["module"]
    clib_prefix = f'sokol_{module_name}'
    clib_import = f'{clib_prefix}_clib'
    windows_d3d11_libs = get_system_libs(prefix, 'windows', 'd3d11')
    windows_gl_libs = get_system_libs(prefix, 'windows', 'gl')
    macos_metal_libs = get_system_libs(prefix, 'macos', 'metal')
    macos_gl_libs = get_system_libs(prefix, 'macos', 'gl')
    linux_gl_libs = get_system_libs(prefix, 'linux', 'gl')
    l( '#module_parameters(DEBUG := false, USE_GL := false, USE_DLL := false);')
    l( '')
    l( '#scope_export;')
    l( '')
    l( '#if OS == .WINDOWS {')
    l( '    #if USE_DLL {')
    l( '        #if USE_GL {')
    l(f'            {windows_gl_libs}')
    l(f'            #if  DEBUG {{ {clib_import} :: #library "{clib_prefix}_windows_x64_gl_debug";   }}')
    l(f'            else       {{ {clib_import} :: #library "{clib_prefix}_windows_x64_gl_release"; }}')
    l( '        } else {')
    l(f'            {windows_d3d11_libs}')
    l(f'            #if  DEBUG {{ {clib_import} :: #library "{clib_prefix}_windows_x64_d3d11_debug";   }}')
    l(f'            else       {{ {clib_import} :: #library "{clib_prefix}_windows_x64_d3d11_release"; }}')
    l( '        }')
    l( '    } else {')
    l( '        #if USE_GL {')
    l(f'            {windows_gl_libs}')
    l(f'            #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_windows_x64_gl_debug";   }}')
    l(f'            else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_windows_x64_gl_release"; }}')
    l( '        } else {')
    l(f'            {windows_d3d11_libs}')
    l(f'            #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_windows_x64_d3d11_debug";   }}')
    l(f'            else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_windows_x64_d3d11_release"; }}')
    l( '        }')
    l( '    }')
    l( '}')
    l( 'else #if OS == .MACOS {')
    l( '    #if USE_DLL {')
    l(f'             #if  USE_GL && CPU == .ARM64 &&  DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_arm64_gl_debug.dylib"; }}')
    l(f'        else #if  USE_GL && CPU == .ARM64 && !DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_arm64_gl_release.dylib"; }}')
    l(f'        else #if  USE_GL && CPU == .X64   &&  DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_x64_gl_debug.dylib"; }}')
    l(f'        else #if  USE_GL && CPU == .X64   && !DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_x64_gl_release.dylib"; }}')
    l(f'        else #if !USE_GL && CPU == .ARM64 &&  DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_arm64_metal_debug.dylib"; }}')
    l(f'        else #if !USE_GL && CPU == .ARM64 && !DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_arm64_metal_release.dylib"; }}')
    l(f'        else #if !USE_GL && CPU == .X64   &&  DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_x64_metal_debug.dylib"; }}')
    l(f'        else #if !USE_GL && CPU == .X64   && !DEBUG {{ {clib_import} :: #library "../dylib/sokol_dylib_macos_x64_metal_release.dylib"; }}')
    l( '    } else {')
    l( '        #if USE_GL {')
    l(f'            {macos_gl_libs}')
    l( '            #if CPU == .ARM64 {')
    l(f'                #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_arm64_gl_debug";   }}')
    l(f'                else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_arm64_gl_release"; }}')
    l( '            } else {')
    l(f'                #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_x64_gl_debug";   }}')
    l(f'                else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_x64_gl_release"; }}')
    l( '            }')
    l( '        } else {')
    l(f'            {macos_metal_libs}')
    l( '            #if CPU == .ARM64 {')
    l(f'                #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_arm64_metal_debug";   }}')
    l(f'                else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_arm64_metal_release"; }}')
    l( '            } else {')
    l(f'                #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_x64_metal_debug";   }}')
    l(f'                else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_macos_x64_metal_release"; }}')
    l( '            }')
    l( '        }')
    l( '    }')
    l( '} else #if OS == .LINUX {')
    if linux_gl_libs:
        l(f'    {linux_gl_libs}')
    l(f'    #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_linux_x64_gl_debug";   }}')
    l(f'    else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_linux_x64_gl_release"; }}')
    l( '} else #if OS == .WASM {')
    l(f'    #if  DEBUG {{ {clib_import} :: #library,no_dll "{clib_prefix}_wasm_gl_debug";   }}')
    l(f'    else       {{ {clib_import} :: #library,no_dll "{clib_prefix}_wasm_gl_release"; }}')
    l( '} else {')
    l( '    log_error("This OS is currently not supported");')
    l( '}')
    l( '')

    prefix = inp['prefix']
    for decl in inp['decls']:
        if decl['kind'] == 'func' and not decl['is_dep'] and not check_ignore(decl['name']):
            args = funcdecl_args_c(decl, prefix)
            res_type = funcdecl_result_c(decl, prefix)
            res_str = '-> void' if res_type == '' else f'-> {res_type}'
            if decl.get('comment'):
                c(decl['comment'])
            l(f"{decl['name']} :: ({args}) {res_str} #foreign {clib_import};")
    l('')

def gen_consts(decl, prefix):
    for item in decl['items']:
        item_name = check_override(item['name'])
        c(item.get('comment'))
        l(f"{as_snake_case(item_name, prefix)} :: {item['value']};")
    l('')

def gen_struct(decl, prefix):
    c_struct_name = check_override(decl['name'])
    struct_name = as_struct_or_enum_type(c_struct_name, prefix)
    l(f'{struct_name} :: struct {{')
    for field in decl['fields']:
        field_name = check_override(field['name'])
        field_type = map_type(check_override(f'{c_struct_name}.{field_name}', default=field['type']), prefix, 'struct_field')
        # any field name starting with _ is considered private
        if field_name.startswith('_'):
            l(f'    _ : {field_type};')
        else:
            l(f'    {field_name} : {field_type};')
    l('}')
    l('')

def gen_enum(decl, prefix):
    enum_name = check_override(decl['name'])
    if decl.get('comment'):
        c(decl['comment'])
    l(f'{as_struct_or_enum_type(enum_name, prefix)} :: enum u32 {{')
    for item in decl['items']:
        item_name = as_enum_item_name(check_override(item['name']))
        if item_name != 'FORCE_U32' and item_name != 'NUM':
            if 'value' in item:
                l(f"    {item_name} :: {item['value']};")
            else:
                l(f"    {item_name};")
    l('}')
    l('')

def gen_imports(dep_prefixes):
    for dep_prefix in dep_prefixes:
        dep_module_name = module_names[dep_prefix]
        l(f'#import,dir "../{dep_module_name}"(DEBUG = USE_DLL, USE_GL = USE_DLL, USE_DLL = USE_DLL);')
    l('')

def gen_helpers(inp):
    if inp['prefix'] == 'sdtx_':
        l('sdtx_printf :: (s: string, args: ..Any) {')
        l('    #import "Basic";')
        l('    fstr := tprint(s, ..args);')
        l('    sdtx_putr(to_c_string(fstr), xx fstr.count);')
        l('}')

def gen_module(inp, c_prefix, dep_prefixes):
    pre_parse(inp)
    l('// machine generated, do not edit')
    if inp.get('comment'):
        l('')
        c(inp['comment'])
    gen_imports(dep_prefixes)
    gen_helpers(inp)
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
    print('=== Generating Jai bindings:')
    if not os.path.isdir(module_root):
        os.makedirs(module_root)
    if not os.path.isdir(c_root):
        os.makedirs(c_root)

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'  >> warning: skipping generation for {c_prefix} prefix...')
        return
    reset_globals()
    make_jai_module_directory(c_prefix)
    print(f'  {c_header_path} => {module_names[c_prefix]}')
    shutil.copyfile(c_header_path, f'{c_root}/{os.path.basename(c_header_path)}')
    csource_path = get_csource_path(c_prefix)
    module_name = module_names[c_prefix]
    ir = gen_ir.gen(c_header_path, csource_path, module_name, c_prefix, dep_c_prefixes, with_comments=True)
    gen_module(ir, c_prefix, dep_c_prefixes)
    with open(f"{module_root}/{ir['module']}/module.jai", 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
