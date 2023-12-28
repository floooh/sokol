# -------------------------------------------------------------------------------
#   Generate rust bindings.
#
#   rust coding style:
#   - types are PascalCase
#   - otherwise snake_case
# -------------------------------------------------------------------------------
import gen_ir
import os, shutil, sys

import gen_util as util

module_names = {
    "slog_": "log",
    "sg_": "gfx",
    "sapp_": "app",
    "stm_": "time",
    "saudio_": "audio",
    "sgl_": "gl",
    "sdtx_": "debugtext",
    "sshape_": "shape",
    "sapp_sg": "glue",
    "simgui_": "imgui",
    "sg_imgui_": "gfx_imgui",
}

module_requires_rust_feature = {
    module_names["simgui_"]: "imgui",
    module_names["sg_imgui_"]: "imgui",
}

c_source_paths = {
    "slog_": "sokol-rust/src/sokol/c/sokol_log.c",
    "sg_": "sokol-rust/src/sokol/c/sokol_gfx.c",
    "sapp_": "sokol-rust/src/sokol/c/sokol_app.c",
    "stm_": "sokol-rust/src/sokol/c/sokol_time.c",
    "saudio_": "sokol-rust/src/sokol/c/sokol_audio.c",
    "sgl_": "sokol-rust/src/sokol/c/sokol_gl.c",
    "sdtx_": "sokol-rust/src/sokol/c/sokol_debugtext.c",
    "sshape_": "sokol-rust/src/sokol/c/sokol_shape.c",
    "sapp_sg": "sokol-rust/src/sokol/c/sokol_glue.c",
    "simgui_": "sokol-rust/src/sokol/c/sokol_imgui.c",
    "sg_imgui_": "sokol-rust/src/sokol/c/sokol_gfx_imgui.c",
}

ignores = [
    "sdtx_printf",
    "sdtx_vprintf",
    # "sg_install_trace_hooks",
    # "sg_trace_hooks",
]

range_struct_name = "Range"

# functions that need to be exposed as 'raw' C callbacks without a rust wrapper function
c_callbacks = ["slog_func"]

# NOTE: syntax for function results: "func_name.RESULT"
overrides = {
    "type": "_type",
    "ref": "_ref",

    "sg_apply_uniforms.ub_index": "uintptr_t",
    "sg_draw.base_element": "uintptr_t",
    "sg_draw.num_elements": "uintptr_t",
    "sg_draw.num_instances": "uintptr_t",
    "sshape_element_range_t.base_element": "uintptr_t",
    "sshape_element_range_t.num_elements": "uintptr_t",
    "sdtx_font.font_index": "uintptr_t",

    "sdtx_move": "sdtx_move_cursor",
    "sdtx_move_x": "sdtx_move_cursor_x",
    "sdtx_move_y": "sdtx_move_cursor_y",

    "sg_image_type::SG_IMAGETYPE_2D": "SG_IMAGEYPE_DIM2",
    "sg_image_type::SG_IMAGETYPE_3D": "SG_IMAGETYPE_DIM3",

    "sapp_keycode::SAPP_KEYCODE_0": "SAPP_KEYCODE_NUM0",
    "sapp_keycode::SAPP_KEYCODE_1": "SAPP_KEYCODE_NUM1",
    "sapp_keycode::SAPP_KEYCODE_2": "SAPP_KEYCODE_NUM2",
    "sapp_keycode::SAPP_KEYCODE_3": "SAPP_KEYCODE_NUM3",
    "sapp_keycode::SAPP_KEYCODE_4": "SAPP_KEYCODE_NUM4",
    "sapp_keycode::SAPP_KEYCODE_5": "SAPP_KEYCODE_NUM5",
    "sapp_keycode::SAPP_KEYCODE_6": "SAPP_KEYCODE_NUM6",
    "sapp_keycode::SAPP_KEYCODE_7": "SAPP_KEYCODE_NUM7",
    "sapp_keycode::SAPP_KEYCODE_8": "SAPP_KEYCODE_NUM8",
    "sapp_keycode::SAPP_KEYCODE_9": "SAPP_KEYCODE_NUM9",

    # "sgl_error": "sgl_get_error",  # 'error' is reserved in zig
    # "sgl_deg": "sgl_as_degrees",
    # "sgl_rad": "sgl_as_radians",
    # "sg_context_desc.color_format": "int",
    # "SGL_NO_ERROR": "SGL_ERROR_NO_ERROR",
    # "sg_context_desc.depth_format": "int",
}

