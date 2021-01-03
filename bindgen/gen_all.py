import gen_ir, gen_zig

tasks = [
    [ '../sokol_gfx.h', 'sg_', 'gfx' ],
    [ '../sokol_app.h', 'sapp_', 'app' ],
    [ '../sokol_time.h', 'stm_', 'time' ],
    [ '../sokol_audio.h', 'saudio_', 'audio' ]
]

# Zig
print('> generating Zig bindings...')
gen_zig.prepare()
for task in tasks:
    c_header_path = task[0]
    c_prefix = task[1]
    module_name = task[2]
    print(f'  {c_header_path} => {module_name}.zig')
    ir = gen_ir.gen(c_header_path, module_name, c_prefix, ["-DSOKOL_ZIG_BINDINGS"])
    gen_zig.gen(c_header_path, ir)
