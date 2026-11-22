# LLM maintained.
import argparse, gen_nim, gen_zig, gen_odin, gen_rust, gen_d, gen_jai, gen_c3, gen_util

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
    [ '../sokol_args.h',             'sargs_',    [] ],
    [ '../util/sokol_memtrack.h',    'smemtrack_', [] ],
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

spine_tasks = [
    [ '../util/sokol_spine.h', 'sspine_', ['sg_'] ],
]

fontstash_tasks = [
    [ '../util/sokol_fontstash.h', 'sfons_', [] ],
]

opaque_types = {
    'sfons_': {'FONScontext *': 'FonsContext'},
}

# Nuklear needs bindings for its C types.
nuklear_task = [ '../util/sokol_nuklear.h', 'snk_', ['sg_', 'sapp_'] ]

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
    'sspine_':    'spine',
    'sfons_':     'fontstash',
}

# common to every binding
common_tasks = tasks + imgui_tasks + spine_tasks
tasks_with_fontstash = common_tasks + fontstash_tasks

def gen_tasks(generator, tasks, **options):
    for header_path, prefix, dep_prefixes in tasks:
        generator.gen({
            'c_header_path': header_path,
            'c_prefix': prefix,
            'dep_c_prefixes': dep_prefixes,
            'module_names': module_names,
            'opaque_types': opaque_types.get(prefix, {}),
            **options,
        })

# Jai keeps its handwritten Fontstash module.
gen_jai.prepare()
gen_tasks(gen_jai, common_tasks)

# Odin
gen_odin.prepare()
gen_tasks(gen_odin, tasks_with_fontstash)

# Nim
gen_nim.prepare()
gen_tasks(gen_nim, tasks_with_fontstash)

# Zig
gen_zig.prepare()
zig_options = {'tiger-style': args.zig_tiger_style}
gen_tasks(gen_zig, tasks_with_fontstash, **zig_options)
with gen_util.stage_nuklear(gen_zig.c_root):
    gen_tasks(gen_zig, [nuklear_task], **zig_options)
gen_zig.gen_color(args.zig_tiger_style)

# D
gen_d.prepare()
gen_tasks(gen_d, tasks_with_fontstash)
with gen_util.stage_nuklear(gen_d.c_root):
    gen_tasks(gen_d, [nuklear_task])

# Rust
gen_rust.prepare()
gen_tasks(gen_rust, tasks_with_fontstash)

# C3
gen_c3.prepare()
gen_tasks(gen_c3, tasks_with_fontstash)
