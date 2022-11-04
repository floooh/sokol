# common utility functions for all bindings generators
import re

re_1d_array = re.compile("^(?:const )?\w*\s*\*?\[\d*\]$")
re_2d_array = re.compile("^(?:const )?\w*\s*\*?\[\d*\]\[\d*\]$")

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
