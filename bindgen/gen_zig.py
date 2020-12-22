#-------------------------------------------------------------------------------
#   Read output of gen_json.py and generate Zig language bindings.
#
#   Zig coding style:
#   - types are PascalCase
#   - functions are camelCase
#   - otherwise snake_case
#-------------------------------------------------------------------------------
import json, re, os

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

prim_types = {
    'int':          'i32',
    'bool':         'bool',
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
    'uintptr_t':    'usize',
    'intptr_t':     'isize'
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
    'intptr_t':     '0'
}

struct_field_type_overrides = {
    'sg_context_desc.color_format': 'int',
    'sg_context_desc.depth_format': 'int',
}

def l(s):
    global out_lines
    out_lines += s + '\n'

def as_zig_prim_type(s):
    return prim_types[s]

def check_struct_field_type_override(struct_name, field_name, orig_type):
    s = f"{struct_name}.{field_name}"
    if s in struct_field_type_overrides:
        return struct_field_type_overrides[s]
    else:
        return orig_type

# PREFIX_BLA_BLUB to bla_blub
def as_snake_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    return outp

# prefix_bla_blub => BlaBlub
def as_title_case(s):
    parts = s.lower().split('_')[1:]
    outp = ''
    for part in parts:
        outp += part.capitalize()
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

def is_prim_ptr(s):
    for prim_type in prim_types:
        if s == f"const {prim_type} *":
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

def as_extern_c_arg_type(arg_type):
    if arg_type == "void":
        return "void"
    elif is_prim_type(arg_type):
        return as_zig_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return as_title_case(arg_type)
    elif is_enum_type(arg_type):
        return as_title_case(arg_type)
    elif is_void_ptr(arg_type):
        return "?*c_void"
    elif is_const_void_ptr(arg_type):
        return "?*const c_void"
    elif is_string_ptr(arg_type):
        return "[*c]const u8"
    elif is_const_struct_ptr(arg_type):
        return f"[*c]const {as_title_case(extract_ptr_type(arg_type))}"
    else:
        return '???'

def as_zig_arg_type(arg_prefix, arg_type):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix
    if arg_type == "void":
        if arg_prefix is None:
            return "void"
        else:
            return ""
    elif is_prim_type(arg_type):
        return pre + as_zig_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_title_case(arg_type)
    elif is_enum_type(arg_type):
        return pre + as_title_case(arg_type)
    elif is_void_ptr(arg_type):
        return pre + "?*c_void"
    elif is_const_void_ptr(arg_type):
        return pre + "?*const c_void"
    elif is_string_ptr(arg_type):
        return pre + "[]const u8"
    elif is_const_struct_ptr(arg_type):
        # not a bug, pass const structs by value
        return pre + f"{as_title_case(extract_ptr_type(arg_type))}"
    else:
        return arg_prefix + "???"

# get C-style arguments of a function pointer as string
def funcptr_args_c(field_type):
    tokens = field_type[field_type.index('(*)')+4:-1].split(',')
    s = ""
    for token in tokens:
        arg_type = token.strip();
        if s != "":
            s += ", "
        c_arg = as_extern_c_arg_type(arg_type)
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
        return '?*const c_void'
    else:
        return '???'

def funcdecl_args_c(decl):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_type = param_decl['type']
        s += as_extern_c_arg_type(arg_type)
    return s

def funcdecl_args_zig(decl):
    s = ""
    for param_decl in decl['params']:
        if s != "":
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        s += f"{as_zig_arg_type(f'{arg_name}: ', arg_type)}"
    return s

def funcdecl_res_c(decl):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    return as_extern_c_arg_type(res_type)

def funcdecl_res_zig(decl):
    decl_type = decl['type']
    res_type = decl_type[:decl_type.index('(')].strip()
    zig_res_type = as_zig_arg_type(None, res_type)
    if zig_res_type == "":
        zig_res_type = "void"
    return zig_res_type

