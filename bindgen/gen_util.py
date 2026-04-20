# common utility functions for all bindings generators
import re, shutil, os
import gen_ir as ir

re_1d_array = re.compile(r"^(?:const )?\w*\s*\*?\[\d*\]$")
re_2d_array = re.compile(r"^(?:const )?\w*\s*\*?\[\d*\]\[\d*\]$")

def is_1d_array_type(s):
    return re_1d_array.match(s) is not None

def is_2d_array_type(s):
    return re_2d_array.match(s) is not None

def is_array_type(s):
    return is_1d_array_type(s) or is_2d_array_type(s)

def extract_array_type(s):
    return s[:s.index('[')].strip()

def extract_array_sizes(s):
    return s[s.index('['):].replace('[', ' ').replace(']', ' ').split()

def is_string_ptr(s):
    return s == "const char *"

def is_const_void_ptr(s):
    return s == "const void *"

def is_void_ptr(s):
    return s == "void *"

def is_func_ptr(s):
    return '(*)' in s

def extract_ptr_type(s):
    tokens = s.split()
    if tokens[0] == 'const':
        return tokens[1]
    else:
        return tokens[0]

# PREFIX_BLA_BLUB to bla_blub
def as_lower_snake_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    return outp

# prefix_bla_blub => blaBlub, PREFIX_BLA_BLUB => blaBlub
def as_lower_camel_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    parts = outp.split('_')
    outp = parts[0]
    for part in parts[1:]:
        outp += part.capitalize()
    return outp

def prepare(language_name, rel_mod_root_dir, rel_c_root_dir):
    print(f'=== Generating {language_name} bindings:')
    if not os.path.isdir(rel_mod_root_dir):
        os.makedirs(rel_mod_root_dir, exist_ok = True)
    if not os.path.isdir(rel_c_root_dir):
        os.makedirs(rel_c_root_dir, exist_ok = True)
    shutil.copyfile('impl/sokol_defines.h', f'{rel_c_root_dir}/sokol_defines.h')

def gen_ir(opts, c_root, with_comments=False):
    c_src_header_path = opts['c_header_path']
    c_prefix = opts['c_prefix']
    dep_c_prefixes = opts['dep_c_prefixes']
    module_names = opts['module_names']
    if not c_prefix in module_names:
        print('>> warning: skipping generation for {c_prefix} prefix...')
        return False, {}
    c_src_source_path = f'impl/{os.path.splitext(os.path.basename(c_src_header_path))[0]}.c'
    c_dst_header_path = f'{c_root}/{os.path.basename(c_src_header_path)}'
    c_dst_source_path = f'{c_root}/{os.path.splitext(os.path.basename(c_src_header_path))[0]}.c'
    shutil.copyfile(c_src_header_path, c_dst_header_path)
    shutil.copyfile(c_src_source_path, c_dst_source_path)
    module_name = module_names[c_prefix]
    res = ir.gen(c_dst_header_path, c_dst_source_path, module_name, module_names, c_prefix, dep_c_prefixes, with_comments)
    return True, res
