#-------------------------------------------------------------------------------
#   Generate the sokol_color.h header from a predefined palette
#-------------------------------------------------------------------------------
#   LICENSE
#   =======
#
#   zlib/libpng license
#
#   Copyright (c) 2020 Stuart Adams
#
#   This software is provided 'as-is', without any express or implied warranty.
#   In no event will the authors be held liable for any damages arising from the
#   use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
#   including commercial applications, and to alter it and redistribute it
#   freely, subject to the following restrictions:
#
#       1. The origin of this software must not be misrepresented; you must not
#       claim that you wrote the original software. If you use this software in a
#       product, an acknowledgment in the product documentation would be
#       appreciated but is not required.
#
#       2. Altered source versions must be plainly marked as such, and must not
#       be misrepresented as being the original software.
#
#       3. This notice may not be removed or altered from any source
#       distribution.

colors = [
("Alice Blue",          0xF0F8FFFF),
("Antique White",       0xFAEBD7FF),
("Aqua",                0x00FFFFFF),
("Aquamarine",          0x7FFFD4FF),
("Azure",               0xF0FFFFFF),
("Beige",               0xF5F5DCFF),
("Bisque",              0xFFE4C4FF),
("Black",               0x000000FF),
("Blanched Almond",     0xFFEBCDFF),
("Blue",                0x0000FFFF),
("Blue Violet",         0x8A2BE2FF),
("Brown",               0xA52A2AFF),
("Burlywood",           0xDEB887FF),
("Cadet Blue",          0x5F9EA0FF),
("Chartreuse",          0x7FFF00FF),
("Chocolate",           0xD2691EFF),
("Coral",               0xFF7F50FF),
("Cornflower Blue",     0x6495EDFF),
("Cornsilk",            0xFFF8DCFF),
("Crimson",             0xDC143CFF),
("Cyan",                0x00FFFFFF),
("Dark Blue",           0x00008BFF),
("Dark Cyan",           0x008B8BFF),
("Dark Goldenrod",      0xB8860BFF),
("Dark Gray",           0xA9A9A9FF),
("Dark Green",          0x006400FF),
("Dark Khaki",          0xBDB76BFF),
("Dark Magenta",        0x8B008BFF),
("Dark Olive Green",    0x556B2FFF),
("Dark Orange",         0xFF8C00FF),
("Dark Orchid",         0x9932CCFF),
("Dark Red",            0x8B0000FF),
("Dark Salmon",         0xE9967AFF),
("Dark Sea Green",      0x8FBC8FFF),
("Dark Slate Blue",     0x483D8BFF),
("Dark Slate Gray",     0x2F4F4FFF),
("Dark Turquoise",      0x00CED1FF),
("Dark Violet",         0x9400D3FF),
("Deep Pink",           0xFF1493FF),
("Deep Sky Blue",       0x00BFFFFF),
("Dim Gray",            0x696969FF),
("Dodger Blue",         0x1E90FFFF),
("Firebrick",           0xB22222FF),
("Floral White",        0xFFFAF0FF),
("Forest Green",        0x228B22FF),
("Fuchsia",             0xFF00FFFF),
("Gainsboro",           0xDCDCDCFF),
("Ghost White",         0xF8F8FFFF),
("Gold",                0xFFD700FF),
("Goldenrod",           0xDAA520FF),
("Gray",                0xBEBEBEFF),
("Web Gray",            0x808080FF),
("Green",               0x00FF00FF),
("Web Green",           0x008000FF),
("Green Yellow",        0xADFF2FFF),
("Honeydew",            0xF0FFF0FF),
("Hot Pink",            0xFF69B4FF),
("Indian Red",          0xCD5C5CFF),
("Indigo",              0x4B0082FF),
("Ivory",               0xFFFFF0FF),
("Khaki",               0xF0E68CFF),
("Lavender",            0xE6E6FAFF),
("Lavender Blush",      0xFFF0F5FF),
("Lawn Green",          0x7CFC00FF),
("Lemon Chiffon",       0xFFFACDFF),
("Light Blue",          0xADD8E6FF),
("Light Coral",         0xF08080FF),
("Light Cyan",          0xE0FFFFFF),
("Light Goldenrod",     0xFAFAD2FF),
("Light Gray",          0xD3D3D3FF),
("Light Green",         0x90EE90FF),
("Light Pink",          0xFFB6C1FF),
("Light Salmon",        0xFFA07AFF),
("Light Sea Green",     0x20B2AAFF),
("Light Sky Blue",      0x87CEFAFF),
("Light Slate Gray",    0x778899FF),
("Light Steel Blue",    0xB0C4DEFF),
("Light Yellow",        0xFFFFE0FF),
("Lime",                0x00FF00FF),
("Lime Green",          0x32CD32FF),
("Linen",               0xFAF0E6FF),
("Magenta",             0xFF00FFFF),
("Maroon",              0xB03060FF),
("Web Maroon",          0x800000FF),
("Medium Aquamarine",   0x66CDAAFF),
("Medium Blue",         0x0000CDFF),
("Medium Orchid",       0xBA55D3FF),
("Medium Purple",       0x9370DBFF),
("Medium Sea Green",    0x3CB371FF),
("Medium Slate Blue",   0x7B68EEFF),
("Medium Spring Green", 0x00FA9AFF),
("Medium Turquoise",    0x48D1CCFF),
("Medium Violet Red",   0xC71585FF),
("Midnight Blue",       0x191970FF),
("Mint Cream",          0xF5FFFAFF),
("Misty Rose",          0xFFE4E1FF),
("Moccasin",            0xFFE4B5FF),
("Navajo White",        0xFFDEADFF),
("Navy Blue",           0x000080FF),
("Old Lace",            0xFDF5E6FF),
("Olive",               0x808000FF),
("Olive Drab",          0x6B8E23FF),
("Orange",              0xFFA500FF),
("Orange Red",          0xFF4500FF),
("Orchid",              0xDA70D6FF),
("Pale Goldenrod",      0xEEE8AAFF),
("Pale Green",          0x98FB98FF),
("Pale Turquoise",      0xAFEEEEFF),
("Pale Violet Red",     0xDB7093FF),
("Papaya Whip",         0xFFEFD5FF),
("Peach Puff",          0xFFDAB9FF),
("Peru",                0xCD853FFF),
("Pink",                0xFFC0CBFF),
("Plum",                0xDDA0DDFF),
("Powder Blue",         0xB0E0E6FF),
("Purple",              0xA020F0FF),
("Web Purple",          0x800080FF),
("Rebecca Purple",      0x663399FF),
("Red",                 0xFF0000FF),
("Rosy Brown",          0xBC8F8FFF),
("Royal Blue",          0x4169E1FF),
("Saddle Brown",        0x8B4513FF),
("Salmon",              0xFA8072FF),
("Sandy Brown",         0xF4A460FF),
("Sea Green",           0x2E8B57FF),
("Seashell",            0xFFF5EEFF),
("Sienna",              0xA0522DFF),
("Silver",              0xC0C0C0FF),
("Sky Blue",            0x87CEEBFF),
("Slate Blue",          0x6A5ACDFF),
("Slate Gray",          0x708090FF),
("Snow",                0xFFFAFAFF),
("Spring Green",        0x00FF7FFF),
("Steel Blue",          0x4682B4FF),
("Tan",                 0xD2B48CFF),
("Teal",                0x008080FF),
("Thistle",             0xD8BFD8FF),
("Tomato",              0xFF6347FF),
("Transparent",         0x00000000),
("Turquoise",           0x40E0D0FF),
("Violet",              0xEE82EEFF),
("Wheat",               0xF5DEB3FF),
("White",               0xFFFFFFFF),
("White Smoke",         0xF5F5F5FF),
("Yellow",              0xFFFF00FF),
("Yellow Green",        0x9ACD32FF)
]

