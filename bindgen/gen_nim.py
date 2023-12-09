#-------------------------------------------------------------------------------
#   Generate Nim bindings
#
#   Nim coding style:
#   - type identifiers are PascalCase, everything else is camelCase
#   - reference: https://nim-lang.org/docs/nep1.html
#-------------------------------------------------------------------------------
import gen_ir
import gen_util as util
import os, shutil, sys

module_names = {
    'slog_':    'log',
    'sg_':      'gfx',
    'sapp_':    'app',
    'sapp_sg':  'glue',
    'stm_':     'time',
    'saudio_':  'audio',
    'sgl_':     'gl',
    'sdtx_':    'debugtext',
    'sshape_':  'shape',
}

c_source_paths = {
    'slog_':    'sokol-nim/src/sokol/c/sokol_log.c',
    'sg_':      'sokol-nim/src/sokol/c/sokol_gfx.c',
    'sapp_':    'sokol-nim/src/sokol/c/sokol_app.c',
    'sapp_sg':  'sokol-nim/src/sokol/c/sokol_glue.c',
    'stm_':     'sokol-nim/src/sokol/c/sokol_time.c',
    'saudio_':  'sokol-nim/src/sokol/c/sokol_audio.c',
    'sgl_':     'sokol-nim/src/sokol/c/sokol_gl.c',
    'sdtx_':    'sokol-nim/src/sokol/c/sokol_debugtext.c',
    'sshape_':  'sokol-nim/src/sokol/c/sokol_shape.c',
}

c_callbacks = [
    'slog_func',
]

ignores = [
    'sdtx_printf',
    'sdtx_vprintf',
]

overrides = {
    'sgl_error':                    'sgl_get_error',
    'sgl_deg':                      'sgl_as_degrees',
    'sgl_rad':                      'sgl_as_radians',
    'sg_context_desc.color_format': 'int',
    'sg_context_desc.depth_format': 'int',
    'SGL_NO_ERROR':                 'SGL_ERROR_NO_ERROR',
    'SG_BUFFERTYPE_VERTEXBUFFER':   'SG_BUFFERTYPE_VERTEX_BUFFER',
    'SG_BUFFERTYPE_INDEXBUFFER':    'SG_BUFFERTYPE_INDEX_BUFFER',
    'SG_ACTION_DONTCARE':           'SG_ACTION_DONT_CARE',
    'ptr':                          'addr', # range ptr
    'func':                         'fn',
    'slog_func':                    'fn',
}

