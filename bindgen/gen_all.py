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

# Jai
gen_jai.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_jai.gen(c_header_path, main_prefix, dep_prefixes)

# Odin
gen_odin.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_odin.gen(c_header_path, main_prefix, dep_prefixes)

# Nim
gen_nim.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_nim.gen(c_header_path, main_prefix, dep_prefixes)

# Zig
zig_tasks = [
    *tasks,
    [ '../sokol_fetch.h', 'sfetch_', [] ],
    [ '../util/sokol_imgui.h', 'simgui_',   ['sg_', 'sapp_'] ],
]
gen_zig.prepare()
for task in zig_tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_zig.gen(c_header_path, main_prefix, dep_prefixes, {"tiger-style": args.zig_tiger_style})

# D
d_tasks = [
    *tasks,
    [ '../sokol_args.h',  'sargs_',  [] ],
    [ '../sokol_fetch.h', 'sfetch_', [] ],
    [ '../util/sokol_memtrack.h', 'smemtrack_', [] ],
    [ '../util/sokol_imgui.h', 'simgui_',   ['sg_', 'sapp_'] ],
    [ '../util/sokol_gfx_imgui.h', 'sgimgui_',   ['sg_', 'sapp_'] ],
]
# check if nuklear.h is available and copy it
if os.path.exists('../tests/ext/nuklear.h'):
    d_tasks.append([ '../util/sokol_nuklear.h', 'snk_',   ['sg_', 'sapp_'] ])
    if os.path.exists('sokol-d'):
        shutil.copy('../tests/ext/nuklear.h', 'sokol-d/src/sokol/c/nuklear.h')
gen_d.prepare()
for task in d_tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_d.gen(c_header_path, main_prefix, dep_prefixes)
# drop nuklear.h if copied (after generated D files)
if os.path.exists('sokol-d/src/sokol/c/nuklear.h'):
    os.remove('sokol-d/src/sokol/c/nuklear.h')

# Rust
gen_rust.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_rust.gen(c_header_path, main_prefix, dep_prefixes)

# C3
gen_c3.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_c3.gen(c_header_path, main_prefix, dep_prefixes)