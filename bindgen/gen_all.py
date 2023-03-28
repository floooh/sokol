import os, gen_nim, gen_zig, gen_odin, gen_rust

tasks = [
    [ '../sokol_log.h',            'slog_',     [] ],
    [ '../sokol_gfx.h',            'sg_',       [] ],
    [ '../sokol_app.h',            'sapp_',     [] ],
    [ '../sokol_glue.h',           'sapp_sg',   ['sg_'] ],
    [ '../sokol_time.h',           'stm_',      [] ],
    [ '../sokol_audio.h',          'saudio_',   [] ],
    [ '../util/sokol_gl.h',        'sgl_',      ['sg_'] ],
    [ '../util/sokol_debugtext.h', 'sdtx_',     ['sg_'] ],
    [ '../util/sokol_shape.h',     'sshape_',   ['sg_'] ],
]

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
gen_zig.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_zig.gen(c_header_path, main_prefix, dep_prefixes)

# Rust
gen_rust.prepare()
for task in tasks:
    [c_header_path, main_prefix, dep_prefixes] = task
    gen_rust.gen(c_header_path, main_prefix, dep_prefixes)