enumPrefixOverrides = {
    # sokol_gfx.h
    'LOADACTION': 'loadAction',
    'STOREACTION': 'storeAction',
    'PIXELFORMAT': 'pixelFormat',
    'RESOURCESTATE': 'resourceState',
    'BUFFERTYPE': 'bufferType',
    'INDEXTYPE': 'indexType',
    'IMAGETYPE': 'imageType',
    'SAMPLERTYPE': 'samplerType',
    'CUBEFACE': 'cubeFace',
    'SHADERSTAGE': 'shaderStage',
    'PRIMITIVETYPE': 'primitiveType',
    'BORDERCOLOR': 'borderColor',
    'VERTEXFORMAT': 'vertexFormat',
    'VERTEXSTEP': 'vertexStep',
    'UNIFORMTYPE': 'uniformType',
    'UNIFORMLAYOUT': 'uniformLayout',
    'CULLMODE': 'cullMode',
    'FACEWINDING': 'faceWinding',
    'COMPAREFUNC': 'compareFunc',
    'STENCILOP': 'stencilOp',
    'BLENDFACTOR': 'blendFactor',
    'BLENDOP': 'blendOp',
    'COLORMASK': 'colorMask',
    # sokol_app.h
    'EVENTTYPE': 'eventType',
    'KEYCODE': 'keyCode',
    'MOUSEBUTTON': 'mouseButton',
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
    'size_t':       'int',  # not a bug, Nim's sizeof() returns int
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

common_prim_types = """
array
untyped typed void
bool byte char
int int8 int16 int32 int64
uint uint8 uint16 uint32 uint64
float float32 float64
string
cchar cint csize_t
cfloat cdouble
cstring
pointer
""".split()

keywords = """
addr and as asm
bind block break
case cast concept const continue converter
defer discard distinct div do
elif else end enum except export
finally for from func
if import in include interface is isnot iterator
let
macro method mixin mod
nil not notin
object of or out
proc ptr
raise ref return
shl shr static
template try tuple type
using
var
when while
xor
yield
""".split() + common_prim_types

struct_types = []
enum_types = []
out_lines = ''

def reset_globals():
    global struct_types
    global enum_types
    global out_lines
    struct_types = []
    enum_types = []
    out_lines = ''

def l(s):
    global out_lines
    out_lines += s + '\n'

def as_nim_prim_type(s):
    return prim_types[s]

# prefix_bla_blub(_t) => (dep.)BlaBlub
def as_nim_type_name(s, prefix):
    parts = s.lower().split('_')
    dep = parts[0] + '_'
    outp = ''
    if not s.startswith(prefix) and dep in module_names:
        outp = module_names[dep] + '.'
    for part in parts[1:]:
        # ignore '_t' type postfix
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

def is_power_of_two(val):
    return val == 0 or val & (val - 1) == 0

def wrap_keywords(s):
    if s in keywords:
        return f'`{s}`'
    else:
        return s

# prefix_bla_blub => blaBlub
def as_camel_case(s, prefix, wrap=True):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    parts = outp.lstrip('_').split('_')
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    if wrap:
        outp = wrap_keywords(outp)
    return outp

# PREFIX_ENUM_BLA_BLO => blaBlo
def as_enum_item_name(s, wrap=True):
    outp = s.lstrip('_')
    parts = outp.split('_')[1:]
    if parts[0] in enumPrefixOverrides:
        parts[0] = enumPrefixOverrides[parts[0]]
    else:
        parts[0] = parts[0].lower()
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    if wrap:
        outp = wrap_keywords(outp)
    return outp

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

def funcptr_args(field_type, prefix):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ""
    n = 0
    for token in tokens:
        n += 1
        arg_ctype = token.strip()
        if s != "":
            s += ", "
        arg_nimtype = as_nim_type(arg_ctype, prefix)
        if arg_nimtype == "":
            return "" # fun(void)
        s += f"a{n}:{arg_nimtype}"
    if s == "a1:void":
        s = ""
    return s

def funcptr_result(field_type, prefix):
    ctype = field_type[:field_type.index('(*)')].strip()
    return as_nim_type(ctype, prefix)

def as_nim_type(ctype, prefix, struct_ptr_as_value=False):
    if ctype == "void":
        return ""
    elif is_prim_type(ctype):
        return as_nim_prim_type(ctype)
    elif is_struct_type(ctype):
        return as_nim_type_name(ctype, prefix)
    elif is_enum_type(ctype):
        return as_nim_type_name(ctype, prefix)
    elif util.is_string_ptr(ctype):
        return "cstring"
    elif util.is_void_ptr(ctype) or util.is_const_void_ptr(ctype):
        return "pointer"
    elif is_const_struct_ptr(ctype):
        nim_type = as_nim_type(util.extract_ptr_type(ctype), prefix)
        if struct_ptr_as_value:
            return f"{nim_type}"
        else:
            return f"ptr {nim_type}"
    elif is_prim_ptr(ctype) or is_const_prim_ptr(ctype):
        return f"ptr {as_nim_type(util.extract_ptr_type(ctype), prefix)}"
    elif util.is_func_ptr(ctype):
        args = funcptr_args(ctype, prefix)
        res = funcptr_result(ctype, prefix)
        if res != "":
            res = ":" + res
        return f"proc({args}){res} {{.cdecl.}}"
    elif util.is_1d_array_type(ctype):
        array_ctype = util.extract_array_type(ctype)
        array_sizes = util.extract_array_sizes(ctype)
        return f'array[{array_sizes[0]}, {as_nim_type(array_ctype, prefix)}]'
    elif util.is_2d_array_type(ctype):
        array_ctype = util.extract_array_type(ctype)
        array_sizes = util.extract_array_sizes(ctype)
        return f'array[{array_sizes[0]}, array[{array_sizes[1]}, {as_nim_type(array_ctype, prefix)}]]'
    else:
        sys.exit(f"ERROR as_nim_type: {ctype}")

def as_nim_struct_name(struct_decl, prefix):
    struct_name = check_override(struct_decl['name'])
    nim_type = f'{as_nim_type_name(struct_name, prefix)}'
    return nim_type

def as_nim_field_name(field_decl, prefix, check_private=True):
    field_name = as_camel_case(check_override(field_decl['name']), prefix)
    if check_private:
        is_private = field_decl['name'].startswith('_')
        if not is_private:
            field_name += "*"
    return field_name

def as_nim_field_type(struct_decl, field_decl, prefix):
    return as_nim_type(check_override(f"{struct_decl['name']}.{field_decl['name']}", default=field_decl['type']), prefix)

def gen_struct(decl, prefix):
    l(f"type {as_nim_struct_name(decl, prefix)}* = object")
    for field in decl['fields']:
        l(f"  {as_nim_field_name(field, prefix)}:{as_nim_field_type(decl, field, prefix)}")
    l("")

def gen_consts(decl, prefix):
    l("const")
    for item in decl['items']:
        item_name = check_override(item['name'])
        l(f"  {as_camel_case(item_name, prefix)}* = {item['value']}")
    l("")

def gen_enum(decl, prefix):
    item_names_by_value = {}
    value = -1
    has_explicit_values = False
    for item in decl['items']:
        item_name = check_override(item['name'])
        if item_name.endswith("_NUM") or item_name.endswith("_FORCE_U32"):
            continue
        else:
            if 'value' in item:
                has_explicit_values = True
                value = int(item['value'])
            else:
                value += 1
            item_names_by_value[value] = as_enum_item_name(item_name)
    enum_name_nim = as_nim_type_name(decl['name'], prefix)
    l('type')
    l(f"  {enum_name_nim}* {{.size:sizeof(int32).}} = enum")
    if has_explicit_values:
        # Nim requires explicit enum values to be declared in ascending order
        for value in sorted(item_names_by_value):
            name = item_names_by_value[value]
            l(f"    {name} = {value},")
    else:
        for name in item_names_by_value.values():
            l(f"    {name},")
    l("")

# returns C prototype compatible function args (with pointers)
def funcdecl_args_c(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = check_override(f'{func_name}.{arg_name}', default=param_decl['type'])
        s += f"{as_camel_case(arg_name, prefix)}:{as_nim_type(arg_type, prefix)}"
    return s

# returns Nim function args (pass structs by value)
def funcdecl_args_nim(decl, prefix):
    s = ""
    func_name = decl['name']
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = check_override(f'{func_name}.{arg_name}', default=param_decl['type'])
        s += f"{as_camel_case(arg_name, prefix)}:{as_nim_type(arg_type, prefix, struct_ptr_as_value=True)}"
    return s

def funcdecl_result(decl, prefix):
    func_name = decl['name']
    decl_type = decl['type']
    result_type = check_override(f'{func_name}.RESULT', default=decl_type[:decl_type.index('(')].strip())
    nim_res_type = as_nim_type(result_type, prefix)
    if nim_res_type == "":
        nim_res_type = "void"
    return nim_res_type

def gen_func_nim(decl, prefix):
    c_func_name = decl['name']
    nim_func_name = as_camel_case(check_override(c_func_name), prefix, wrap=False)
    nim_res_type = funcdecl_result(decl, prefix)
    if c_func_name in c_callbacks:
        l(f"proc {nim_func_name}*({funcdecl_args_c(decl, prefix)}):{nim_res_type} {{.cdecl, importc:\"{c_func_name}\".}}")
    else:
        l(f"proc c_{nim_func_name}({funcdecl_args_c(decl, prefix)}):{nim_res_type} {{.cdecl, importc:\"{c_func_name}\".}}")
        l(f"proc {wrap_keywords(nim_func_name)}*({funcdecl_args_nim(decl, prefix)}):{nim_res_type} =")
        s = f"    c_{nim_func_name}("
        for i, param_decl in enumerate(decl['params']):
            if i > 0:
                s += ", "
            arg_name = param_decl['name']
            arg_type = param_decl['type']
            if is_const_struct_ptr(arg_type):
                s += f"addr({arg_name})"
            else:
                s += arg_name
        s += ")"
        l(s)
    l("")

def gen_array_converters(decl, prefix):
    for field in decl['fields']:
        if util.is_array_type(field['type']):
            array_type = util.extract_array_type(field['type'])
            array_sizes = util.extract_array_sizes(field['type'])
            struct_name = as_nim_struct_name(decl, prefix)
            field_name = as_nim_field_name(field, prefix, check_private=False)
            array_base_type = as_nim_type(array_type, prefix)
            if util.is_1d_array_type(field['type']):
                n = array_sizes[0]
                l(f'converter to{struct_name}{field_name}*[N:static[int]](items: array[N, {array_base_type}]): array[{n}, {array_base_type}] =')
                l(f'  static: assert(N <= {n})')
                l(f'  for index,item in items.pairs: result[index]=item')
                l('')
            elif util.is_2d_array_type(field['type']):
                x = array_sizes[1]
                y = array_sizes[0]
                l(f'converter to{struct_name}{field_name}*[Y:static[int], X:static[int]](items: array[Y, array[X, {array_base_type}]]): array[{y}, array[{x}, {array_base_type}]] =')
                l(f'  static: assert(X <= {x})')
                l(f'  static: assert(Y <= {y})')
                l(f'  for indexY,itemY in items.pairs:')
                l(f'    for indexX, itemX in itemY.pairs:')
                l(f'      result[indexY][indexX] = itemX')
                l('')
            else:
                sys.exit('Unsupported converter array dimension (> 2)!')

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

def gen_imports(inp, dep_prefixes):
    for dep_prefix in dep_prefixes:
        dep_module_name = module_names[dep_prefix]
        l(f'import {dep_module_name}')
    l('')

def gen_extra(inp):
    if inp['prefix'] in ['sg_']:
        # FIXME: remove when sokol-shdc has been integrated!
        l('when defined gl:')
        l('  const gl*    = true')
        l('  const d3d11* = false')
        l('  const metal* = false')
        l('elif defined windows:')
        l('  const gl*    = false')
        l('  const d3d11* = true')
        l('  const metal* = false')
        l('elif defined macosx:')
        l('  const gl*    = false')
        l('  const d3d11* = false')
        l('  const metal* = true')
        l('elif defined linux:')
        l('  const gl*    = true')
        l('  const d3d11* = false')
        l('  const metal* = false')
        l('else:')
        l('  error("unsupported platform")')
        l('')
    if inp['prefix'] in ['sg_', 'sapp_']:
        l('when defined windows:')
        l('  when not defined vcc:')
        l('    {.passl:"-lkernel32 -luser32 -lshell32 -lgdi32".}')
        l('  when defined gl:')
        l('    {.passc:"-DSOKOL_GLCORE33".}')
        l('  else:')
        l('    {.passc:"-DSOKOL_D3D11".}')
        l('    when not defined vcc:')
        l('      {.passl:"-ld3d11 -ldxgi".}')
        l('elif defined macosx:')
        l('  {.passc:"-x objective-c".}')
        l('  {.passl:"-framework Cocoa -framework QuartzCore".}')
        l('  when defined gl:')
        l('    {.passc:"-DSOKOL_GLCORE33".}')
        l('    {.passl:"-framework OpenGL".}')
        l('  else:')
        l('    {.passc:"-DSOKOL_METAL".}')
        l('    {.passl:"-framework Metal -framework MetalKit".}')
        l('elif defined linux:')
        l('  {.passc:"-DSOKOL_GLCORE33".}')
        l('  {.passl:"-lX11 -lXi -lXcursor -lGL -lm -ldl -lpthread".}')
        l('else:')
        l('  error("unsupported platform")')
        l('')
    if inp['prefix'] in ['saudio_']:
        l('when defined windows:')
        l('  when not defined vcc:')
        l('    {.passl:"-lkernel32 -lole32".}')
        l('elif defined macosx:')
        l('  {.passl:"-framework AudioToolbox".}')
        l('elif defined linux:')
        l('  {.passl:"-lasound -lm -lpthread".}')
        l('else:')
        l('  error("unsupported platform")')
        l('')
    if inp['prefix'] in ['sg_']:
        l('## Convert a 4-element tuple of numbers to a gfx.Color')
        l('converter toColor*[R:SomeNumber,G:SomeNumber,B:SomeNumber,A:SomeNumber](rgba: tuple [r:R,g:G,b:B,a:A]):Color =')
        l('  Color(r:rgba.r.float32, g:rgba.g.float32, b:rgba.b.float32, a:rgba.a.float32)')
        l('')
        l('## Convert a 3-element tuple of numbers to a gfx.Color')
        l('converter toColor*[R:SomeNumber,G:SomeNumber,B:SomeNumber](rgba: tuple [r:R,g:G,b:B]):Color =')
        l('  Color(r:rgba.r.float32, g:rgba.g.float32, b:rgba.b.float32, a:1.float32)')
        l('')
    # NOTE: this simplistic to_Range() converter has various issues, some of them dangerous:
    #   - doesn't work as expected for slice types
    #   - it's very easy to create a range that points to invalid memory
    #     (so far observed for stack-allocated structs <= 16 bytes)
    #if inp['prefix'] in ['sg_', 'sdtx_', 'sshape_']:
    #    l('# helper function to convert "anything" into a Range')
    #    l('converter to_Range*[T](source: T): Range =')
    #    l('  Range(addr: source.addr, size: source.sizeof.uint)')
    #    l('')
    c_source_path = '/'.join(c_source_paths[inp['prefix']].split('/')[3:])
    l('{.passc:"-DSOKOL_NIM_IMPL".}')
    l('when defined(release):')
    l('  {.passc:"-DNDEBUG".}')
    l(f'{{.compile:"{c_source_path}".}}')

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
            elif not check_ignore(decl['name']):
                if kind == 'struct':
                    gen_struct(decl, prefix)
                    gen_array_converters(decl, prefix)
                elif kind == 'enum':
                    gen_enum(decl, prefix)
                elif kind == 'func':
                    gen_func_nim(decl, prefix)
    gen_extra(inp)

def prepare():
    print('=== Generating Nim bindings:')
    if not os.path.isdir('sokol-nim/src/sokol'):
        os.makedirs('sokol-nim/src/sokol')
    if not os.path.isdir('sokol-nim/src/sokol/c'):
        os.makedirs('sokol-nim/src/sokol/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'  >> warning: skipping generation for {c_prefix} prefix...')
        return
    global out_lines
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-nim/src/sokol/c/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-nim/src/sokol/{ir['module']}.nim"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