prim_types = {
    "int": "i32",
    "bool": "bool",
    "char": "core::ffi::c_char",
    "int8_t": "i8",
    "uint8_t": "u8",
    "int16_t": "i16",
    "uint16_t": "u16",
    "int32_t": "i32",
    "uint32_t": "u32",
    "int64_t": "i64",
    "uint64_t": "u64",
    "float": "f32",
    "double": "f64",
    "uintptr_t": "usize",
    "intptr_t": "isize",
    "size_t": "usize",
}

prim_defaults = {
    "int": "0",
    "bool": "false",
    "int8_t": "0",
    "uint8_t": "0",
    "int16_t": "0",
    "uint16_t": "0",
    "int32_t": "0",
    "uint32_t": "0",
    "int64_t": "0",
    "uint64_t": "0",
    "float": "0.0",
    "double": "0.0",
    "uintptr_t": "0",
    "intptr_t": "0",
    "size_t": "0",
    "char": "0",
}

special_constant_types = {
    "SG_INVALID_ID": "u32",
    "SAPP_MODIFIER_SHIFT": "u32",
    "SAPP_MODIFIER_CTRL": "u32",
    "SAPP_MODIFIER_ALT": "u32",
    "SAPP_MODIFIER_SUPER": "u32",
    "SAPP_MODIFIER_LMB": "u32",
    "SAPP_MODIFIER_RMB": "u32",
    "SAPP_MODIFIER_MMB": "u32",
}

struct_types = []
enum_types = []
enum_items = {}
out_lines = ""


def reset_globals():
    global struct_types
    global enum_types
    global enum_items
    global out_lines
    struct_types = []
    enum_types = []
    enum_items = {}
    out_lines = ""


def l(s):
    global out_lines
    out_lines += s + "\n"


def as_rust_prim_type(s):
    return prim_types[s]


def as_upper_snake_case(s, prefix):
    outp = s.lower()
    if outp.startswith(prefix):
        outp = outp[len(prefix):]
    return outp.upper()


# prefix_bla_blub(_t) => (dep::)BlaBlub
def as_rust_struct_type(s, prefix):
    parts = s.lower().split("_")
    outp = "" if s.startswith(prefix) else f"{parts[0]}::"
    for part in parts[1:]:
        # ignore '_t' type postfix
        if part != "t":
            outp += part.capitalize()
    return outp


# prefix_bla_blub(_t) => (dep::)BlaBlub
def as_rust_enum_type(s, prefix):
    parts = s.lower().split("_")
    outp = "" if s.startswith(prefix) else f"{parts[0]}::"
    for part in parts[1:]:
        # ignore '_t' type postfix
        if part != "t":
            outp += part.capitalize()
    return outp


def check_override(name, default=None):
    if name in overrides:
        return overrides[name]
    elif default is None:
        return name
    else:
        return default


def check_ignore(name):
    return name in ignores


# PREFIX_ENUM_BLA_BLA => BlaBla, _PREFIX_ENUM_BLA_BLA => BlaBla
def as_enum_item_name(s):
    parts = s.lstrip("_").split("_")
    outp = ""
    for i, part in enumerate(parts[2:]):
        # TODO: What to do with enum fields starting with numbers?
        outp += part.capitalize()

    return outp


def enum_default_item(enum_name):
    return enum_items[enum_name][0]


def is_prim_type(s):
    return s in prim_types


def is_struct_type(s):
    return s in struct_types


def is_enum_type(s):
    return s in enum_types


def is_const_prim_ptr(s):
    for prim_type in prim_types:
        if s == f"const {prim_type} *":
            return True
    return False


def is_prim_ptr(s):
    for prim_type in prim_types:
        if s == f"{prim_type} *":
            return True
    return False


def is_const_struct_ptr(s):
    for struct_type in struct_types:
        if s == f"const {struct_type} *":
            return True
    return False


def is_struct_ptr(s):
    for struct_type in struct_types:
        if s == f"{struct_type} *":
            return True
    return False


