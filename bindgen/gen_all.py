import os, gen_zig

tasks = [
    [ '../sokol_gfx.h',     'sg_',      'gfx', [] ],
    [ '../sokol_app.h',     'sapp_',    'app', [] ],
    [ '../sokol_time.h',    'stm_',     'time', [] ],
    [ '../sokol_audio.h',   'saudio_',  'audio', [] ],
    [ '../util/sokol_gl.h', 'sgl_',     'sgl', ['sg_'] ],
]

# Zig
print('> generating Zig bindings...')
gen_zig.prepare()
for task in tasks:
    c_header_path = task[0]
    main_prefix = task[1]
    module_name = task[2]
    dep_prefixes = task[3]
    print(f'  {c_header_path} => {module_name}.zig')
    gen_zig.gen(c_header_path, module_name, main_prefix, dep_prefixes)