header = open("sokol_color.h", "w")

header.write("""
#ifndef SOKOL_COLOR_INCLUDED
/*
    sokol_color.h -- sg_color utilities

    Project URL: https://github.com/floooh/sokol

    Include the following headers before including sokol_color.h:

        sokol_gfx.h

    FEATURE OVERVIEW
    ================
    sokol_color.h defines preset colors based on the X11 color names.

    https://en.wikipedia.org/wiki/X11_color_names

    This is useful when you want to reference a familiar color, but
    don't want to write it out by hand. The palette should be familiar to
    web developers and those familiar with XNA / MonoGame.

    BASIC USAGE
    =============
    The palette is defined using static const (or constexpr if you are using a
    C++ compiler) objects. These objects use lowercase names:

        static SOKOL_COLOR_CONSTEXPR sg_color sg_red = SG_RED;
        static SOKOL_COLOR_CONSTEXPR sg_color sg_green = SG_GREEN;
        static SOKOL_COLOR_CONSTEXPR sg_color sg_blue = SG_BLUE;

    An sg_color preset object like sg_red can be used to initialize
    an sg_pass_action:

        sg_pass_action pass_action = {
            .colors[0] = { .action=SG_ACTION_CLEAR, .value = sg_red }
        };

    Initializing an object with static storage duration is more complicated
    because of C language rules. Technically, a static const is not a
    compile-time constant in C. To work around this, the palette is also
    defined as a series of brace-enclosed list macro definitions. These
    definitions use uppercase names:

        #define SG_RED { 1.0f, 0.0f, 0.0f, 1.0f }
        #define SG_GREEN { 0.0f, 1.0f, 0.0f, 1.0f }
        #define SG_BLUE { 0.0f, 0.0f, 1.0f, 1.0f }

    A preset macro like SG_RED can be used to initialize objects with static
    storage duration:

        static struct {
            sg_pass_action pass_action;
        } state = {
            .pass_action = {
                .colors[0] = { .action = SG_ACTION_CLEAR, .value = SG_RED }
            }
        };

    A second set of macro definitions exists for colors packed as 32 bit integer
    values. These definitions are also uppercase, but use the _RGBA32 suffix:

        #define SG_RED_RGBA32 0xFF0000FF
        #define SG_GREEN_RGBA32 0x00FF00FF
        #define SG_BLUE_RGBA32 0x0000FFFF

    This is useful if your code makes use of packed colors, as sokol_gl.h does for its
    internal vertex format:

        sgl_begin_triangles();
        sgl_v2f_c1i( 0.0f,  0.5f, SG_RED_RGBA32);
        sgl_v2f_c1i( 0.5f, -0.5f, SG_GREEN_RGBA32);
        sgl_v2f_c1i(-0.5f, -0.5f, SG_BLUE_RGBA32);
        sgl_end();

    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2020 Stuart Adams

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_COLOR_INCLUDED (1)

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_color.h"
#endif

#ifdef __cplusplus
#define SOKOL_COLOR_CONSTEXPR constexpr
#else
#define SOKOL_COLOR_CONSTEXPR const
#endif

""")