def type_default_value(s):
    return prim_defaults[s]


def as_c_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix

    if arg_type == "void":
        return ""
    elif is_prim_type(arg_type):
        return pre + as_rust_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_rust_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_rust_enum_type(arg_type, prefix)
    elif util.is_void_ptr(arg_type):
        return pre + "*mut core::ffi::c_void"
    elif util.is_const_void_ptr(arg_type):
        return pre + "*const core::ffi::c_void"
    elif util.is_string_ptr(arg_type):
        return pre + "*const core::ffi::c_char"
    elif is_const_struct_ptr(arg_type):
        return pre + f"*const {as_rust_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_struct_ptr(arg_type):
        return pre + f"*mut {as_rust_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"*mut {as_rust_prim_type(util.extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return pre + f"*const {as_rust_prim_type(util.extract_ptr_type(arg_type))}"
    else:
        sys.exit(f"ERROR as_c_arg_type(): {arg_type}")


def as_rust_arg_type(arg_prefix, arg_type, prefix):
    # NOTE: if arg_prefix is None, the result is used as return value
    pre = "" if arg_prefix is None else arg_prefix

    if arg_type == "void":
        return ""
    elif is_prim_type(arg_type):
        return pre + as_rust_prim_type(arg_type)
    elif is_struct_type(arg_type):
        return pre + as_rust_struct_type(arg_type, prefix)
    elif is_enum_type(arg_type):
        return pre + as_rust_enum_type(arg_type, prefix)
    elif util.is_void_ptr(arg_type):
        return pre + "*mut core::ffi::c_void"
    elif util.is_const_void_ptr(arg_type):
        return pre + "*const core::ffi::c_void"
    elif util.is_string_ptr(arg_type):
        return pre + "&str"
    elif is_const_struct_ptr(arg_type):
        return pre + f"&{as_rust_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_struct_ptr(arg_type):
        return pre + f"&mut {as_rust_struct_type(util.extract_ptr_type(arg_type), prefix)}"
    elif is_prim_ptr(arg_type):
        return pre + f"&mut {as_rust_prim_type(util.extract_ptr_type(arg_type))}"
    elif is_const_prim_ptr(arg_type):
        return pre + f"&{as_rust_prim_type(util.extract_ptr_type(arg_type))}"
    else:
        sys.exit(f"ERROR as_rust_arg_type(): {arg_type}")


def is_rust_string(rust_type):
    return rust_type == "&str" or rust_type == " -> &'static str"


# get C-style arguments of a function pointer as string
def funcptr_args_c(field_type, prefix):
    tokens = field_type[field_type.index("(*)") + 4: -1].split(",")
    s = ""
    for token in tokens:
        arg_type = token.strip()
        if s != "":
            s += ", "
        c_arg = as_c_arg_type(None, arg_type, prefix)
        if c_arg == "void":
            return ""
        else:
            s += c_arg
    return s


# get C-style result of a function pointer as string
def funcptr_result_c(field_type):
    res_type = field_type[: field_type.index("(*)")].strip()
    if res_type == "void":
        return ""
    elif is_prim_type(res_type):
        return f" -> {as_rust_prim_type(res_type)}"
    elif util.is_const_void_ptr(res_type):
        return " -> *const core::ffi::c_void"
    elif util.is_void_ptr(res_type):
        return " -> *mut core::ffi::c_void"
    else:
        sys.exit(f"ERROR funcptr_result_c(): {field_type}")


def funcdecl_args_c(decl, prefix):
    s = ""
    func_name = decl["name"]
    for param_decl in decl["params"]:
        if s != "":
            s += ", "
        param_name = param_decl["name"]
        param_type = check_override(
            f"{func_name}.{param_name}", default=param_decl["type"]
        )
        s += f"{as_c_arg_type(f'{param_name}: ', param_type, prefix)}"
    return s


def funcdecl_args_rust(decl, prefix):
    s = ""
    func_name = decl["name"]
    for param_decl in decl["params"]:
        if s != "":
            s += ", "
        param_name = param_decl["name"]
        param_type = check_override(
            f"{func_name}.{param_name}", default=param_decl["type"]
        )
        s += f"{as_rust_arg_type(f'{param_name}: ', param_type, prefix)}"
    return s


def funcdecl_result_c(decl, prefix):
    func_name = decl["name"]
    decl_type = decl["type"]
    result_type = check_override(
        f"{func_name}.RESULT", default=decl_type[: decl_type.index("(")].strip()
    )

    it = as_c_arg_type(None, result_type, prefix)
    if it == "()" or it == "":
        return ""
    else:
        return f" -> {it}"


def funcdecl_result_rust(decl, prefix):
    func_name = decl["name"]
    decl_type = decl["type"]
    result_type = check_override(
        f"{func_name}.RESULT", default=decl_type[: decl_type.index("(")].strip()
    )
    rust_res_type = as_rust_arg_type(None, result_type, prefix)

    if is_rust_string(rust_res_type):
        rust_res_type = "&'static str"

    if rust_res_type == "":
        return ""
    else:
        return f" -> {rust_res_type }"


def gen_struct(decl, prefix):
    struct_name = check_override(decl["name"])
    rust_type = as_rust_struct_type(struct_name, prefix)
    rust_struct_type = rust_type

    struct_lines = []
    default_lines = []

    for field in decl["fields"]:
        field_name = check_override(field["name"])
        field_type = check_override(
            f"{struct_name}.{field_name}", default=field["type"]
        )

        if is_prim_type(field_type):
            struct_lines.append(
                f"pub {field_name}: {as_rust_prim_type(field_type)}"
            )
            default_lines.append(
                f"{field_name}: {type_default_value(field_type)}"
            )
        elif is_struct_type(field_type):
            struct_lines.append(
                f"pub {field_name}: {as_rust_struct_type(field_type, prefix)}"
            )
            default_lines.append(
                f"{field_name}: {as_rust_struct_type(field_type, prefix)}::new()"
            )
        elif is_enum_type(field_type):
            struct_lines.append(
                f"pub {field_name}: {as_rust_enum_type(field_type, prefix)}"
            )
            default_lines.append(
                f"{field_name}: {as_rust_enum_type(field_type, prefix)}::new()"
            )
        elif util.is_string_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *const core::ffi::c_char"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null()"
            )
        elif util.is_const_void_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *const core::ffi::c_void"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null()"
            )
        elif util.is_void_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *mut core::ffi::c_void"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null_mut()"
            )
        elif is_const_prim_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *const {as_rust_prim_type(util.extract_ptr_type(field_type))}"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null()"
            )
        elif is_prim_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *mut {as_rust_prim_type(util.extract_ptr_type(field_type))}"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null_mut()"
            )
        elif is_const_struct_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *const {as_rust_struct_type(util.extract_ptr_type(field_type), prefix)}"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null()"
            )
        elif is_struct_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: *mut {as_rust_struct_type(util.extract_ptr_type(field_type), prefix)}"
            )
            default_lines.append(
                f"{field_name}: core::ptr::null_mut()"
            )
        elif util.is_func_ptr(field_type):
            struct_lines.append(
                f"pub {field_name}: Option<extern \"C\" fn({funcptr_args_c(field_type, prefix)}){funcptr_result_c(field_type)}>"
            )
            default_lines.append(
                f"{field_name}: None"
            )
        elif util.is_1d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)
            if is_prim_type(array_type) or is_struct_type(array_type):
                if is_prim_type(array_type):
                    rust_type = as_rust_prim_type(array_type)
                    def_val = type_default_value(array_type)
                elif is_struct_type(array_type) or is_enum_type(array_type):
                    rust_type = as_rust_struct_type(array_type, prefix)
                    def_val = f"{rust_type}::new()"
                else:
                    sys.exit(f"ERROR gen_struct is_1d_array_type: {array_type}")
                t0 = f"[{rust_type}; {array_sizes[0]}]"
                # t1 = f"&{rust_type}"
                struct_lines.append(
                    f"pub {field_name}: {t0}"
                )
                default_lines.append(
                    f"{field_name}: [{def_val}; {array_sizes[0]}]"
                )
            elif util.is_const_void_ptr(array_type):
                struct_lines.append(
                    f"pub {field_name}: [*const core::ffi::c_void; {array_sizes[0]}]"
                )
                default_lines.append(
                    f"{field_name}: [core::ptr::null(); {array_sizes[0]}]"
                )
            else:
                sys.exit(
                    f"ERROR gen_struct: array {field_name}: {field_type} => [{array_type}: {array_sizes[0]}]"
                )
        elif util.is_2d_array_type(field_type):
            array_type = util.extract_array_type(field_type)
            array_sizes = util.extract_array_sizes(field_type)

            if is_prim_type(array_type):
                rust_type = as_rust_prim_type(array_type)
                def_val = type_default_value(array_type)
            elif is_struct_type(array_type):
                rust_type = as_rust_struct_type(array_type, prefix)
                def_val = f"{rust_type}::new()"
            else:
                sys.exit(f"ERROR gen_struct is_2d_array_type: {array_type}")

            struct_lines.append(
                f"pub {field_name}: [[{rust_type}; {array_sizes[1]}]; {array_sizes[0]}]"
            )
            default_lines.append(
                f"{field_name}: [[{def_val}; {array_sizes[1]}]; {array_sizes[0]}]"
            )
        else:
            sys.exit(f"ERROR gen_struct: {field_name}: {field_type};")

    #
    # TODO: Is this the best way to have zero-initialization with support for constants?
    #       core::mem::zeroed() cleaner?
    #

    l("#[repr(C)]")
    l("#[derive(Copy, Clone, Debug)]")
    l(f"pub struct {rust_struct_type} {{")
    for line in struct_lines:
        l(f"    {line},")
    l("}")

    l(f"impl {rust_struct_type} {{")
    l("    pub const fn new() -> Self {")
    l("        Self {")
    for line in default_lines:
        l(f"            {line},")
    l("        }")
    l("    }")
    l("}")

    l(f"impl Default for {rust_struct_type} {{")
    l("    fn default() -> Self {")
    l("        Self::new()")
    l("    }")
    l("}")


def gen_consts(decl, prefix):
    for item in decl["items"]:
        #
        # TODO: What type should these constants have? Currently giving all `usize`
        #       unless specifically overriden by `special_constant_types`
        #

        item_name = check_override(item["name"])
        if item_name in special_constant_types:
            special_type = special_constant_types[item_name]
            l(f"pub const {as_upper_snake_case(item_name, prefix)}: {special_type} = {item['value']};")
        else:
            l(f"pub const {as_upper_snake_case(item_name, prefix)}: usize = {item['value']};")


def gen_enum(decl, prefix):
    enum_name = check_override(decl["name"])

    names = [
        as_enum_item_name(check_override(f"{decl['name']}::{item['name']}", item['name'])) for item in decl["items"]
    ]

    is_u32 = False
    for name in names:
        if name == "ForceU32":
            is_u32 = True
            break

    l("#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]")
    if is_u32:
        l("#[repr(u32)]")
    else:
        l("#[repr(i32)]")

    rust_enum_name = as_rust_enum_type(enum_name, prefix)

    l(f"pub enum {rust_enum_name} {{")
    for item_name, item in zip(names, decl["items"]):
        if item_name != "ForceU32":
            if "value" in item:
                l(f"    {item_name} = {item['value']},")
            else:
                l(f"    {item_name},")
    l("}")

    default_item = enum_default_item(enum_name)
    l(f"impl {rust_enum_name} {{")
    l("    pub const fn new() -> Self {")
    l(f"        Self::{default_item}")
    l("    }")
    l("}")

    l(f"impl Default for {rust_enum_name} {{")
    l("    fn default() -> Self {")
    l(f"        Self::{default_item}")
    l("    }")
    l("}")


