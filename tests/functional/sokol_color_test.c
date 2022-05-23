//------------------------------------------------------------------------------
//  sokol_color_test.c
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#define SOKOL_COLOR_IMPL
#include "sokol_color.h"
#include "utest.h"
#include <math.h>

#define T(b) EXPECT_TRUE(b)
#define TFLT(f0,f1,epsilon) {T(fabs((f0)-(f1))<=(epsilon));}

UTEST(sokol_color, make_color) {
    const sg_color c0 = sg_make_color_4b(255, 127, 0, 255);
    TFLT(c0.r, 1.0f, 0.01f);
    TFLT(c0.g, 0.5f, 0.01f);
    TFLT(c0.b, 0.0f, 0.01f);
    TFLT(c0.a, 1.0f, 0.01f);
    const sg_color c1 = sg_make_color_1i(SG_BLACK_RGBA32);
    TFLT(c1.r, 0.0f, 0.01f);
    TFLT(c1.g, 0.0f, 0.01f);
    TFLT(c1.b, 0.0f, 0.01f);
    TFLT(c1.a, 1.0f, 0.01f);
    const sg_color c2 = sg_make_color_1i(SG_GREEN_RGBA32);
    TFLT(c2.r, 0.0f, 0.01f);
    TFLT(c2.g, 1.0f, 0.01f);
    TFLT(c2.b, 0.0f, 0.01f);
    TFLT(c2.a, 1.0f, 0.01f);
    const sg_color c3 = sg_make_color_1i(SG_RED_RGBA32);
    TFLT(c3.r, 1.0f, 0.01f);
    TFLT(c3.g, 0.0f, 0.01f);
    TFLT(c3.b, 0.0f, 0.01f);
    TFLT(c3.a, 1.0f, 0.01f);
    const sg_color c4 = sg_make_color_1i(SG_BLUE_RGBA32);
    TFLT(c4.r, 0.0f, 0.01f);
    TFLT(c4.g, 0.0f, 0.01f);
    TFLT(c4.b, 1.0f, 0.01f);
    TFLT(c4.a, 1.0f, 0.01f);
}

UTEST(sokol_color, lerp) {
    const sg_color c0 = sg_color_lerp(&sg_red, &sg_green, 0.5f);
    TFLT(c0.r, 0.5f, 0.001f);
    TFLT(c0.g, 0.5f, 0.001f);
    TFLT(c0.b, 0.0f, 0.001f);
    TFLT(c0.a, 1.0f, 0.001f);
    const sg_color c1 = sg_color_lerp_precise(&sg_red, &sg_green, 0.5f);
    TFLT(c1.r, 0.5f, 0.001f);
    TFLT(c1.g, 0.5f, 0.001f);
    TFLT(c1.b, 0.0f, 0.001f);
    TFLT(c1.a, 1.0f, 0.001f);
}

UTEST(sokol_color, multiply) {
    const sg_color c0 = sg_color_multiply(&sg_red, 0.5f);
    TFLT(c0.r, 0.5f, 0.001f);
    TFLT(c0.g, 0.0f, 0.001f);
    TFLT(c0.b, 0.0f, 0.001f);
    TFLT(c0.a, 0.5f, 0.001f);
    const sg_color c1 = sg_color_multiply(&sg_green, 0.5f);
    TFLT(c1.r, 0.0f, 0.001f);
    TFLT(c1.g, 0.5f, 0.001f);
    TFLT(c1.b, 0.0f, 0.001f);
    TFLT(c1.a, 0.5f, 0.001f);
    const sg_color c2 = sg_color_multiply(&sg_blue, 0.5f);
    TFLT(c2.r, 0.0f, 0.001f);
    TFLT(c2.g, 0.0f, 0.001f);
    TFLT(c2.b, 0.5f, 0.001f);
    TFLT(c2.a, 0.5f, 0.001f);
}
