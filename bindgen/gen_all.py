import os, argparse, gen_nim, gen_zig, gen_odin, gen_rust, gen_d, gen_jai, gen_c3, shutil

parser = argparse.ArgumentParser()
parser.add_argument("--zig-tiger-style", action="store_true", help="Enable zig tiger style mode.")
args = parser.parse_args()

tasks = [
    [ '../sokol_log.h',            'slog_',     [] ],
    [ '../sokol_gfx.h',            'sg_',       [] ],
    [ '../sokol_app.h',            'sapp_',     [] ],
    [ '../sokol_glue.h',           'sglue_',    ['sg_'] ],
    [ '../sokol_time.h',           'stm_',      [] ],
    [ '../sokol_audio.h',          'saudio_',   [] ],
    [ '../util/sokol_gl.h',        'sgl_',      ['sg_'] ],
    [ '../util/sokol_debugtext.h', 'sdtx_',     ['sg_'] ],
    [ '../util/sokol_shape.h',     'sshape_',   ['sg_'] ],
]

# common prefix- to module-names mapping table
# (language bindings may decide to ignore those and use their own idiomatic mapping)
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
    'sappimgui_': 'appimgui',
    'snk_':     'nuklear',
    'smemtrack_': 'memtrack',
}

# Jai
gen_jai.prepare()
for task in tasks:
    gen_jai.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# Odin
gen_odin.prepare()
for task in tasks:
    gen_odin.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# Nim
#gen_nim.prepare()
#for task in tasks:
#    gen_nim.gen({
#        c_header_path: task[0],
#        c_prefix: task[1],
#        dep_c_prefixes: task[2],
#        module_names: module_names,
#    })

# Zig
zig_tasks = [
    *tasks,
    [ '../sokol_fetch.h', 'sfetch_', [] ],
    [ '../util/sokol_imgui.h', 'simgui_',   ['sg_', 'sapp_'] ],
    [ '../util/sokol_gfx_imgui.h', 'sgimgui_', [] ],
    [ '../util/sokol_app_imgui.h', 'sappimgui_', ['sapp_'] ],
]
gen_zig.prepare()
for task in zig_tasks:
    gen_zig.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
        'tiger-style': args.zig_tiger_style,
    })

# D
#d_tasks = [
#    *tasks,
#    [ '../sokol_args.h',  'sargs_',  [] ],
#    [ '../sokol_fetch.h', 'sfetch_', [] ],
#    [ '../util/sokol_memtrack.h', 'smemtrack_', [] ],
#    [ '../util/sokol_imgui.h', 'simgui_',   ['sg_', 'sapp_'] ],
#    [ '../util/sokol_gfx_imgui.h', 'sgimgui_',   ['sg_', 'sapp_'] ],
#    [ '../util/sokol_app_imgui.h', 'sappimgui_', ['sapp_'] ],
#]
## check if nuklear.h is available and copy it
#if os.path.exists('../tests/ext/nuklear.h'):
#    d_tasks.append([ '../util/sokol_nuklear.h', 'snk_',   ['sg_', 'sapp_'] ])
#    if os.path.exists('sokol-d'):
#        shutil.copy('../tests/ext/nuklear.h', 'sokol-d/src/sokol/c/nuklear.h')
#gen_d.prepare()
#for task in d_tasks:
#    gen_d.gen({
#        c_header_path: task[0],
#        c_prefix: task[1],
#        dep_c_prefixes: task[2],
#        module_names: module_names,
#    })
## drop nuklear.h if copied (after generated D files)
#if os.path.exists('sokol-d/src/sokol/c/nuklear.h'):
#    os.remove('sokol-d/src/sokol/c/nuklear.h')

# Rust
#gen_rust.prepare()
#for task in tasks:
#    gen_rust.gen({
#        c_header_path: task[0],
#        c_prefix: task[1],
#        dep_c_prefixes: task[2],
#        module_names: module_names,
#    })

# C3
gen_c3.prepare()
for task in tasks:
    gen_c3.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })
