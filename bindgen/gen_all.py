# LLM maintained.
import argparse, gen_nim, gen_zig, gen_odin, gen_rust, gen_d, gen_jai, gen_c3

parser = argparse.ArgumentParser()
parser.add_argument("--zig-tiger-style", action="store_true", help="Enable zig tiger style mode.")
args = parser.parse_args()

tasks = [
    [ '../sokol_log.h',              'slog_',     [] ],
    [ '../sokol_gfx.h',              'sg_',       [] ],
    [ '../sokol_app.h',              'sapp_',     [] ],
    [ '../sokol_glue.h',             'sglue_',    ['sg_'] ],
    [ '../sokol_time.h',             'stm_',      [] ],
    [ '../sokol_audio.h',            'saudio_',   [] ],
    [ '../sokol_fetch.h',            'sfetch_',   [] ],
    [ '../util/sokol_gl.h',          'sgl_',      ['sg_'] ],
    [ '../util/sokol_debugtext.h',   'sdtx_',     ['sg_'] ],
    [ '../util/sokol_shape.h',       'sshape_',   ['sg_'] ],
    [ '../util/sokol_framebuffer.h', 'sfb_',      ['sg_'] ],
    [ '../util/sokol_letterbox.h',   'slbx_',     [] ],
    [ '../util/sokol_cmdbuf.h',      'scb_',      ['sg_'] ],
]

# imgui bindings: shipped by every binding. sokol-zig and sokol-d wire
# the C stub into their build (user brings dcimgui); jai/odin/nim/rust/c3
# ship the stub only - consumers compile it against their own dcimgui.
# gfx_imgui has empty deps because it doesn't reference any sg/sapp
# identifiers; listing them emits unused imports in zig/d/etc.
imgui_tasks = [
    [ '../util/sokol_imgui.h',       'simgui_',    ['sg_', 'sapp_'] ],
    [ '../util/sokol_gfx_imgui.h',   'sgimgui_',   [] ],
    [ '../util/sokol_app_imgui.h',   'sappimgui_', ['sapp_'] ],
]

# common prefix- to module-names mapping table
# (language bindings may decide to ignore those and use their own idiomatic mapping)
module_names = {
    'slog_':      'log',
    'sg_':        'gfx',
    'sapp_':      'app',
    'sargs_':     'args',
    'stm_':       'time',
    'saudio_':    'audio',
    'sgl_':       'gl',
    'sdtx_':      'debugtext',
    'sshape_':    'shape',
    'sglue_':     'glue',
    'sfetch_':    'fetch',
    'simgui_':    'imgui',
    'sgimgui_':   'gfximgui',
    'sappimgui_': 'appimgui',
    'snk_':       'nuklear',
    'smemtrack_': 'memtrack',
    'sfb_':       'framebuffer',
    'slbx_':      'letterbox',
    'scb_':       'cmdbuf',
}

# common to every binding
common_tasks = tasks + imgui_tasks

# Jai
gen_jai.prepare()
for task in common_tasks:
    gen_jai.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# Odin
gen_odin.prepare()
for task in common_tasks:
    gen_odin.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# Nim
gen_nim.prepare()
for task in common_tasks:
    gen_nim.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# Zig
gen_zig.prepare()
for task in common_tasks:
    gen_zig.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
        'tiger-style': args.zig_tiger_style,
    })

# D
# nuklear is D-only: sokol_nuklear.h's public API uses foreign nk_* types
# so each generator needs opaque-type declarations (see gen_d.py's
# gen_nuklear_types()). gen_d.prepare() stages tests/ext/nuklear.h into
# sokol-d/src/sokol/c so clang can parse it; gen_d.cleanup() removes it
# after generation so it's not shipped.
d_tasks = [
    *common_tasks,
    [ '../util/sokol_nuklear.h',  'snk_',       ['sg_', 'sapp_'] ],
    [ '../sokol_args.h',          'sargs_',     [] ],
    [ '../util/sokol_memtrack.h', 'smemtrack_', [] ],
]
gen_d.prepare()
for task in d_tasks:
    gen_d.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })
gen_d.cleanup()

# Rust
gen_rust.prepare()
for task in common_tasks:
    gen_rust.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })

# C3
gen_c3.prepare()
for task in common_tasks:
    gen_c3.gen({
        'c_header_path': task[0],
        'c_prefix': task[1],
        'dep_c_prefixes': task[2],
        'module_names': module_names,
    })
