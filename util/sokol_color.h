#if defined(SOKOL_IMPL) && !defined(SOKOL_COLOR_IMPL)
    #define SOKOL_COLOR_IMPL
#endif

#ifndef SOKOL_COLOR_INCLUDED
    #define SOKOL_COLOR_INCLUDED (1)
    #if !defined(SOKOL_GFX_INCLUDED)
        #error "Please include sokol_gfx.h before sokol_color.h"
    #endif
#endif /* SOKOL_COLOR_INCLUDED */

extern const sg_color sg_alice_blue;
extern const sg_color sg_antique_white;
extern const sg_color sg_aqua;
extern const sg_color sg_aquamarine;
extern const sg_color sg_azure;
extern const sg_color sg_beige;
extern const sg_color sg_bisque;
extern const sg_color sg_black;
extern const sg_color sg_blanched_almond;
extern const sg_color sg_blue;
extern const sg_color sg_blue_violet;
extern const sg_color sg_brown;
extern const sg_color sg_burly_wood;
extern const sg_color sg_cadet_blue;
extern const sg_color sg_chartreuse;
extern const sg_color sg_chocolate;
extern const sg_color sg_coral;
extern const sg_color sg_cornflower_blue;
extern const sg_color sg_cornsilk;
extern const sg_color sg_crimson;
extern const sg_color sg_cyan;
extern const sg_color sg_dark_blue;
extern const sg_color sg_dark_cyan;
extern const sg_color sg_dark_goldenrod;
extern const sg_color sg_dark_gray;
extern const sg_color sg_dark_green;
extern const sg_color sg_dark_khaki;
extern const sg_color sg_dark_magenta;
extern const sg_color sg_dark_olive_green;
extern const sg_color sg_dark_orange;
extern const sg_color sg_dark_orchid;
extern const sg_color sg_dark_red;
extern const sg_color sg_dark_salmon;
extern const sg_color sg_dark_sea_green;
extern const sg_color sg_dark_slate_blue;
extern const sg_color sg_dark_slate_gray;
extern const sg_color sg_dark_turquoise;
extern const sg_color sg_dark_violet;
extern const sg_color sg_deep_pink;
extern const sg_color sg_deep_sky_blue;
extern const sg_color sg_dim_gray;
extern const sg_color sg_dodger_blue;
extern const sg_color sg_fire_brick;
extern const sg_color sg_floral_white;
extern const sg_color sg_forest_green;
extern const sg_color sg_fuchsia;
extern const sg_color sg_gainsboro;
extern const sg_color sg_ghost_white;
extern const sg_color sg_gold;
extern const sg_color sg_goldenrod;
extern const sg_color sg_gray;
extern const sg_color sg_green;
extern const sg_color sg_green_yellow;
extern const sg_color sg_honeydew;
extern const sg_color sg_hot_pink;
extern const sg_color sg_indian_red;
extern const sg_color sg_indigo;
extern const sg_color sg_ivory;
extern const sg_color sg_khaki;
extern const sg_color sg_lavender;
extern const sg_color sg_lavender_blush;
extern const sg_color sg_lawn_green;
extern const sg_color sg_lemon_chiffon;
extern const sg_color sg_light_blue;
extern const sg_color sg_light_coral;
extern const sg_color sg_light_cyan;
extern const sg_color sg_light_goldenrod_yellow;
extern const sg_color sg_light_green;
extern const sg_color sg_light_grey;
extern const sg_color sg_light_pink;
extern const sg_color sg_light_salmon;
extern const sg_color sg_light_sea_green;
extern const sg_color sg_light_sky_blue;
extern const sg_color sg_light_slate_gray;
extern const sg_color sg_light_steel_blue;
extern const sg_color sg_light_yellow;
extern const sg_color sg_lime;
extern const sg_color sg_lime_green;
extern const sg_color sg_linen;
extern const sg_color sg_magenta;
extern const sg_color sg_maroon;
extern const sg_color sg_medium_aquamarine;
extern const sg_color sg_medium_blue;
extern const sg_color sg_medium_orchid;
extern const sg_color sg_medium_purple;
extern const sg_color sg_medium_sea_green;
extern const sg_color sg_medium_slate_blue;
extern const sg_color sg_medium_spring_green;
extern const sg_color sg_medium_turquoise;
extern const sg_color sg_medium_violet_red;
extern const sg_color sg_midnight_blue;
extern const sg_color sg_mint_cream;
extern const sg_color sg_misty_rose;
extern const sg_color sg_moccasin;
extern const sg_color sg_navajo_white;
extern const sg_color sg_navy;
extern const sg_color sg_old_lace;
extern const sg_color sg_olive;
extern const sg_color sg_olive_drab;
extern const sg_color sg_orange;
extern const sg_color sg_orange_red;
extern const sg_color sg_orchid;
extern const sg_color sg_pale_goldenrod;
extern const sg_color sg_pale_green;
extern const sg_color sg_pale_turquoise;
extern const sg_color sg_pale_violet_red;
extern const sg_color sg_papaya_whip;
extern const sg_color sg_peach_puff;
extern const sg_color sg_peru;
extern const sg_color sg_pink;
extern const sg_color sg_plum;
extern const sg_color sg_powder_blue;
extern const sg_color sg_purple;
extern const sg_color sg_red;
extern const sg_color sg_rosy_brown;
extern const sg_color sg_royal_blue;
extern const sg_color sg_saddle_brown;
extern const sg_color sg_salmon;
extern const sg_color sg_sandy_brown;
extern const sg_color sg_sea_green;
extern const sg_color sg_seashell;
extern const sg_color sg_sienna;
extern const sg_color sg_silver;
extern const sg_color sg_sky_blue;
extern const sg_color sg_slate_blue;
extern const sg_color sg_slate_gray;
extern const sg_color sg_snow;
extern const sg_color sg_spring_green;
extern const sg_color sg_steel_blue;
extern const sg_color sg_tan;
extern const sg_color sg_teal;
extern const sg_color sg_thistle;
extern const sg_color sg_tomato;
extern const sg_color sg_turquoise;
extern const sg_color sg_violet;
extern const sg_color sg_wheat;
extern const sg_color sg_white;
extern const sg_color sg_white_smoke;
extern const sg_color sg_yellow;
extern const sg_color sg_yellow_green;

#ifdef SOKOL_COLOR_IMPL
    #define SOKOL_COLOR_IMPL_INCLUDED (1)

#define SOKOL_DEFINE_COLOR(value) { ((uint8_t)(value >> 24)) / 255.0f, ((uint8_t)(value >> 16)) / 255.0f, ((uint8_t)(value >> 8)) / 255.0f, ((uint8_t)value / 255.0f) }

const sg_color sg_cornflower_blue = SOKOL_DEFINE_COLOR(0x6495EDFF);

#endif /* SOKOL_COLOR_IMPL */