def gen_struct(decl, prefix, callconvc_funcptrs = True, use_raw_name=False, use_extern=True):
    struct_name = decl['name']
    zig_type = struct_name if use_raw_name else as_title_case(struct_name)
    l(f"pub const {zig_type} = {'extern ' if use_extern else ''}struct {{")
    #l(f"    pub fn init(options: anytype) {zig_type} {{ var item: {zig_type} = .{{ }}; init_with(&item, options); return item; }}")
    for field in decl['fields']:
        field_name = field['name']
        field_type = field['type']
        field_type = check_struct_field_type_override(struct_name, field_name, field_type)
        if is_prim_type(field_type):
            l(f"    {field_name}: {as_zig_prim_type(field_type)} = {type_default_value(field_type)},")
        elif is_struct_type(field_type):
            l(f"    {field_name}: {as_title_case(field_type)} = .{{ }},")
        elif is_enum_type(field_type):
            l(f"    {field_name}: {as_title_case(field_type)} = .{enum_default_item(field_type)},")
        elif is_string_ptr(field_type):
            l(f"    {field_name}: [*c]const u8 = null,")
        elif is_const_void_ptr(field_type):
            l(f"    {field_name}: ?*const c_void = null,")
        elif is_void_ptr(field_type):
            l(f"    {field_name}: ?*c_void = null,")
        elif is_prim_ptr(field_type):
            l(f"    {field_name}: ?[*]const {as_zig_prim_type(extract_ptr_type(field_type))} = null,")
        elif is_func_ptr(field_type):
            if callconvc_funcptrs:
                l(f"    {field_name}: ?fn({funcptr_args_c(field_type)}) callconv(.C) {funcptr_res_c(field_type)} = null,")
            else:
                l(f"    {field_name}: ?fn({funcptr_args_c(field_type)}) {funcptr_res_c(field_type)} = null,")
        elif is_1d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    zig_type = as_zig_prim_type(array_type)
                    def_val = type_default_value(array_type)
                else:
                    zig_type = as_title_case(array_type)
                    def_val = ".{}"
                t0 = f"[{array_nums[0]}]{zig_type}"
                t0_slice = f"[]const {zig_type}"
                t1 = f"[_]{zig_type}"
                l(f"    {field_name}: {t0} = {t1}{{{def_val}}} ** {array_nums[0]},")
            elif is_const_void_ptr(array_type):
                l(f"    {field_name}: [{array_nums[0]}]?*const c_void = [_]?*const c_void {{ null }} ** {array_nums[0]},")
            else:
                l(f"//    FIXME: ??? array {field_name}: {field_type} => {array_type} [{array_nums[0]}]")
        elif is_2d_array_type(field_type):
            array_type = extract_array_type(field_type)
            array_nums = extract_array_nums(field_type)
            if is_prim_type(array_type):
                l(f"// FIXME: 2D array with primitive type: {field_name}")
            elif is_struct_type(array_type):
                zig_type = as_title_case(array_type)
                t0 = f"[{array_nums[0]}][{array_nums[1]}]{zig_type}"
                l(f"    {field_name}: {t0} = [_][{array_nums[1]}]{zig_type}{{[_]{zig_type}{{ .{{ }} }}**{array_nums[1]}}}**{array_nums[0]},")
        else:
            l(f"// FIXME: {field_name}: {field_type};")
    l("};")

def gen_consts(decl, prefix):
    for item in decl['items']:
        l(f"pub const {as_snake_case(item['name'], prefix)} = {item['value']};")

def gen_enum(decl, prefix):
    l(f"pub const {as_title_case(decl['name'])} = extern enum(i32) {{")
    for item in decl['items']:
        item_name = as_enum_item_name(item['name'])
        if item_name != "FORCE_U32":
            if 'value' in item:
                l(f"    {item_name} = {item['value']},")
            else:
                l(f"    {item_name},")
    l("};")

def gen_func_c(decl, prefix):
    l(f"pub extern fn {decl['name']}({funcdecl_args_c(decl)}) {funcdecl_res_c(decl)};")

def gen_func_zig(decl, prefix):
    c_func_name = decl['name']
    zig_func_name = as_camel_case(decl['name'])
    zig_res_type = funcdecl_res_zig(decl)
    l(f"pub fn {zig_func_name}({funcdecl_args_zig(decl)}) {funcdecl_res_zig(decl)} {{")
    if zig_res_type != 'void':
        s = f"    return {c_func_name}("
    else:
        s = f"    {c_func_name}("
    for i, param_decl in enumerate(decl['params']):
        if i > 0:
            s += ", "
        arg_name = param_decl['name']
        arg_type = param_decl['type']
        if is_const_struct_ptr(arg_type):
            s += "&" + arg_name
        else:
            s += arg_name
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

def gen_module(inp):
    l('// machine generated, do not edit')
    l('')
    pre_parse(inp)
    prefix = inp['prefix']
    for decl in inp['decls']:
        kind = decl['kind']
        if kind == 'struct':
            gen_struct(decl, prefix)
        elif kind == 'consts':
            gen_consts(decl, prefix)
        elif kind == 'enum':
            gen_enum(decl, prefix)
        elif kind == 'func':
            gen_func_c(decl, prefix)
            gen_func_zig(decl, prefix)

def gen_zig(input_path, output_path):
    reset_globals()
    try:
        print(f">>> {input_path} => {output_path}")
        with open(input_path, 'r') as f_inp:
            inp = json.load(f_inp)
            gen_module(inp)
            with open(output_path, 'w', newline='\n') as f_outp:
                f_outp.write(out_lines)
    except EnvironmentError as err:
        print(f"{err}")

def main():
    if not os.path.isdir('zig/'):
        os.mkdir('zig')
    if not os.path.isdir('zig/sokol'):
        os.mkdir('zig/sokol')
    gen_zig('sokol_gfx.json', 'zig/sokol/gfx.zig')
    gen_zig('sokol_app.json', 'zig/sokol/app.zig')

if __name__ == '__main__':
    main()