def gen_func_c(decl, prefix):
    l("pub extern \"C\" {")
    l(f"    fn {decl['name']}({funcdecl_args_c(decl, prefix)}){funcdecl_result_c(decl, prefix)};")
    l("}")


def gen_c_funcs(funcs):
    l("pub mod ffi {")
    l("    #![allow(unused_imports)]")
    l("    use super::*;")
    l("    extern \"C\" {")
    for decl, prefix in funcs:
        l(f"        pub fn {decl['name']}({funcdecl_args_c(decl, prefix)}){funcdecl_result_c(decl, prefix)};")
    l("    }")
    l("}")


def gen_rust_funcs(funcs):
    for decl, prefix in funcs:
        gen_func_rust(decl, prefix)


def gen_func_rust(decl, prefix):
    c_func_name = decl["name"]
    rust_func_name = util.as_lower_snake_case(check_override(decl["name"]), prefix)
    rust_res_type = funcdecl_result_rust(decl, prefix)

    if c_func_name in c_callbacks:
        c_res_type = funcdecl_result_c(decl, prefix)
        l("#[inline]")
        l(f'pub extern "C" fn {c_func_name}({funcdecl_args_c(decl, prefix)}){c_res_type} {{')
        l("    unsafe {")
        s = f"        ffi::{c_func_name}("
        for i, param_decl in enumerate(decl["params"]):
            if i > 0:
                s += ", "
            arg_name = param_decl["name"]
            s += arg_name
        s += ")"
        l(s)
        l("    }")
        l("}")
    else:
        l("#[inline]")
        l(f"pub fn {rust_func_name}({funcdecl_args_rust(decl, prefix)}){rust_res_type} {{")
        for i, param_decl in enumerate(decl["params"]):
            arg_name = param_decl["name"]
            arg_type = param_decl["type"]
            if util.is_string_ptr(arg_type):
                l(f"        let tmp_{i} = std::ffi::CString::new({arg_name}).unwrap();")

        l("    unsafe {")
        if is_rust_string(rust_res_type):
            # special case: convert C string to rust string slice
            s = f"        c_char_ptr_to_rust_str(ffi::{c_func_name}("
        else:
            s = f"        ffi::{c_func_name}("

        for i, param_decl in enumerate(decl["params"]):
            if i > 0:
                s += ", "
            arg_name = param_decl["name"]
            arg_type = param_decl["type"]

            if util.is_string_ptr(arg_type):
                s += f"tmp_{i}.as_ptr()"
            else:
                s += arg_name

        if is_rust_string(rust_res_type):
            s += ")"
        s += ")"
        l(s)
        l("    }")
        l("}")


