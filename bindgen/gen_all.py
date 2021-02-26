import os, gen_nim, gen_zig

tasks = [
    [ '../sokol_gfx.h',            'sg_',       [] ],
    [ '../sokol_app.h',            'sapp_',     [] ],
    [ '../sokol_time.h',           'stm_',      [] ],
    [ '../sokol_audio.h',          'saudio_',   [] ],
    [ '../util/sokol_gl.h',        'sgl_',      ['sg_'] ],
    [ '../util/sokol_debugtext.h', 'sdtx_',     ['sg_'] ],
    [ '../util/sokol_shape.h',     'sshape_',   ['sg_'] ],
]

# Nim
gen_nim.prepare()
for task in tasks:
    c_header_path = task[0]
    main_prefix = task[1]
    dep_prefixes = task[2]
    gen_nim.gen(c_header_path, main_prefix, dep_prefixes)

# Zig
gen_zig.prepare()
for task in tasks:
    c_header_path = task[0]
    main_prefix = task[1]
    dep_prefixes = task[2]
    gen_zig.gen(c_header_path, main_prefix, dep_prefixes)
