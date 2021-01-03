#-------------------------------------------------------------------------------
#   Generate an intermediate representation of a clang AST dump.
#-------------------------------------------------------------------------------
import json, sys, subprocess

def is_api_decl(decl, prefix):
    if 'name' in decl:
        return decl['name'].startswith(prefix)
    elif decl['kind'] == 'EnumDecl':
        # an anonymous enum, check if the items start with the prefix
        return decl['inner'][0]['name'].lower().startswith(prefix)
    else:
        return False

def filter_types(str):
    return str.replace('_Bool', 'bool')

def parse_struct(decl):
    outp = {}
    outp['kind'] = 'struct'
    outp['name'] = decl['name']
    outp['fields'] = []
    for item_decl in decl['inner']:
        if item_decl['kind'] != 'FieldDecl':
            sys.exit(f"ERROR: Structs must only contain simple fields ({decl['name']})")
        item = {}
        if 'name' in item_decl:
            item['name'] = item_decl['name']
        item['type'] = filter_types(item_decl['type']['qualType'])
        outp['fields'].append(item)
    return outp

def parse_enum(decl):
    outp = {}
    if 'name' in decl:
        outp['kind'] = 'enum'
        outp['name'] = decl['name']
        needs_value = False
    else:
        outp['kind'] = 'consts'
        needs_value = True
    outp['items'] = []
    for item_decl in decl['inner']:
        if item_decl['kind'] == 'EnumConstantDecl':
            item = {}
            item['name'] = item_decl['name']
            if 'inner' in item_decl:
                const_expr = item_decl['inner'][0]
                if const_expr['kind'] != 'ConstantExpr':
                    sys.exit(f"ERROR: Enum values must be a ConstantExpr ({decl['name']})")
                if const_expr['valueCategory'] != 'rvalue':
                    sys.exit(f"ERROR: Enum value ConstantExpr must be 'rvalue' ({decl['name']})")
                if not ((len(const_expr['inner']) == 1) and (const_expr['inner'][0]['kind'] == 'IntegerLiteral')):
                    sys.exit(f"ERROR: Enum value ConstantExpr must have exactly one IntegerLiteral ({decl['name']})")
                item['value'] = const_expr['inner'][0]['value']
            if needs_value and 'value' not in item:
                sys.exit(f"ERROR: anonymous enum items require an explicit value")
            outp['items'].append(item)
    return outp

def parse_func(decl):
    outp = {}
    outp['kind'] = 'func'
    outp['name'] = decl['name']
    outp['type'] = filter_types(decl['type']['qualType'])
    outp['params'] = []
    if 'inner' in decl:
        for param in decl['inner']:
            if param['kind'] != 'ParmVarDecl':
                sys.exit(f"ERROR: func param kind must be 'ParmVarDecl' ({decl['name']})")
            outp_param = {}
            outp_param['name'] = param['name']
            outp_param['type'] = filter_types(param['type']['qualType'])
            outp['params'].append(outp_param)
    return outp

def parse_decl(decl):
    kind = decl['kind']
    if kind == 'RecordDecl':
        return parse_struct(decl)
    elif kind == 'EnumDecl':
        return parse_enum(decl)
    elif kind == 'FunctionDecl':
        return parse_func(decl)
    else:
        return None

def clang(header_path, additional_options):
    cmd = ['clang', '-Xclang', '-ast-dump=json' ]
    cmd.extend(additional_options)
    cmd.append(header_path)
    return subprocess.check_output(cmd)

def gen(header_path, module, prefix, clang_options):
    ast = clang(header_path, clang_options)
    inp = json.loads(ast)
    outp = {}
    outp['module'] = module
    outp['prefix'] = prefix
    outp['decls'] = []
    for decl in inp['inner']:
        if is_api_decl(decl, prefix):
            outp_decl = parse_decl(decl)
            if outp_decl is not None:
                outp['decls'].append(outp_decl)
    return outp