def pre_parse(inp):
    global struct_types
    global enum_types
    for decl in inp["decls"]:
        kind = decl["kind"]
        if kind == "struct":
            struct_types.append(decl["name"])
        elif kind == "enum":
            enum_name = decl["name"]
            enum_types.append(enum_name)
            enum_items[enum_name] = []
            for item in decl["items"]:
                enum_items[enum_name].append(as_enum_item_name(item["name"]))

def gen_imports(inp, dep_prefixes):
    for dep_prefix in dep_prefixes:
        dep_module_name = module_names[dep_prefix]
        # l(f'const {dep_prefix[:-1]} = @import("{dep_module_name}.rs");')
        l(f'use crate::{dep_module_name} as {dep_prefix[:-1]};')
    l("")


def gen_helpers(inp):
    l("/// Helper function to convert a C string to a rust string slice")
    l("#[inline]")
    l("fn c_char_ptr_to_rust_str(c_char_ptr: *const core::ffi::c_char) -> &'static str {")
    l("    let c_str = unsafe { core::ffi::CStr::from_ptr(c_char_ptr) };")
    l("    c_str.to_str().expect(\"c_char_ptr contained invalid Utf8 Data\")")
    l("}")
    l("")

    if inp['prefix'] in ['sg_', 'sdtx_', 'sshape_', 'sapp_']:
        l("/// Helper function to cast a rust slice into a sokol Range")
        l(f"pub fn slice_as_range<T>(data: &[T]) -> {range_struct_name} {{")
        l(f"    {range_struct_name} {{ size: std::mem::size_of_val(data), ptr: data.as_ptr() as *const _ }}")
        l("}")
        l("/// Helper function to cast a rust reference into a sokol Range")
        l(f"pub fn value_as_range<T>(value: &T) -> {range_struct_name} {{")
        l(f"    {range_struct_name} {{ size: std::mem::size_of::<T>(), ptr: value as *const T as *const _ }}")
        l("}")
        l("")
        l(f"impl<T> From<&[T]> for {range_struct_name} {{")
        l("    #[inline]")
        l("    fn from(data: &[T]) -> Self {")
        l("        slice_as_range(data)")
        l("    }")
        l("}")
        l(f"impl<T> From<&T> for {range_struct_name} {{")
        l("    #[inline]")
        l("    fn from(value: &T) -> Self {")
        l("        value_as_range(value)")
        l("    }")
        l("}")
        l("")

    # if inp["prefix"] == "sdtx_":
    #     l("/// std.fmt compatible Writer")
    #     l("pub const Writer = struct {")
    #     l("    pub const Error = error { };")
    #     l("    pub fn writeAll(self: Writer, bytes: []const u8) Error!void {")
    #     l("        _ = self;")
    #     l("        for (bytes) |byte| {")
    #     l("            putc(byte);")
    #     l("        }")
    #     l("    }")
    #     l("    pub fn writeByteNTimes(self: Writer, byte: u8, n: u64) Error!void {")
    #     l("        _ = self;")
    #     l("        var i: u64 = 0;")
    #     l("        while (i < n): (i += 1) {")
    #     l("            putc(byte);")
    #     l("        }")
    #     l("    }")
    #     l("};")
    #     l("// std.fmt-style formatted print")
    #     l("pub fn print(comptime fmt: anytype, args: anytype) void {")
    #     l("    var writer: Writer = .{};")
    #     l('    @import("std").fmt.format(writer, fmt, args) catch {};')
    #     l("}")
    #     l("")