def unpack_rgba(color):
    red   = (color & 0xFF000000) >> 24
    green = (color & 0xFF0000) >> 16
    blue  = (color & 0xFF00) >> 8
    alpha = (color & 0xFF)
    return (red, green, blue, alpha)

def add_documentation(color):
    documentation = "/* {name} {{ R:{r}, G:{g}, B:{b}, A:{a} }} */\n"
    rgba = unpack_rgba(color[1])
    header.write(documentation.format(
        name = color[0], r = rgba[0], g = rgba[1], b = rgba[2], a = rgba[3]))

for color in colors:
    add_documentation(color)
    init_color = "SG_" + color[0].upper().replace(" ", "_")
    init_color_definition = "#define {name} {{ {r}f, {g}f, {b}f, {a}f }}\n"
    rgba = unpack_rgba(color[1])
    r = rgba[0] / 255
    g = rgba[1] / 255
    b = rgba[2] / 255
    a = rgba[3] / 255
    r_text = "{:.1f}".format(r) if r.is_integer() else "{:.9g}".format(r)
    g_text = "{:.1f}".format(g) if g.is_integer() else "{:.9g}".format(g)
    b_text = "{:.1f}".format(b) if b.is_integer() else "{:.9g}".format(b)
    a_text = "{:.1f}".format(a) if a.is_integer() else "{:.9g}".format(a)
    header.write(init_color_definition.format(
        name = init_color, r = r_text, g = g_text, b = b_text, a = a_text))

header.write("\n")

for color in colors:
    add_documentation(color)
    init_color = "sg_" + color[0].lower().replace(" ", "_")
    init_color_definition = "static SOKOL_COLOR_CONSTEXPR sg_color {name} = {init};\n"
    init_color_name = "SG_" + color[0].upper().replace(" ", "_")
    header.write(init_color_definition.format(name = init_color, init = init_color_name))

header.write("\n")

for color in colors:
    add_documentation(color)
    hex_color = "0x{0:08X}".format(color[1])
    packed_color = "SG_" + color[0].upper().replace(" ", "_") + "_RGBA32"
    packed_color_definition = "#define {name} {rgba}\n"
    header.write(packed_color_definition.format(name = packed_color, rgba = hex_color))

header.write("\n#endif /* SOKOL_COLOR_INCLUDED */\n")
