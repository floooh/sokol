#-------------------------------------------------------------------------------
#   Read output of gen_json.py and generate Zig language bindings.
#
#   Nim coding style:
#   - type identifiers are PascalCase, everything else is camelCase
#   - reference: https://nim-lang.org/docs/nep1.html
#-------------------------------------------------------------------------------
import gen_ir
import json, re, os, shutil

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

c_source_paths = {
    'sg_':      'sokol-nim/src/sokol/gen/sokol_gfx.c',
    'sapp_':    'sokol-nim/src/sokol/gen/sokol_app.c',
    'sapp_sg':  'sokol-nim/src/sokol/gen/sokol_glue.c',
    'stm_':     'sokol-nim/src/sokol/gen/sokol_time.c',
    'saudio_':  'sokol-nim/src/sokol/gen/sokol_audio.c',
    'sgl_':     'sokol-nim/src/sokol/gen/sokol_gl.c',
    'sdtx_':    'sokol-nim/src/sokol/gen/sokol_debugtext.c',
    'sshape_':  'sokol-nim/src/sokol/gen/sokol_shape.c',
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

# consts that should be converted to Nim enum bitfields, values mimic C type declarations
const_bitfield_overrides = {
    'SAPP_MODIFIER_': 'sapp_event_modifier',
}

struct_field_type_overrides = {
    'sapp_event.modifiers': 'sapp_event_modifier', # type declared above
    'sapp_allocator.alloc': 'void * (*)(size_t, void *)',
    'sapp_allocator.free': 'void (*)(void *, void *)',
    'sg_allocator.alloc': 'void * (*)(size_t, void *)',
    'sg_allocator.free': 'void (*)(void *, void *)',
    'sgl_allocator_t.alloc': 'void * (*)(size_t, void *)',
    'sgl_allocator_t.free': 'void (*)(void *, void *)',
    'sdtx_allocator_t.alloc': 'void * (*)(size_t, void *)',
    'sdtx_allocator_t.free': 'void (*)(void *, void *)',
    'saudio_allocator.alloc': 'void * (*)(size_t, void *)',
    'saudio_allocator.free': 'void (*)(void *, void *)',
}

prim_types = {
    'int':          'cint',
    'bool':         'bool',
    'char':         'cchar',
    'int8_t':       'int8',
    'uint8_t':      'uint8',
    'int16_t':      'int16',
    'uint16_t':     'uint16',
    'int32_t':      'int32',
    'uint32_t':     'uint32',
    'int64_t':      'int64',
    'uint64_t':     'uint64',
    'float':        'cfloat',
    'double':       'cdouble',
    'uintptr_t':    'uint',
    'intptr_t':     'int',
    'size_t':       'csize_t',
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

re_array = re.compile("([a-z_\d\s\*]+)\s*\[(\d+)\]")

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

def is_power_of_two(val):
    return val == 0 or val & (val - 1) == 0

def check_consts_bitfield_override(decl):
    for override in const_bitfield_overrides:
        if all(override in item['name'] for item in decl['items']):
            if any(not is_power_of_two(int(item['value'])) for item in decl['items']):
                print(f"warning: bitfield override '{override}' encountered non-power-of-two value")
            return const_bitfield_overrides[override]
    return None

def trim_prefix(s, prefix):
    outp = s;
    if outp.lower().startswith(prefix.lower()):
        outp = outp[len(prefix):]
    return outp

def wrap_keywords(s):
    if s in keywords:
        return f'`{s}`'
    else:
        return s

# prefix_bla_blub => blaBlub
def as_camel_case(s, prefix = ""):
    parts = trim_prefix(s, prefix).lower().split('_')
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    return wrap_keywords(outp)

# prefix_bla_blub => BlaBlub
def as_pascal_case(s, prefix):
    parts = trim_prefix(s, prefix).lower().split('_')
    outp = ""
    for part in parts:
        outp += part.capitalize()
    return wrap_keywords(outp)

# PREFIX_ENUM_BLA_BLO => Bla, _PREFIX_ENUM_BLA_BLO => blaBlo
def as_enum_item_name(s):
    outp = s
    if outp.startswith('_'):
        outp = outp[1:]
    parts = outp.lower().split('_')[2:]
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    if outp[0].isdigit():
        outp = 'n' + outp
    return wrap_keywords(outp)

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

def is_array_type(s):
    return re_array.match(s) is not None

def type_default_value(s):
    return prim_defaults[s]

def extract_array_type(s):
    return s[:s.find('[')].strip() + s[s.find(']')+1:].strip()

def extract_array_nums(s):
    return s[s.find('[')+1:s.find(']')].strip()

def extract_ptr_type(s):
    tokens = s.split()
    if tokens[0] == 'const':
        return tokens[1]
    else:
        return tokens[0]

def as_nim_type(ctype, prefix):
    if ctype == "void":
        return ""
    elif is_prim_type(ctype):
        return as_nim_prim_type(ctype)
    elif is_struct_type(ctype):
        return as_nim_type_name(ctype, prefix)
    elif is_enum_type(ctype):
        return as_nim_type_name(ctype, prefix)
    elif is_string_ptr(ctype):
        return "cstring"
    elif is_void_ptr(ctype) or is_const_void_ptr(ctype):
        return "pointer"
    elif is_const_struct_ptr(ctype):
        return f"ptr {as_nim_type(extract_ptr_type(ctype), prefix)}"
    elif is_prim_ptr(ctype) or is_const_prim_ptr(ctype):
        return f"ptr {as_nim_type(extract_ptr_type(ctype), prefix)}"
    elif is_func_ptr(ctype):
        args = funcptr_args(ctype, prefix)
        res = funcptr_res(ctype, prefix)
        if res != "":
            res = ":" + res
        return f"proc({args}){res} {{.cdecl.}}"
    elif is_array_type(ctype):
        array_ctype = extract_array_type(ctype)
        array_nums = extract_array_nums(ctype)
        return f"array[{array_nums}, {as_nim_type(array_ctype, prefix)}]"
    else:
        l(f"// FIXME: {ctype};")

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

def funcptr_res(field_type, prefix):
    ctype = field_type[:field_type.index('(*)')].strip()
    return as_nim_type(ctype, prefix)

def funcdecl_args(decl, prefix):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        s += f"{arg_name}:{as_nim_type(arg_type, prefix)}"
    return s

def funcdecl_res(decl, prefix):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    nim_res_type = as_nim_type(res_type, prefix)
    if nim_res_type == "":
        nim_res_type = "void"
    return nim_res_type

def gen_struct(decl, prefix, use_raw_name=False):
    struct_name = decl['name']
    nim_type = struct_name if use_raw_name else as_nim_type_name(struct_name, prefix)
    l(f"type {nim_type}* = object")
    is_public = True
    for field in decl['fields']:
        field_name = field['name']
        if field_name == "__pad":
            # FIXME: these should be guarded by SOKOL_ZIG_BINDINGS, but aren't?
            continue
        is_public = not field_name.startswith("_")
        field_name = as_camel_case(field_name, "_")
        if is_public:
            field_name += "*"
        field_type = check_struct_field_type_override(decl['name'], field['name'], field['type'])
        l(f"  {field_name}:{as_nim_type(field_type, prefix)}")
    l("")

def gen_consts(decl, prefix):
    l("const")
    for item in decl['items']:
        l(f"  {as_camel_case(trim_prefix(item['name'], prefix), prefix)}* = {item['value']}")
    l("")

def gen_enum(decl, prefix, bitfield=None):
    item_names_by_value = {}
    value = -1
    has_force_u32 = False
    has_explicit_values = False
    for item in decl['items']:
        itemName = item['name']
        if itemName.endswith("_FORCE_U32"):
            has_force_u32 = True
        elif itemName.endswith("_NUM"):
            continue
        else:
            if 'value' in item:
                has_explicit_values = True
                value = int(item['value'])
            else:
                value += 1
            item_names_by_value[value] = as_enum_item_name(item['name']);
    enum_name = bitfield if bitfield is not None else decl['name']
    enum_name_nim = as_nim_type_name(enum_name, prefix)
    l('type')
    if has_force_u32:
        l(f"  {enum_name_nim}* {{.pure, size:sizeof(uint32).}} = enum")
    else:
        l(f"  {enum_name_nim}* {{.pure, size:sizeof(cint).}} = enum")
    if has_explicit_values:
        # Nim requires explicit enum values to be declared in ascending order
        for value in sorted(item_names_by_value):
            name = item_names_by_value[value]
            l(f"    {name} = {value},")
    else:
        for name in item_names_by_value.values():
            l(f"    {name},")
    if bitfield is not None:
        l(f"  {enum_name_nim}s = set[{enum_name_nim}]")
    l("")

def gen_func_nim(decl, prefix):
    c_func_name = decl['name']
    nim_func_name = as_camel_case(decl['name'], prefix)
    nim_res_type = funcdecl_res(decl, prefix)
    l(f"proc {nim_func_name}*({funcdecl_args(decl, prefix)}):{funcdecl_res(decl, prefix)} {{.cdecl, importc:\"{decl['name']}\".}}")
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
    for bitfield in const_bitfield_overrides.values():
        enum_types.append(bitfield)
        enum_types.append(bitfield + 's')

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
                bitfield = check_consts_bitfield_override(decl)
                if bitfield is not None:
                    gen_enum(decl, prefix, bitfield=bitfield)
                else:
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
    if not os.path.isdir('sokol-nim/src/sokol/gen'):
        os.makedirs('sokol-nim/src/sokol/gen')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'warning: skipping generation for {c_prefix} prefix...')
        return
    global out_lines
    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    shutil.copyfile(c_header_path, f'sokol-nim/src/sokol/gen/{os.path.basename(c_header_path)}')
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-nim/src/sokol/{ir['module']}.nim"

    ## some changes for readability
    out_lines = out_lines.replace("pixelformatInfo", "pixelFormatInfo")
    out_lines = out_lines.replace(" dontcare,", " dontCare,")
    out_lines = out_lines.replace(" vertexbuffer,", " vertexBuffer,")
    out_lines = out_lines.replace(" indexbuffer,", " indexBuffer,")
    out_lines = out_lines.replace(" n2d,", " plane,")
    out_lines = out_lines.replace(" n3d,", " volume,")

    ## include extensions in generated code
    l("# Nim-specific API extensions")
    l(f"include extra/{ir['module']}")

    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)
