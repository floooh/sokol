import gen_ir, gen_zig

def gen_bindings(c_header_path, c_prefix, module_name):
    print(f'> {c_header_path}')
    ir = gen_ir.gen_ir(c_header_path, module_name, c_prefix)
    gen_zig.gen_zig(ir)

gen_bindings('../sokol_gfx.h', 'sg_', 'gfx')
gen_bindings('../sokol_app.h', 'sapp_', 'app')