def gen_module(inp, dep_prefixes):
    module = inp['module']
    if module in module_requires_rust_feature:
        feature = module_requires_rust_feature[module]
        l(f"//! To use this module, enable the feature \"{feature}\"")

    l("// machine generated, do not edit")
    l("")


    l("#![allow(dead_code)]")
    l("#![allow(unused_imports)]")
    l("")
    gen_imports(inp, dep_prefixes)
    gen_helpers(inp)
    pre_parse(inp)
    prefix = inp["prefix"]

    funcs = []

    for decl in inp["decls"]:
        #
        #    HACK: gen_ir.py accidentally marks all sg_imgui_ declarations as is_dep since sg_imgui
        #          depends on sg_a but also starts with sg_... Fix gen_ir.py to remove this hack
        #
        dep_hack = False
        if module == "gfx_imgui":
            dep_hack = "name" in decl and decl["name"].startswith("sg_imgui_")

        if not decl["is_dep"] or dep_hack:
            kind = decl["kind"]
            if kind == "consts":
                gen_consts(decl, prefix)
            elif not check_ignore(decl["name"]):
                if kind == "struct":
                    gen_struct(decl, prefix)
                elif kind == "enum":
                    gen_enum(decl, prefix)
                elif kind == "func":
                    funcs.append((decl, prefix))

    gen_c_funcs(funcs)
    gen_rust_funcs(funcs)


def prepare():
    print("=== Generating Rust bindings:")
    if not os.path.isdir("sokol-rust/src/sokol"):
        os.makedirs("sokol-rust/src/sokol")
    if not os.path.isdir("sokol-rust/src/sokol/c"):
        os.makedirs("sokol-rust/src/sokol/c")

    with open("sokol-rust/src/lib.rs", "w", newline="\n") as f_outp:
        f_outp.write("//! Automatically generated sokol bindings for Rust\n\n")


def gen(c_header_path, c_prefix, dep_c_prefixes):
    if c_prefix not in module_names:
        print(f' >> warning: skipping generation for {c_prefix} prefix...')
        return

    module_name = module_names[c_prefix]
    c_source_path = c_source_paths[c_prefix]
    print(f'  {c_header_path} => {module_name}')
    reset_globals()
    c_path_in_project = f'sokol-rust/src/sokol/c/{os.path.basename(c_header_path)}'
    shutil.copyfile(c_header_path, c_path_in_project)
    ir = gen_ir.gen(c_header_path, c_source_path, module_name, c_prefix, dep_c_prefixes)
    gen_module(ir, dep_c_prefixes)
    output_path = f"sokol-rust/src/{ir['module']}.rs"
    with open(output_path, 'w', newline='\n') as f_outp:
        f_outp.write(out_lines)

    with open("sokol-rust/src/lib.rs", "a", newline="\n") as f_outp:
        module = ir['module']
        if module in module_requires_rust_feature:
            feature = module_requires_rust_feature[module]
            f_outp.write(f"/// Enable feature \"{feature}\" to use\n")
            f_outp.write(f"#[cfg(feature=\"{feature}\")]\n")
        f_outp.write(f"pub mod {module};\n")
