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

out_lines = ''

def reset_globals():
    global out_lines
    out_lines = ''

def l(s):
    global out_lines
    out_lines += s + '\n'

def get_odin_module_path(c_prefix):
    return f'{module_root}/{module_names[c_prefix]}'

def get_csource_path(c_prefix):
    return f'{module_root}/c/{c_source_names[c_prefix]}'

def make_odin_module_directory(c_prefix):
    path = get_odin_module_path(c_prefix)
    if not os.path.isdir(path):
        os.makedirs(path)

def gen_module(inp, dep_prefixes):
    l('// machine generated, do not edit')
    l('')

def prepare():
    print('Generating Odin bindings:')
    if not os.path.isdir(f'{module_root}/c'):
        os.makedirs(f'{module_root}/c')

def gen(c_header_path, c_prefix, dep_c_prefixes):
    if not c_prefix in module_names:
        print(f'warning: skipping generation for {c_prefix} prefix...')
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



