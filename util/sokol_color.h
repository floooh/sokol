
#ifndef SOKOL_COLOR_INCLUDED
/*
    sokol_color.h -- sg_color utilities
*/
#define SOKOL_COLOR_INCLUDED (1)

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_color.h"
#endif

/* Alice Blue { R:240, G:248, B:255, A:255 } */
#define SG_ALICE_BLUE_INIT { 0.941176471f, 0.97254902f, 1.0f, 1.0f }
/* Antique White { R:250, G:235, B:215, A:255 } */
#define SG_ANTIQUE_WHITE_INIT { 0.980392157f, 0.921568627f, 0.843137255f, 1.0f }
/* Aqua { R:0, G:255, B:255, A:255 } */
#define SG_AQUA_INIT { 0.0f, 1.0f, 1.0f, 1.0f }
/* Aquamarine { R:127, G:255, B:212, A:255 } */
#define SG_AQUAMARINE_INIT { 0.498039216f, 1.0f, 0.831372549f, 1.0f }
/* Azure { R:240, G:255, B:255, A:255 } */
#define SG_AZURE_INIT { 0.941176471f, 1.0f, 1.0f, 1.0f }
/* Beige { R:245, G:245, B:220, A:255 } */
#define SG_BEIGE_INIT { 0.960784314f, 0.960784314f, 0.862745098f, 1.0f }
/* Bisque { R:255, G:228, B:196, A:255 } */
#define SG_BISQUE_INIT { 1.0f, 0.894117647f, 0.768627451f, 1.0f }
/* Black { R:0, G:0, B:0, A:255 } */
#define SG_BLACK_INIT { 0.0f, 0.0f, 0.0f, 1.0f }
/* Blanched Almond { R:255, G:235, B:205, A:255 } */
#define SG_BLANCHED_ALMOND_INIT { 1.0f, 0.921568627f, 0.803921569f, 1.0f }
/* Blue { R:0, G:0, B:255, A:255 } */
#define SG_BLUE_INIT { 0.0f, 0.0f, 1.0f, 1.0f }
/* Blue Violet { R:138, G:43, B:226, A:255 } */
#define SG_BLUE_VIOLET_INIT { 0.541176471f, 0.168627451f, 0.88627451f, 1.0f }
/* Brown { R:165, G:42, B:42, A:255 } */
#define SG_BROWN_INIT { 0.647058824f, 0.164705882f, 0.164705882f, 1.0f }
/* Burly Wood { R:222, G:184, B:135, A:255 } */
#define SG_BURLY_WOOD_INIT { 0.870588235f, 0.721568627f, 0.529411765f, 1.0f }
/* Cadet Blue { R:95, G:158, B:160, A:255 } */
#define SG_CADET_BLUE_INIT { 0.37254902f, 0.619607843f, 0.62745098f, 1.0f }
/* Chartreuse { R:127, G:255, B:0, A:255 } */
#define SG_CHARTREUSE_INIT { 0.498039216f, 1.0f, 0.0f, 1.0f }
/* Chocolate { R:210, G:105, B:30, A:255 } */
#define SG_CHOCOLATE_INIT { 0.823529412f, 0.411764706f, 0.117647059f, 1.0f }
/* Coral { R:255, G:127, B:80, A:255 } */
#define SG_CORAL_INIT { 1.0f, 0.498039216f, 0.31372549f, 1.0f }
/* Cornflower Blue { R:100, G:149, B:237, A:255 } */
#define SG_CORNFLOWER_BLUE_INIT { 0.392156863f, 0.584313725f, 0.929411765f, 1.0f }
/* Cornsilk { R:255, G:248, B:220, A:255 } */
#define SG_CORNSILK_INIT { 1.0f, 0.97254902f, 0.862745098f, 1.0f }
/* Crimson { R:220, G:20, B:60, A:255 } */
#define SG_CRIMSON_INIT { 0.862745098f, 0.0784313725f, 0.235294118f, 1.0f }
/* Cyan { R:0, G:255, B:255, A:255 } */
#define SG_CYAN_INIT { 0.0f, 1.0f, 1.0f, 1.0f }
/* Dark Blue { R:0, G:0, B:139, A:255 } */
#define SG_DARK_BLUE_INIT { 0.0f, 0.0f, 0.545098039f, 1.0f }
/* Dark Cyan { R:0, G:139, B:139, A:255 } */
#define SG_DARK_CYAN_INIT { 0.0f, 0.545098039f, 0.545098039f, 1.0f }
/* Dark Goldenrod { R:184, G:134, B:11, A:255 } */
#define SG_DARK_GOLDENROD_INIT { 0.721568627f, 0.525490196f, 0.0431372549f, 1.0f }
/* Dark Gray { R:169, G:169, B:169, A:255 } */
#define SG_DARK_GRAY_INIT { 0.662745098f, 0.662745098f, 0.662745098f, 1.0f }
/* Dark Grey { R:169, G:169, B:169, A:255 } */
#define SG_DARK_GREY_INIT { 0.662745098f, 0.662745098f, 0.662745098f, 1.0f }
/* Dark Green { R:0, G:100, B:0, A:255 } */
#define SG_DARK_GREEN_INIT { 0.0f, 0.392156863f, 0.0f, 1.0f }
/* Dark Khaki { R:189, G:183, B:107, A:255 } */
#define SG_DARK_KHAKI_INIT { 0.741176471f, 0.717647059f, 0.419607843f, 1.0f }
/* Dark Magenta { R:139, G:0, B:139, A:255 } */
#define SG_DARK_MAGENTA_INIT { 0.545098039f, 0.0f, 0.545098039f, 1.0f }
/* Dark Olive Green { R:85, G:107, B:47, A:255 } */
#define SG_DARK_OLIVE_GREEN_INIT { 0.333333333f, 0.419607843f, 0.184313725f, 1.0f }
/* Dark Orange { R:255, G:140, B:0, A:255 } */
#define SG_DARK_ORANGE_INIT { 1.0f, 0.549019608f, 0.0f, 1.0f }
/* Dark Orchid { R:153, G:50, B:204, A:255 } */
#define SG_DARK_ORCHID_INIT { 0.6f, 0.196078431f, 0.8f, 1.0f }
/* Dark Red { R:139, G:0, B:0, A:255 } */
#define SG_DARK_RED_INIT { 0.545098039f, 0.0f, 0.0f, 1.0f }
/* Dark Salmon { R:233, G:150, B:122, A:255 } */
#define SG_DARK_SALMON_INIT { 0.91372549f, 0.588235294f, 0.478431373f, 1.0f }
/* Dark Sea Green { R:143, G:188, B:143, A:255 } */
#define SG_DARK_SEA_GREEN_INIT { 0.560784314f, 0.737254902f, 0.560784314f, 1.0f }
/* Dark Slate Blue { R:72, G:61, B:139, A:255 } */
#define SG_DARK_SLATE_BLUE_INIT { 0.282352941f, 0.239215686f, 0.545098039f, 1.0f }
/* Dark Slate Gray { R:47, G:79, B:79, A:255 } */
#define SG_DARK_SLATE_GRAY_INIT { 0.184313725f, 0.309803922f, 0.309803922f, 1.0f }
/* Dark Slate Grey { R:47, G:79, B:79, A:255 } */
#define SG_DARK_SLATE_GREY_INIT { 0.184313725f, 0.309803922f, 0.309803922f, 1.0f }
/* Dark Turquoise { R:0, G:206, B:209, A:255 } */
#define SG_DARK_TURQUOISE_INIT { 0.0f, 0.807843137f, 0.819607843f, 1.0f }
/* Dark Violet { R:148, G:0, B:211, A:255 } */
#define SG_DARK_VIOLET_INIT { 0.580392157f, 0.0f, 0.82745098f, 1.0f }
/* Deep Pink { R:255, G:20, B:147, A:255 } */
#define SG_DEEP_PINK_INIT { 1.0f, 0.0784313725f, 0.576470588f, 1.0f }
/* Deep Sky Blue { R:0, G:191, B:255, A:255 } */
#define SG_DEEP_SKY_BLUE_INIT { 0.0f, 0.749019608f, 1.0f, 1.0f }
/* Dim Gray { R:105, G:105, B:105, A:255 } */
#define SG_DIM_GRAY_INIT { 0.411764706f, 0.411764706f, 0.411764706f, 1.0f }
/* Dim Grey { R:105, G:105, B:105, A:255 } */
#define SG_DIM_GREY_INIT { 0.411764706f, 0.411764706f, 0.411764706f, 1.0f }
/* Dodger Blue { R:30, G:144, B:255, A:255 } */
#define SG_DODGER_BLUE_INIT { 0.117647059f, 0.564705882f, 1.0f, 1.0f }
/* Fire Brick { R:178, G:34, B:34, A:255 } */
#define SG_FIRE_BRICK_INIT { 0.698039216f, 0.133333333f, 0.133333333f, 1.0f }
/* Floral White { R:255, G:250, B:240, A:255 } */
#define SG_FLORAL_WHITE_INIT { 1.0f, 0.980392157f, 0.941176471f, 1.0f }
/* Forest Green { R:34, G:139, B:34, A:255 } */
#define SG_FOREST_GREEN_INIT { 0.133333333f, 0.545098039f, 0.133333333f, 1.0f }
/* Fuchsia { R:255, G:0, B:255, A:255 } */
#define SG_FUCHSIA_INIT { 1.0f, 0.0f, 1.0f, 1.0f }
/* Gainsboro { R:220, G:220, B:220, A:255 } */
#define SG_GAINSBORO_INIT { 0.862745098f, 0.862745098f, 0.862745098f, 1.0f }
/* Ghost White { R:248, G:248, B:255, A:255 } */
#define SG_GHOST_WHITE_INIT { 0.97254902f, 0.97254902f, 1.0f, 1.0f }
/* Gold { R:255, G:215, B:0, A:255 } */
#define SG_GOLD_INIT { 1.0f, 0.843137255f, 0.0f, 1.0f }
/* Goldenrod { R:218, G:165, B:32, A:255 } */
#define SG_GOLDENROD_INIT { 0.854901961f, 0.647058824f, 0.125490196f, 1.0f }
/* Gray { R:128, G:128, B:128, A:255 } */
#define SG_GRAY_INIT { 0.501960784f, 0.501960784f, 0.501960784f, 1.0f }
/* Grey { R:128, G:128, B:128, A:255 } */
#define SG_GREY_INIT { 0.501960784f, 0.501960784f, 0.501960784f, 1.0f }
/* Green { R:0, G:128, B:0, A:255 } */
#define SG_GREEN_INIT { 0.0f, 0.501960784f, 0.0f, 1.0f }
/* Green Yellow { R:173, G:255, B:47, A:255 } */
#define SG_GREEN_YELLOW_INIT { 0.678431373f, 1.0f, 0.184313725f, 1.0f }
/* Honeydew { R:240, G:255, B:240, A:255 } */
#define SG_HONEYDEW_INIT { 0.941176471f, 1.0f, 0.941176471f, 1.0f }
/* Hot Pink { R:255, G:105, B:180, A:255 } */
#define SG_HOT_PINK_INIT { 1.0f, 0.411764706f, 0.705882353f, 1.0f }
/* Indian Red { R:205, G:92, B:92, A:255 } */
#define SG_INDIAN_RED_INIT { 0.803921569f, 0.360784314f, 0.360784314f, 1.0f }
/* Indigo { R:75, G:0, B:130, A:255 } */
#define SG_INDIGO_INIT { 0.294117647f, 0.0f, 0.509803922f, 1.0f }
/* Ivory { R:255, G:255, B:240, A:255 } */
#define SG_IVORY_INIT { 1.0f, 1.0f, 0.941176471f, 1.0f }
/* Khaki { R:240, G:230, B:140, A:255 } */
#define SG_KHAKI_INIT { 0.941176471f, 0.901960784f, 0.549019608f, 1.0f }
/* Lavender { R:230, G:230, B:250, A:255 } */
#define SG_LAVENDER_INIT { 0.901960784f, 0.901960784f, 0.980392157f, 1.0f }
/* Lavender Blush { R:255, G:240, B:245, A:255 } */
#define SG_LAVENDER_BLUSH_INIT { 1.0f, 0.941176471f, 0.960784314f, 1.0f }
/* Lawn Green { R:124, G:252, B:0, A:255 } */
#define SG_LAWN_GREEN_INIT { 0.48627451f, 0.988235294f, 0.0f, 1.0f }
/* Lemon Chiffon { R:255, G:250, B:205, A:255 } */
#define SG_LEMON_CHIFFON_INIT { 1.0f, 0.980392157f, 0.803921569f, 1.0f }
/* Light Blue { R:173, G:216, B:230, A:255 } */
#define SG_LIGHT_BLUE_INIT { 0.678431373f, 0.847058824f, 0.901960784f, 1.0f }
/* Light Coral { R:240, G:128, B:128, A:255 } */
#define SG_LIGHT_CORAL_INIT { 0.941176471f, 0.501960784f, 0.501960784f, 1.0f }
/* Light Cyan { R:224, G:255, B:255, A:255 } */
#define SG_LIGHT_CYAN_INIT { 0.878431373f, 1.0f, 1.0f, 1.0f }
/* Light Goldenrod Yellow { R:250, G:250, B:210, A:255 } */
#define SG_LIGHT_GOLDENROD_YELLOW_INIT { 0.980392157f, 0.980392157f, 0.823529412f, 1.0f }
/* Light Green { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GREEN_INIT { 0.82745098f, 0.82745098f, 0.82745098f, 1.0f }
/* Light Gray { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GRAY_INIT { 0.82745098f, 0.82745098f, 0.82745098f, 1.0f }
/* Light Grey { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GREY_INIT { 0.82745098f, 0.82745098f, 0.82745098f, 1.0f }
/* Light Pink { R:255, G:182, B:193, A:255 } */
#define SG_LIGHT_PINK_INIT { 1.0f, 0.71372549f, 0.756862745f, 1.0f }
/* Light Salmon { R:255, G:160, B:122, A:255 } */
#define SG_LIGHT_SALMON_INIT { 1.0f, 0.62745098f, 0.478431373f, 1.0f }
/* Light Sea Green { R:32, G:178, B:170, A:255 } */
#define SG_LIGHT_SEA_GREEN_INIT { 0.125490196f, 0.698039216f, 0.666666667f, 1.0f }
/* Light Sky Blue { R:135, G:206, B:250, A:255 } */
#define SG_LIGHT_SKY_BLUE_INIT { 0.529411765f, 0.807843137f, 0.980392157f, 1.0f }
/* Light Slate Gray { R:119, G:136, B:153, A:255 } */
#define SG_LIGHT_SLATE_GRAY_INIT { 0.466666667f, 0.533333333f, 0.6f, 1.0f }
/* Light Slate Grey { R:119, G:136, B:153, A:255 } */
#define SG_LIGHT_SLATE_GREY_INIT { 0.466666667f, 0.533333333f, 0.6f, 1.0f }
/* Light Steel Blue { R:176, G:196, B:222, A:255 } */
#define SG_LIGHT_STEEL_BLUE_INIT { 0.690196078f, 0.768627451f, 0.870588235f, 1.0f }
/* Light Yellow { R:255, G:255, B:224, A:255 } */
#define SG_LIGHT_YELLOW_INIT { 1.0f, 1.0f, 0.878431373f, 1.0f }
/* Lime { R:0, G:255, B:0, A:255 } */
#define SG_LIME_INIT { 0.0f, 1.0f, 0.0f, 1.0f }
/* Lime Green { R:50, G:205, B:50, A:255 } */
#define SG_LIME_GREEN_INIT { 0.196078431f, 0.803921569f, 0.196078431f, 1.0f }
/* Linen { R:250, G:240, B:230, A:255 } */
#define SG_LINEN_INIT { 0.980392157f, 0.941176471f, 0.901960784f, 1.0f }
/* Magenta { R:255, G:0, B:255, A:255 } */
#define SG_MAGENTA_INIT { 1.0f, 0.0f, 1.0f, 1.0f }
/* Maroon { R:128, G:0, B:0, A:255 } */
#define SG_MAROON_INIT { 0.501960784f, 0.0f, 0.0f, 1.0f }
/* Medium Aquamarine { R:102, G:205, B:170, A:255 } */
#define SG_MEDIUM_AQUAMARINE_INIT { 0.4f, 0.803921569f, 0.666666667f, 1.0f }
/* Medium Blue { R:0, G:0, B:205, A:255 } */
#define SG_MEDIUM_BLUE_INIT { 0.0f, 0.0f, 0.803921569f, 1.0f }
/* Medium Orchid { R:186, G:85, B:211, A:255 } */
#define SG_MEDIUM_ORCHID_INIT { 0.729411765f, 0.333333333f, 0.82745098f, 1.0f }
/* Medium Purple { R:147, G:112, B:216, A:255 } */
#define SG_MEDIUM_PURPLE_INIT { 0.576470588f, 0.439215686f, 0.847058824f, 1.0f }
/* Medium Sea Green { R:60, G:179, B:113, A:255 } */
#define SG_MEDIUM_SEA_GREEN_INIT { 0.235294118f, 0.701960784f, 0.443137255f, 1.0f }
/* Medium Slate Blue { R:123, G:104, B:238, A:255 } */
#define SG_MEDIUM_SLATE_BLUE_INIT { 0.482352941f, 0.407843137f, 0.933333333f, 1.0f }
/* Medium Spring Green { R:0, G:250, B:154, A:255 } */
#define SG_MEDIUM_SPRING_GREEN_INIT { 0.0f, 0.980392157f, 0.603921569f, 1.0f }
/* Medium Turquoise { R:72, G:209, B:204, A:255 } */
#define SG_MEDIUM_TURQUOISE_INIT { 0.282352941f, 0.819607843f, 0.8f, 1.0f }
/* Medium Violet Red { R:199, G:21, B:133, A:255 } */
#define SG_MEDIUM_VIOLET_RED_INIT { 0.780392157f, 0.0823529412f, 0.521568627f, 1.0f }
/* Midnight Blue { R:25, G:25, B:112, A:255 } */
#define SG_MIDNIGHT_BLUE_INIT { 0.0980392157f, 0.0980392157f, 0.439215686f, 1.0f }
/* Mint Cream { R:245, G:255, B:250, A:255 } */
#define SG_MINT_CREAM_INIT { 0.960784314f, 1.0f, 0.980392157f, 1.0f }
/* Misty Rose { R:255, G:228, B:225, A:255 } */
#define SG_MISTY_ROSE_INIT { 1.0f, 0.894117647f, 0.882352941f, 1.0f }
/* Moccasin { R:255, G:228, B:181, A:255 } */
#define SG_MOCCASIN_INIT { 1.0f, 0.894117647f, 0.709803922f, 1.0f }
/* Navajo White { R:255, G:222, B:173, A:255 } */
#define SG_NAVAJO_WHITE_INIT { 1.0f, 0.870588235f, 0.678431373f, 1.0f }
/* Navy { R:0, G:0, B:128, A:255 } */
#define SG_NAVY_INIT { 0.0f, 0.0f, 0.501960784f, 1.0f }
/* Old Lace { R:253, G:245, B:230, A:255 } */
#define SG_OLD_LACE_INIT { 0.992156863f, 0.960784314f, 0.901960784f, 1.0f }
/* Olive { R:128, G:128, B:0, A:255 } */
#define SG_OLIVE_INIT { 0.501960784f, 0.501960784f, 0.0f, 1.0f }
/* Olive Drab { R:107, G:142, B:35, A:255 } */
#define SG_OLIVE_DRAB_INIT { 0.419607843f, 0.556862745f, 0.137254902f, 1.0f }
/* Orange { R:255, G:165, B:0, A:255 } */
#define SG_ORANGE_INIT { 1.0f, 0.647058824f, 0.0f, 1.0f }
/* Orange Red { R:255, G:69, B:0, A:255 } */
#define SG_ORANGE_RED_INIT { 1.0f, 0.270588235f, 0.0f, 1.0f }
/* Orchid { R:218, G:112, B:214, A:255 } */
#define SG_ORCHID_INIT { 0.854901961f, 0.439215686f, 0.839215686f, 1.0f }
/* Pale Goldenrod { R:238, G:232, B:170, A:255 } */
#define SG_PALE_GOLDENROD_INIT { 0.933333333f, 0.909803922f, 0.666666667f, 1.0f }
/* Pale Green { R:152, G:251, B:152, A:255 } */
#define SG_PALE_GREEN_INIT { 0.596078431f, 0.984313725f, 0.596078431f, 1.0f }
/* Pale Turquoise { R:175, G:238, B:238, A:255 } */
#define SG_PALE_TURQUOISE_INIT { 0.68627451f, 0.933333333f, 0.933333333f, 1.0f }
/* Pale Violet Red { R:216, G:112, B:147, A:255 } */
#define SG_PALE_VIOLET_RED_INIT { 0.847058824f, 0.439215686f, 0.576470588f, 1.0f }
/* Papaya Whip { R:255, G:239, B:213, A:255 } */
#define SG_PAPAYA_WHIP_INIT { 1.0f, 0.937254902f, 0.835294118f, 1.0f }
/* Peach Puff { R:255, G:218, B:185, A:255 } */
#define SG_PEACH_PUFF_INIT { 1.0f, 0.854901961f, 0.725490196f, 1.0f }
/* Peru { R:205, G:133, B:63, A:255 } */
#define SG_PERU_INIT { 0.803921569f, 0.521568627f, 0.247058824f, 1.0f }
/* Pink { R:255, G:192, B:203, A:255 } */
#define SG_PINK_INIT { 1.0f, 0.752941176f, 0.796078431f, 1.0f }
/* Plum { R:221, G:160, B:221, A:255 } */
#define SG_PLUM_INIT { 0.866666667f, 0.62745098f, 0.866666667f, 1.0f }
/* Powder Blue { R:176, G:224, B:230, A:255 } */
#define SG_POWDER_BLUE_INIT { 0.690196078f, 0.878431373f, 0.901960784f, 1.0f }
/* Purple { R:128, G:0, B:128, A:255 } */
#define SG_PURPLE_INIT { 0.501960784f, 0.0f, 0.501960784f, 1.0f }
/* Red { R:255, G:0, B:0, A:255 } */
#define SG_RED_INIT { 1.0f, 0.0f, 0.0f, 1.0f }
/* Rosy Brown { R:188, G:143, B:143, A:255 } */
#define SG_ROSY_BROWN_INIT { 0.737254902f, 0.560784314f, 0.560784314f, 1.0f }
/* Royal Blue { R:65, G:105, B:225, A:255 } */
#define SG_ROYAL_BLUE_INIT { 0.254901961f, 0.411764706f, 0.882352941f, 1.0f }
/* Saddle Brown { R:139, G:69, B:19, A:255 } */
#define SG_SADDLE_BROWN_INIT { 0.545098039f, 0.270588235f, 0.0745098039f, 1.0f }
/* Salmon { R:250, G:128, B:114, A:255 } */
#define SG_SALMON_INIT { 0.980392157f, 0.501960784f, 0.447058824f, 1.0f }
/* Sandy Brown { R:244, G:164, B:96, A:255 } */
#define SG_SANDY_BROWN_INIT { 0.956862745f, 0.643137255f, 0.376470588f, 1.0f }
/* Sea Green { R:46, G:139, B:87, A:255 } */
#define SG_SEA_GREEN_INIT { 0.180392157f, 0.545098039f, 0.341176471f, 1.0f }
/* Seashell { R:255, G:245, B:238, A:255 } */
#define SG_SEASHELL_INIT { 1.0f, 0.960784314f, 0.933333333f, 1.0f }
/* Sienna { R:160, G:82, B:45, A:255 } */
#define SG_SIENNA_INIT { 0.62745098f, 0.321568627f, 0.176470588f, 1.0f }
/* Silver { R:192, G:192, B:192, A:255 } */
#define SG_SILVER_INIT { 0.752941176f, 0.752941176f, 0.752941176f, 1.0f }
/* Sky Blue { R:135, G:206, B:235, A:255 } */
#define SG_SKY_BLUE_INIT { 0.529411765f, 0.807843137f, 0.921568627f, 1.0f }
/* Slate Blue { R:106, G:90, B:205, A:255 } */
#define SG_SLATE_BLUE_INIT { 0.415686275f, 0.352941176f, 0.803921569f, 1.0f }
/* Slate Gray { R:112, G:128, B:144, A:255 } */
#define SG_SLATE_GRAY_INIT { 0.439215686f, 0.501960784f, 0.564705882f, 1.0f }
/* Slate Grey { R:112, G:128, B:144, A:255 } */
#define SG_SLATE_GREY_INIT { 0.439215686f, 0.501960784f, 0.564705882f, 1.0f }
/* Snow { R:255, G:250, B:250, A:255 } */
#define SG_SNOW_INIT { 1.0f, 0.980392157f, 0.980392157f, 1.0f }
/* Spring Green { R:0, G:255, B:127, A:255 } */
#define SG_SPRING_GREEN_INIT { 0.0f, 1.0f, 0.498039216f, 1.0f }
/* Steel Blue { R:70, G:130, B:180, A:255 } */
#define SG_STEEL_BLUE_INIT { 0.274509804f, 0.509803922f, 0.705882353f, 1.0f }
/* Tan { R:210, G:180, B:140, A:255 } */
#define SG_TAN_INIT { 0.823529412f, 0.705882353f, 0.549019608f, 1.0f }
/* Teal { R:0, G:128, B:128, A:255 } */
#define SG_TEAL_INIT { 0.0f, 0.501960784f, 0.501960784f, 1.0f }
/* Thistle { R:216, G:191, B:216, A:255 } */
#define SG_THISTLE_INIT { 0.847058824f, 0.749019608f, 0.847058824f, 1.0f }
/* Tomato { R:255, G:99, B:71, A:255 } */
#define SG_TOMATO_INIT { 1.0f, 0.388235294f, 0.278431373f, 1.0f }
/* Transparent { R:0, G:0, B:0, A:0 } */
#define SG_TRANSPARENT_INIT { 0.0f, 0.0f, 0.0f, 0.0f }
/* Turquoise { R:64, G:224, B:208, A:255 } */
#define SG_TURQUOISE_INIT { 0.250980392f, 0.878431373f, 0.815686275f, 1.0f }
/* Violet { R:238, G:130, B:238, A:255 } */
#define SG_VIOLET_INIT { 0.933333333f, 0.509803922f, 0.933333333f, 1.0f }
/* Wheat { R:245, G:222, B:179, A:255 } */
#define SG_WHEAT_INIT { 0.960784314f, 0.870588235f, 0.701960784f, 1.0f }
/* White { R:255, G:255, B:255, A:255 } */
#define SG_WHITE_INIT { 1.0f, 1.0f, 1.0f, 1.0f }
/* White Smoke { R:245, G:245, B:245, A:255 } */
#define SG_WHITE_SMOKE_INIT { 0.960784314f, 0.960784314f, 0.960784314f, 1.0f }
/* Yellow { R:255, G:255, B:0, A:255 } */
#define SG_YELLOW_INIT { 1.0f, 1.0f, 0.0f, 1.0f }
/* Yellow Green { R:154, G:205, B:50, A:255 } */
#define SG_YELLOW_GREEN_INIT { 0.603921569f, 0.803921569f, 0.196078431f, 1.0f }

/* Alice Blue { R:240, G:248, B:255, A:255 } */
static const sg_color sg_alice_blue = SG_ALICE_BLUE_INIT;
/* Antique White { R:250, G:235, B:215, A:255 } */
static const sg_color sg_antique_white = SG_ANTIQUE_WHITE_INIT;
/* Aqua { R:0, G:255, B:255, A:255 } */
static const sg_color sg_aqua = SG_AQUA_INIT;
/* Aquamarine { R:127, G:255, B:212, A:255 } */
static const sg_color sg_aquamarine = SG_AQUAMARINE_INIT;
/* Azure { R:240, G:255, B:255, A:255 } */
static const sg_color sg_azure = SG_AZURE_INIT;
/* Beige { R:245, G:245, B:220, A:255 } */
static const sg_color sg_beige = SG_BEIGE_INIT;
/* Bisque { R:255, G:228, B:196, A:255 } */
static const sg_color sg_bisque = SG_BISQUE_INIT;
/* Black { R:0, G:0, B:0, A:255 } */
static const sg_color sg_black = SG_BLACK_INIT;
/* Blanched Almond { R:255, G:235, B:205, A:255 } */
static const sg_color sg_blanched_almond = SG_BLANCHED_ALMOND_INIT;
/* Blue { R:0, G:0, B:255, A:255 } */
static const sg_color sg_blue = SG_BLUE_INIT;
/* Blue Violet { R:138, G:43, B:226, A:255 } */
static const sg_color sg_blue_violet = SG_BLUE_VIOLET_INIT;
/* Brown { R:165, G:42, B:42, A:255 } */
static const sg_color sg_brown = SG_BROWN_INIT;
/* Burly Wood { R:222, G:184, B:135, A:255 } */
static const sg_color sg_burly_wood = SG_BURLY_WOOD_INIT;
/* Cadet Blue { R:95, G:158, B:160, A:255 } */
static const sg_color sg_cadet_blue = SG_CADET_BLUE_INIT;
/* Chartreuse { R:127, G:255, B:0, A:255 } */
static const sg_color sg_chartreuse = SG_CHARTREUSE_INIT;
/* Chocolate { R:210, G:105, B:30, A:255 } */
static const sg_color sg_chocolate = SG_CHOCOLATE_INIT;
/* Coral { R:255, G:127, B:80, A:255 } */
static const sg_color sg_coral = SG_CORAL_INIT;
/* Cornflower Blue { R:100, G:149, B:237, A:255 } */
static const sg_color sg_cornflower_blue = SG_CORNFLOWER_BLUE_INIT;
/* Cornsilk { R:255, G:248, B:220, A:255 } */
static const sg_color sg_cornsilk = SG_CORNSILK_INIT;
/* Crimson { R:220, G:20, B:60, A:255 } */
static const sg_color sg_crimson = SG_CRIMSON_INIT;
/* Cyan { R:0, G:255, B:255, A:255 } */
static const sg_color sg_cyan = SG_CYAN_INIT;
/* Dark Blue { R:0, G:0, B:139, A:255 } */
static const sg_color sg_dark_blue = SG_DARK_BLUE_INIT;
/* Dark Cyan { R:0, G:139, B:139, A:255 } */
static const sg_color sg_dark_cyan = SG_DARK_CYAN_INIT;
/* Dark Goldenrod { R:184, G:134, B:11, A:255 } */
static const sg_color sg_dark_goldenrod = SG_DARK_GOLDENROD_INIT;
/* Dark Gray { R:169, G:169, B:169, A:255 } */
static const sg_color sg_dark_gray = SG_DARK_GRAY_INIT;
/* Dark Grey { R:169, G:169, B:169, A:255 } */
static const sg_color sg_dark_grey = SG_DARK_GREY_INIT;
/* Dark Green { R:0, G:100, B:0, A:255 } */
static const sg_color sg_dark_green = SG_DARK_GREEN_INIT;
/* Dark Khaki { R:189, G:183, B:107, A:255 } */
static const sg_color sg_dark_khaki = SG_DARK_KHAKI_INIT;
/* Dark Magenta { R:139, G:0, B:139, A:255 } */
static const sg_color sg_dark_magenta = SG_DARK_MAGENTA_INIT;
/* Dark Olive Green { R:85, G:107, B:47, A:255 } */
static const sg_color sg_dark_olive_green = SG_DARK_OLIVE_GREEN_INIT;
/* Dark Orange { R:255, G:140, B:0, A:255 } */
static const sg_color sg_dark_orange = SG_DARK_ORANGE_INIT;
/* Dark Orchid { R:153, G:50, B:204, A:255 } */
static const sg_color sg_dark_orchid = SG_DARK_ORCHID_INIT;
/* Dark Red { R:139, G:0, B:0, A:255 } */
static const sg_color sg_dark_red = SG_DARK_RED_INIT;
/* Dark Salmon { R:233, G:150, B:122, A:255 } */
static const sg_color sg_dark_salmon = SG_DARK_SALMON_INIT;
/* Dark Sea Green { R:143, G:188, B:143, A:255 } */
static const sg_color sg_dark_sea_green = SG_DARK_SEA_GREEN_INIT;
/* Dark Slate Blue { R:72, G:61, B:139, A:255 } */
static const sg_color sg_dark_slate_blue = SG_DARK_SLATE_BLUE_INIT;
/* Dark Slate Gray { R:47, G:79, B:79, A:255 } */
static const sg_color sg_dark_slate_gray = SG_DARK_SLATE_GRAY_INIT;
/* Dark Slate Grey { R:47, G:79, B:79, A:255 } */
static const sg_color sg_dark_slate_grey = SG_DARK_SLATE_GREY_INIT;
/* Dark Turquoise { R:0, G:206, B:209, A:255 } */
static const sg_color sg_dark_turquoise = SG_DARK_TURQUOISE_INIT;
/* Dark Violet { R:148, G:0, B:211, A:255 } */
static const sg_color sg_dark_violet = SG_DARK_VIOLET_INIT;
/* Deep Pink { R:255, G:20, B:147, A:255 } */
static const sg_color sg_deep_pink = SG_DEEP_PINK_INIT;
/* Deep Sky Blue { R:0, G:191, B:255, A:255 } */
static const sg_color sg_deep_sky_blue = SG_DEEP_SKY_BLUE_INIT;
/* Dim Gray { R:105, G:105, B:105, A:255 } */
static const sg_color sg_dim_gray = SG_DIM_GRAY_INIT;
/* Dim Grey { R:105, G:105, B:105, A:255 } */
static const sg_color sg_dim_grey = SG_DIM_GREY_INIT;
/* Dodger Blue { R:30, G:144, B:255, A:255 } */
static const sg_color sg_dodger_blue = SG_DODGER_BLUE_INIT;
/* Fire Brick { R:178, G:34, B:34, A:255 } */
static const sg_color sg_fire_brick = SG_FIRE_BRICK_INIT;
/* Floral White { R:255, G:250, B:240, A:255 } */
static const sg_color sg_floral_white = SG_FLORAL_WHITE_INIT;
/* Forest Green { R:34, G:139, B:34, A:255 } */
static const sg_color sg_forest_green = SG_FOREST_GREEN_INIT;
/* Fuchsia { R:255, G:0, B:255, A:255 } */
static const sg_color sg_fuchsia = SG_FUCHSIA_INIT;
/* Gainsboro { R:220, G:220, B:220, A:255 } */
static const sg_color sg_gainsboro = SG_GAINSBORO_INIT;
/* Ghost White { R:248, G:248, B:255, A:255 } */
static const sg_color sg_ghost_white = SG_GHOST_WHITE_INIT;
/* Gold { R:255, G:215, B:0, A:255 } */
static const sg_color sg_gold = SG_GOLD_INIT;
/* Goldenrod { R:218, G:165, B:32, A:255 } */
static const sg_color sg_goldenrod = SG_GOLDENROD_INIT;
/* Gray { R:128, G:128, B:128, A:255 } */
static const sg_color sg_gray = SG_GRAY_INIT;
/* Grey { R:128, G:128, B:128, A:255 } */
static const sg_color sg_grey = SG_GREY_INIT;
/* Green { R:0, G:128, B:0, A:255 } */
static const sg_color sg_green = SG_GREEN_INIT;
/* Green Yellow { R:173, G:255, B:47, A:255 } */
static const sg_color sg_green_yellow = SG_GREEN_YELLOW_INIT;
/* Honeydew { R:240, G:255, B:240, A:255 } */
static const sg_color sg_honeydew = SG_HONEYDEW_INIT;
/* Hot Pink { R:255, G:105, B:180, A:255 } */
static const sg_color sg_hot_pink = SG_HOT_PINK_INIT;
/* Indian Red { R:205, G:92, B:92, A:255 } */
static const sg_color sg_indian_red = SG_INDIAN_RED_INIT;
/* Indigo { R:75, G:0, B:130, A:255 } */
static const sg_color sg_indigo = SG_INDIGO_INIT;
/* Ivory { R:255, G:255, B:240, A:255 } */
static const sg_color sg_ivory = SG_IVORY_INIT;
/* Khaki { R:240, G:230, B:140, A:255 } */
static const sg_color sg_khaki = SG_KHAKI_INIT;
/* Lavender { R:230, G:230, B:250, A:255 } */
static const sg_color sg_lavender = SG_LAVENDER_INIT;
/* Lavender Blush { R:255, G:240, B:245, A:255 } */
static const sg_color sg_lavender_blush = SG_LAVENDER_BLUSH_INIT;
/* Lawn Green { R:124, G:252, B:0, A:255 } */
static const sg_color sg_lawn_green = SG_LAWN_GREEN_INIT;
/* Lemon Chiffon { R:255, G:250, B:205, A:255 } */
static const sg_color sg_lemon_chiffon = SG_LEMON_CHIFFON_INIT;
/* Light Blue { R:173, G:216, B:230, A:255 } */
static const sg_color sg_light_blue = SG_LIGHT_BLUE_INIT;
/* Light Coral { R:240, G:128, B:128, A:255 } */
static const sg_color sg_light_coral = SG_LIGHT_CORAL_INIT;
/* Light Cyan { R:224, G:255, B:255, A:255 } */
static const sg_color sg_light_cyan = SG_LIGHT_CYAN_INIT;
/* Light Goldenrod Yellow { R:250, G:250, B:210, A:255 } */
static const sg_color sg_light_goldenrod_yellow = SG_LIGHT_GOLDENROD_YELLOW_INIT;
/* Light Green { R:211, G:211, B:211, A:255 } */
static const sg_color sg_light_green = SG_LIGHT_GREEN_INIT;
/* Light Gray { R:211, G:211, B:211, A:255 } */
static const sg_color sg_light_gray = SG_LIGHT_GRAY_INIT;
/* Light Grey { R:211, G:211, B:211, A:255 } */
static const sg_color sg_light_grey = SG_LIGHT_GREY_INIT;
/* Light Pink { R:255, G:182, B:193, A:255 } */
static const sg_color sg_light_pink = SG_LIGHT_PINK_INIT;
/* Light Salmon { R:255, G:160, B:122, A:255 } */
static const sg_color sg_light_salmon = SG_LIGHT_SALMON_INIT;
/* Light Sea Green { R:32, G:178, B:170, A:255 } */
static const sg_color sg_light_sea_green = SG_LIGHT_SEA_GREEN_INIT;
/* Light Sky Blue { R:135, G:206, B:250, A:255 } */
static const sg_color sg_light_sky_blue = SG_LIGHT_SKY_BLUE_INIT;
/* Light Slate Gray { R:119, G:136, B:153, A:255 } */
static const sg_color sg_light_slate_gray = SG_LIGHT_SLATE_GRAY_INIT;
/* Light Slate Grey { R:119, G:136, B:153, A:255 } */
static const sg_color sg_light_slate_grey = SG_LIGHT_SLATE_GREY_INIT;
/* Light Steel Blue { R:176, G:196, B:222, A:255 } */
static const sg_color sg_light_steel_blue = SG_LIGHT_STEEL_BLUE_INIT;
/* Light Yellow { R:255, G:255, B:224, A:255 } */
static const sg_color sg_light_yellow = SG_LIGHT_YELLOW_INIT;
/* Lime { R:0, G:255, B:0, A:255 } */
static const sg_color sg_lime = SG_LIME_INIT;
/* Lime Green { R:50, G:205, B:50, A:255 } */
static const sg_color sg_lime_green = SG_LIME_GREEN_INIT;
/* Linen { R:250, G:240, B:230, A:255 } */
static const sg_color sg_linen = SG_LINEN_INIT;
/* Magenta { R:255, G:0, B:255, A:255 } */
static const sg_color sg_magenta = SG_MAGENTA_INIT;
/* Maroon { R:128, G:0, B:0, A:255 } */
static const sg_color sg_maroon = SG_MAROON_INIT;
/* Medium Aquamarine { R:102, G:205, B:170, A:255 } */
static const sg_color sg_medium_aquamarine = SG_MEDIUM_AQUAMARINE_INIT;
/* Medium Blue { R:0, G:0, B:205, A:255 } */
static const sg_color sg_medium_blue = SG_MEDIUM_BLUE_INIT;
/* Medium Orchid { R:186, G:85, B:211, A:255 } */
static const sg_color sg_medium_orchid = SG_MEDIUM_ORCHID_INIT;
/* Medium Purple { R:147, G:112, B:216, A:255 } */
static const sg_color sg_medium_purple = SG_MEDIUM_PURPLE_INIT;
/* Medium Sea Green { R:60, G:179, B:113, A:255 } */
static const sg_color sg_medium_sea_green = SG_MEDIUM_SEA_GREEN_INIT;
/* Medium Slate Blue { R:123, G:104, B:238, A:255 } */
static const sg_color sg_medium_slate_blue = SG_MEDIUM_SLATE_BLUE_INIT;
/* Medium Spring Green { R:0, G:250, B:154, A:255 } */
static const sg_color sg_medium_spring_green = SG_MEDIUM_SPRING_GREEN_INIT;
/* Medium Turquoise { R:72, G:209, B:204, A:255 } */
static const sg_color sg_medium_turquoise = SG_MEDIUM_TURQUOISE_INIT;
/* Medium Violet Red { R:199, G:21, B:133, A:255 } */
static const sg_color sg_medium_violet_red = SG_MEDIUM_VIOLET_RED_INIT;
/* Midnight Blue { R:25, G:25, B:112, A:255 } */
static const sg_color sg_midnight_blue = SG_MIDNIGHT_BLUE_INIT;
/* Mint Cream { R:245, G:255, B:250, A:255 } */
static const sg_color sg_mint_cream = SG_MINT_CREAM_INIT;
/* Misty Rose { R:255, G:228, B:225, A:255 } */
static const sg_color sg_misty_rose = SG_MISTY_ROSE_INIT;
/* Moccasin { R:255, G:228, B:181, A:255 } */
static const sg_color sg_moccasin = SG_MOCCASIN_INIT;
/* Navajo White { R:255, G:222, B:173, A:255 } */
static const sg_color sg_navajo_white = SG_NAVAJO_WHITE_INIT;
/* Navy { R:0, G:0, B:128, A:255 } */
static const sg_color sg_navy = SG_NAVY_INIT;
/* Old Lace { R:253, G:245, B:230, A:255 } */
static const sg_color sg_old_lace = SG_OLD_LACE_INIT;
/* Olive { R:128, G:128, B:0, A:255 } */
static const sg_color sg_olive = SG_OLIVE_INIT;
/* Olive Drab { R:107, G:142, B:35, A:255 } */
static const sg_color sg_olive_drab = SG_OLIVE_DRAB_INIT;
/* Orange { R:255, G:165, B:0, A:255 } */
static const sg_color sg_orange = SG_ORANGE_INIT;
/* Orange Red { R:255, G:69, B:0, A:255 } */
static const sg_color sg_orange_red = SG_ORANGE_RED_INIT;
/* Orchid { R:218, G:112, B:214, A:255 } */
static const sg_color sg_orchid = SG_ORCHID_INIT;
/* Pale Goldenrod { R:238, G:232, B:170, A:255 } */
static const sg_color sg_pale_goldenrod = SG_PALE_GOLDENROD_INIT;
/* Pale Green { R:152, G:251, B:152, A:255 } */
static const sg_color sg_pale_green = SG_PALE_GREEN_INIT;
/* Pale Turquoise { R:175, G:238, B:238, A:255 } */
static const sg_color sg_pale_turquoise = SG_PALE_TURQUOISE_INIT;
/* Pale Violet Red { R:216, G:112, B:147, A:255 } */
static const sg_color sg_pale_violet_red = SG_PALE_VIOLET_RED_INIT;
/* Papaya Whip { R:255, G:239, B:213, A:255 } */
static const sg_color sg_papaya_whip = SG_PAPAYA_WHIP_INIT;
/* Peach Puff { R:255, G:218, B:185, A:255 } */
static const sg_color sg_peach_puff = SG_PEACH_PUFF_INIT;
/* Peru { R:205, G:133, B:63, A:255 } */
static const sg_color sg_peru = SG_PERU_INIT;
/* Pink { R:255, G:192, B:203, A:255 } */
static const sg_color sg_pink = SG_PINK_INIT;
/* Plum { R:221, G:160, B:221, A:255 } */
static const sg_color sg_plum = SG_PLUM_INIT;
/* Powder Blue { R:176, G:224, B:230, A:255 } */
static const sg_color sg_powder_blue = SG_POWDER_BLUE_INIT;
/* Purple { R:128, G:0, B:128, A:255 } */
static const sg_color sg_purple = SG_PURPLE_INIT;
/* Red { R:255, G:0, B:0, A:255 } */
static const sg_color sg_red = SG_RED_INIT;
/* Rosy Brown { R:188, G:143, B:143, A:255 } */
static const sg_color sg_rosy_brown = SG_ROSY_BROWN_INIT;
/* Royal Blue { R:65, G:105, B:225, A:255 } */
static const sg_color sg_royal_blue = SG_ROYAL_BLUE_INIT;
/* Saddle Brown { R:139, G:69, B:19, A:255 } */
static const sg_color sg_saddle_brown = SG_SADDLE_BROWN_INIT;
/* Salmon { R:250, G:128, B:114, A:255 } */
static const sg_color sg_salmon = SG_SALMON_INIT;
/* Sandy Brown { R:244, G:164, B:96, A:255 } */
static const sg_color sg_sandy_brown = SG_SANDY_BROWN_INIT;
/* Sea Green { R:46, G:139, B:87, A:255 } */
static const sg_color sg_sea_green = SG_SEA_GREEN_INIT;
/* Seashell { R:255, G:245, B:238, A:255 } */
static const sg_color sg_seashell = SG_SEASHELL_INIT;
/* Sienna { R:160, G:82, B:45, A:255 } */
static const sg_color sg_sienna = SG_SIENNA_INIT;
/* Silver { R:192, G:192, B:192, A:255 } */
static const sg_color sg_silver = SG_SILVER_INIT;
/* Sky Blue { R:135, G:206, B:235, A:255 } */
static const sg_color sg_sky_blue = SG_SKY_BLUE_INIT;
/* Slate Blue { R:106, G:90, B:205, A:255 } */
static const sg_color sg_slate_blue = SG_SLATE_BLUE_INIT;
/* Slate Gray { R:112, G:128, B:144, A:255 } */
static const sg_color sg_slate_gray = SG_SLATE_GRAY_INIT;
/* Slate Grey { R:112, G:128, B:144, A:255 } */
static const sg_color sg_slate_grey = SG_SLATE_GREY_INIT;
/* Snow { R:255, G:250, B:250, A:255 } */
static const sg_color sg_snow = SG_SNOW_INIT;
/* Spring Green { R:0, G:255, B:127, A:255 } */
static const sg_color sg_spring_green = SG_SPRING_GREEN_INIT;
/* Steel Blue { R:70, G:130, B:180, A:255 } */
static const sg_color sg_steel_blue = SG_STEEL_BLUE_INIT;
/* Tan { R:210, G:180, B:140, A:255 } */
static const sg_color sg_tan = SG_TAN_INIT;
/* Teal { R:0, G:128, B:128, A:255 } */
static const sg_color sg_teal = SG_TEAL_INIT;
/* Thistle { R:216, G:191, B:216, A:255 } */
static const sg_color sg_thistle = SG_THISTLE_INIT;
/* Tomato { R:255, G:99, B:71, A:255 } */
static const sg_color sg_tomato = SG_TOMATO_INIT;
/* Transparent { R:0, G:0, B:0, A:0 } */
static const sg_color sg_transparent = SG_TRANSPARENT_INIT;
/* Turquoise { R:64, G:224, B:208, A:255 } */
static const sg_color sg_turquoise = SG_TURQUOISE_INIT;
/* Violet { R:238, G:130, B:238, A:255 } */
static const sg_color sg_violet = SG_VIOLET_INIT;
/* Wheat { R:245, G:222, B:179, A:255 } */
static const sg_color sg_wheat = SG_WHEAT_INIT;
/* White { R:255, G:255, B:255, A:255 } */
static const sg_color sg_white = SG_WHITE_INIT;
/* White Smoke { R:245, G:245, B:245, A:255 } */
static const sg_color sg_white_smoke = SG_WHITE_SMOKE_INIT;
/* Yellow { R:255, G:255, B:0, A:255 } */
static const sg_color sg_yellow = SG_YELLOW_INIT;
/* Yellow Green { R:154, G:205, B:50, A:255 } */
static const sg_color sg_yellow_green = SG_YELLOW_GREEN_INIT;

/* Alice Blue { R:240, G:248, B:255, A:255 } */
#define SG_ALICE_BLUE_RGBA32 0xF0F8FFFF
/* Antique White { R:250, G:235, B:215, A:255 } */
#define SG_ANTIQUE_WHITE_RGBA32 0xFAEBD7FF
/* Aqua { R:0, G:255, B:255, A:255 } */
#define SG_AQUA_RGBA32 0x00FFFFFF
/* Aquamarine { R:127, G:255, B:212, A:255 } */
#define SG_AQUAMARINE_RGBA32 0x7FFFD4FF
/* Azure { R:240, G:255, B:255, A:255 } */
#define SG_AZURE_RGBA32 0xF0FFFFFF
/* Beige { R:245, G:245, B:220, A:255 } */
#define SG_BEIGE_RGBA32 0xF5F5DCFF
/* Bisque { R:255, G:228, B:196, A:255 } */
#define SG_BISQUE_RGBA32 0xFFE4C4FF
/* Black { R:0, G:0, B:0, A:255 } */
#define SG_BLACK_RGBA32 0x000000FF
/* Blanched Almond { R:255, G:235, B:205, A:255 } */
#define SG_BLANCHED_ALMOND_RGBA32 0xFFEBCDFF
/* Blue { R:0, G:0, B:255, A:255 } */
#define SG_BLUE_RGBA32 0x0000FFFF
/* Blue Violet { R:138, G:43, B:226, A:255 } */
#define SG_BLUE_VIOLET_RGBA32 0x8A2BE2FF
/* Brown { R:165, G:42, B:42, A:255 } */
#define SG_BROWN_RGBA32 0xA52A2AFF
/* Burly Wood { R:222, G:184, B:135, A:255 } */
#define SG_BURLY_WOOD_RGBA32 0xDEB887FF
/* Cadet Blue { R:95, G:158, B:160, A:255 } */
#define SG_CADET_BLUE_RGBA32 0x5F9EA0FF
/* Chartreuse { R:127, G:255, B:0, A:255 } */
#define SG_CHARTREUSE_RGBA32 0x7FFF00FF
/* Chocolate { R:210, G:105, B:30, A:255 } */
#define SG_CHOCOLATE_RGBA32 0xD2691EFF
/* Coral { R:255, G:127, B:80, A:255 } */
#define SG_CORAL_RGBA32 0xFF7F50FF
/* Cornflower Blue { R:100, G:149, B:237, A:255 } */
#define SG_CORNFLOWER_BLUE_RGBA32 0x6495EDFF
/* Cornsilk { R:255, G:248, B:220, A:255 } */
#define SG_CORNSILK_RGBA32 0xFFF8DCFF
/* Crimson { R:220, G:20, B:60, A:255 } */
#define SG_CRIMSON_RGBA32 0xDC143CFF
/* Cyan { R:0, G:255, B:255, A:255 } */
#define SG_CYAN_RGBA32 0x00FFFFFF
/* Dark Blue { R:0, G:0, B:139, A:255 } */
#define SG_DARK_BLUE_RGBA32 0x00008BFF
/* Dark Cyan { R:0, G:139, B:139, A:255 } */
#define SG_DARK_CYAN_RGBA32 0x008B8BFF
/* Dark Goldenrod { R:184, G:134, B:11, A:255 } */
#define SG_DARK_GOLDENROD_RGBA32 0xB8860BFF
/* Dark Gray { R:169, G:169, B:169, A:255 } */
#define SG_DARK_GRAY_RGBA32 0xA9A9A9FF
/* Dark Grey { R:169, G:169, B:169, A:255 } */
#define SG_DARK_GREY_RGBA32 0xA9A9A9FF
/* Dark Green { R:0, G:100, B:0, A:255 } */
#define SG_DARK_GREEN_RGBA32 0x006400FF
/* Dark Khaki { R:189, G:183, B:107, A:255 } */
#define SG_DARK_KHAKI_RGBA32 0xBDB76BFF
/* Dark Magenta { R:139, G:0, B:139, A:255 } */
#define SG_DARK_MAGENTA_RGBA32 0x8B008BFF
/* Dark Olive Green { R:85, G:107, B:47, A:255 } */
#define SG_DARK_OLIVE_GREEN_RGBA32 0x556B2FFF
/* Dark Orange { R:255, G:140, B:0, A:255 } */
#define SG_DARK_ORANGE_RGBA32 0xFF8C00FF
/* Dark Orchid { R:153, G:50, B:204, A:255 } */
#define SG_DARK_ORCHID_RGBA32 0x9932CCFF
/* Dark Red { R:139, G:0, B:0, A:255 } */
#define SG_DARK_RED_RGBA32 0x8B0000FF
/* Dark Salmon { R:233, G:150, B:122, A:255 } */
#define SG_DARK_SALMON_RGBA32 0xE9967AFF
/* Dark Sea Green { R:143, G:188, B:143, A:255 } */
#define SG_DARK_SEA_GREEN_RGBA32 0x8FBC8FFF
/* Dark Slate Blue { R:72, G:61, B:139, A:255 } */
#define SG_DARK_SLATE_BLUE_RGBA32 0x483D8BFF
/* Dark Slate Gray { R:47, G:79, B:79, A:255 } */
#define SG_DARK_SLATE_GRAY_RGBA32 0x2F4F4FFF
/* Dark Slate Grey { R:47, G:79, B:79, A:255 } */
#define SG_DARK_SLATE_GREY_RGBA32 0x2F4F4FFF
/* Dark Turquoise { R:0, G:206, B:209, A:255 } */
#define SG_DARK_TURQUOISE_RGBA32 0x00CED1FF
/* Dark Violet { R:148, G:0, B:211, A:255 } */
#define SG_DARK_VIOLET_RGBA32 0x9400D3FF
/* Deep Pink { R:255, G:20, B:147, A:255 } */
#define SG_DEEP_PINK_RGBA32 0xFF1493FF
/* Deep Sky Blue { R:0, G:191, B:255, A:255 } */
#define SG_DEEP_SKY_BLUE_RGBA32 0x00BFFFFF
/* Dim Gray { R:105, G:105, B:105, A:255 } */
#define SG_DIM_GRAY_RGBA32 0x696969FF
/* Dim Grey { R:105, G:105, B:105, A:255 } */
#define SG_DIM_GREY_RGBA32 0x696969FF
/* Dodger Blue { R:30, G:144, B:255, A:255 } */
#define SG_DODGER_BLUE_RGBA32 0x1E90FFFF
/* Fire Brick { R:178, G:34, B:34, A:255 } */
#define SG_FIRE_BRICK_RGBA32 0xB22222FF
/* Floral White { R:255, G:250, B:240, A:255 } */
#define SG_FLORAL_WHITE_RGBA32 0xFFFAF0FF
/* Forest Green { R:34, G:139, B:34, A:255 } */
#define SG_FOREST_GREEN_RGBA32 0x228B22FF
/* Fuchsia { R:255, G:0, B:255, A:255 } */
#define SG_FUCHSIA_RGBA32 0xFF00FFFF
/* Gainsboro { R:220, G:220, B:220, A:255 } */
#define SG_GAINSBORO_RGBA32 0xDCDCDCFF
/* Ghost White { R:248, G:248, B:255, A:255 } */
#define SG_GHOST_WHITE_RGBA32 0xF8F8FFFF
/* Gold { R:255, G:215, B:0, A:255 } */
#define SG_GOLD_RGBA32 0xFFD700FF
/* Goldenrod { R:218, G:165, B:32, A:255 } */
#define SG_GOLDENROD_RGBA32 0xDAA520FF
/* Gray { R:128, G:128, B:128, A:255 } */
#define SG_GRAY_RGBA32 0x808080FF
/* Grey { R:128, G:128, B:128, A:255 } */
#define SG_GREY_RGBA32 0x808080FF
/* Green { R:0, G:128, B:0, A:255 } */
#define SG_GREEN_RGBA32 0x008000FF
/* Green Yellow { R:173, G:255, B:47, A:255 } */
#define SG_GREEN_YELLOW_RGBA32 0xADFF2FFF
/* Honeydew { R:240, G:255, B:240, A:255 } */
#define SG_HONEYDEW_RGBA32 0xF0FFF0FF
/* Hot Pink { R:255, G:105, B:180, A:255 } */
#define SG_HOT_PINK_RGBA32 0xFF69B4FF
/* Indian Red { R:205, G:92, B:92, A:255 } */
#define SG_INDIAN_RED_RGBA32 0xCD5C5CFF
/* Indigo { R:75, G:0, B:130, A:255 } */
#define SG_INDIGO_RGBA32 0x4B0082FF
/* Ivory { R:255, G:255, B:240, A:255 } */
#define SG_IVORY_RGBA32 0xFFFFF0FF
/* Khaki { R:240, G:230, B:140, A:255 } */
#define SG_KHAKI_RGBA32 0xF0E68CFF
/* Lavender { R:230, G:230, B:250, A:255 } */
#define SG_LAVENDER_RGBA32 0xE6E6FAFF
/* Lavender Blush { R:255, G:240, B:245, A:255 } */
#define SG_LAVENDER_BLUSH_RGBA32 0xFFF0F5FF
/* Lawn Green { R:124, G:252, B:0, A:255 } */
#define SG_LAWN_GREEN_RGBA32 0x7CFC00FF
/* Lemon Chiffon { R:255, G:250, B:205, A:255 } */
#define SG_LEMON_CHIFFON_RGBA32 0xFFFACDFF
/* Light Blue { R:173, G:216, B:230, A:255 } */
#define SG_LIGHT_BLUE_RGBA32 0xADD8E6FF
/* Light Coral { R:240, G:128, B:128, A:255 } */
#define SG_LIGHT_CORAL_RGBA32 0xF08080FF
/* Light Cyan { R:224, G:255, B:255, A:255 } */
#define SG_LIGHT_CYAN_RGBA32 0xE0FFFFFF
/* Light Goldenrod Yellow { R:250, G:250, B:210, A:255 } */
#define SG_LIGHT_GOLDENROD_YELLOW_RGBA32 0xFAFAD2FF
/* Light Green { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GREEN_RGBA32 0xD3D3D3FF
/* Light Gray { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GRAY_RGBA32 0xD3D3D3FF
/* Light Grey { R:211, G:211, B:211, A:255 } */
#define SG_LIGHT_GREY_RGBA32 0xD3D3D3FF
/* Light Pink { R:255, G:182, B:193, A:255 } */
#define SG_LIGHT_PINK_RGBA32 0xFFB6C1FF
/* Light Salmon { R:255, G:160, B:122, A:255 } */
#define SG_LIGHT_SALMON_RGBA32 0xFFA07AFF
/* Light Sea Green { R:32, G:178, B:170, A:255 } */
#define SG_LIGHT_SEA_GREEN_RGBA32 0x20B2AAFF
/* Light Sky Blue { R:135, G:206, B:250, A:255 } */
#define SG_LIGHT_SKY_BLUE_RGBA32 0x87CEFAFF
/* Light Slate Gray { R:119, G:136, B:153, A:255 } */
#define SG_LIGHT_SLATE_GRAY_RGBA32 0x778899FF
/* Light Slate Grey { R:119, G:136, B:153, A:255 } */
#define SG_LIGHT_SLATE_GREY_RGBA32 0x778899FF
/* Light Steel Blue { R:176, G:196, B:222, A:255 } */
#define SG_LIGHT_STEEL_BLUE_RGBA32 0xB0C4DEFF
/* Light Yellow { R:255, G:255, B:224, A:255 } */
#define SG_LIGHT_YELLOW_RGBA32 0xFFFFE0FF
/* Lime { R:0, G:255, B:0, A:255 } */
#define SG_LIME_RGBA32 0x00FF00FF
/* Lime Green { R:50, G:205, B:50, A:255 } */
#define SG_LIME_GREEN_RGBA32 0x32CD32FF
/* Linen { R:250, G:240, B:230, A:255 } */
#define SG_LINEN_RGBA32 0xFAF0E6FF
/* Magenta { R:255, G:0, B:255, A:255 } */
#define SG_MAGENTA_RGBA32 0xFF00FFFF
/* Maroon { R:128, G:0, B:0, A:255 } */
#define SG_MAROON_RGBA32 0x800000FF
/* Medium Aquamarine { R:102, G:205, B:170, A:255 } */
#define SG_MEDIUM_AQUAMARINE_RGBA32 0x66CDAAFF
/* Medium Blue { R:0, G:0, B:205, A:255 } */
#define SG_MEDIUM_BLUE_RGBA32 0x0000CDFF
/* Medium Orchid { R:186, G:85, B:211, A:255 } */
#define SG_MEDIUM_ORCHID_RGBA32 0xBA55D3FF
/* Medium Purple { R:147, G:112, B:216, A:255 } */
#define SG_MEDIUM_PURPLE_RGBA32 0x9370D8FF
/* Medium Sea Green { R:60, G:179, B:113, A:255 } */
#define SG_MEDIUM_SEA_GREEN_RGBA32 0x3CB371FF
/* Medium Slate Blue { R:123, G:104, B:238, A:255 } */
#define SG_MEDIUM_SLATE_BLUE_RGBA32 0x7B68EEFF
/* Medium Spring Green { R:0, G:250, B:154, A:255 } */
#define SG_MEDIUM_SPRING_GREEN_RGBA32 0x00FA9AFF
/* Medium Turquoise { R:72, G:209, B:204, A:255 } */
#define SG_MEDIUM_TURQUOISE_RGBA32 0x48D1CCFF
/* Medium Violet Red { R:199, G:21, B:133, A:255 } */
#define SG_MEDIUM_VIOLET_RED_RGBA32 0xC71585FF
/* Midnight Blue { R:25, G:25, B:112, A:255 } */
#define SG_MIDNIGHT_BLUE_RGBA32 0x191970FF
/* Mint Cream { R:245, G:255, B:250, A:255 } */
#define SG_MINT_CREAM_RGBA32 0xF5FFFAFF
/* Misty Rose { R:255, G:228, B:225, A:255 } */
#define SG_MISTY_ROSE_RGBA32 0xFFE4E1FF
/* Moccasin { R:255, G:228, B:181, A:255 } */
#define SG_MOCCASIN_RGBA32 0xFFE4B5FF
/* Navajo White { R:255, G:222, B:173, A:255 } */
#define SG_NAVAJO_WHITE_RGBA32 0xFFDEADFF
/* Navy { R:0, G:0, B:128, A:255 } */
#define SG_NAVY_RGBA32 0x000080FF
/* Old Lace { R:253, G:245, B:230, A:255 } */
#define SG_OLD_LACE_RGBA32 0xFDF5E6FF
/* Olive { R:128, G:128, B:0, A:255 } */
#define SG_OLIVE_RGBA32 0x808000FF
/* Olive Drab { R:107, G:142, B:35, A:255 } */
#define SG_OLIVE_DRAB_RGBA32 0x6B8E23FF
/* Orange { R:255, G:165, B:0, A:255 } */
#define SG_ORANGE_RGBA32 0xFFA500FF
/* Orange Red { R:255, G:69, B:0, A:255 } */
#define SG_ORANGE_RED_RGBA32 0xFF4500FF
/* Orchid { R:218, G:112, B:214, A:255 } */
#define SG_ORCHID_RGBA32 0xDA70D6FF
/* Pale Goldenrod { R:238, G:232, B:170, A:255 } */
#define SG_PALE_GOLDENROD_RGBA32 0xEEE8AAFF
/* Pale Green { R:152, G:251, B:152, A:255 } */
#define SG_PALE_GREEN_RGBA32 0x98FB98FF
/* Pale Turquoise { R:175, G:238, B:238, A:255 } */
#define SG_PALE_TURQUOISE_RGBA32 0xAFEEEEFF
/* Pale Violet Red { R:216, G:112, B:147, A:255 } */
#define SG_PALE_VIOLET_RED_RGBA32 0xD87093FF
/* Papaya Whip { R:255, G:239, B:213, A:255 } */
#define SG_PAPAYA_WHIP_RGBA32 0xFFEFD5FF
/* Peach Puff { R:255, G:218, B:185, A:255 } */
#define SG_PEACH_PUFF_RGBA32 0xFFDAB9FF
/* Peru { R:205, G:133, B:63, A:255 } */
#define SG_PERU_RGBA32 0xCD853FFF
/* Pink { R:255, G:192, B:203, A:255 } */
#define SG_PINK_RGBA32 0xFFC0CBFF
/* Plum { R:221, G:160, B:221, A:255 } */
#define SG_PLUM_RGBA32 0xDDA0DDFF
/* Powder Blue { R:176, G:224, B:230, A:255 } */
#define SG_POWDER_BLUE_RGBA32 0xB0E0E6FF
/* Purple { R:128, G:0, B:128, A:255 } */
#define SG_PURPLE_RGBA32 0x800080FF
/* Red { R:255, G:0, B:0, A:255 } */
#define SG_RED_RGBA32 0xFF0000FF
/* Rosy Brown { R:188, G:143, B:143, A:255 } */
#define SG_ROSY_BROWN_RGBA32 0xBC8F8FFF
/* Royal Blue { R:65, G:105, B:225, A:255 } */
#define SG_ROYAL_BLUE_RGBA32 0x4169E1FF
/* Saddle Brown { R:139, G:69, B:19, A:255 } */
#define SG_SADDLE_BROWN_RGBA32 0x8B4513FF
/* Salmon { R:250, G:128, B:114, A:255 } */
#define SG_SALMON_RGBA32 0xFA8072FF
/* Sandy Brown { R:244, G:164, B:96, A:255 } */
#define SG_SANDY_BROWN_RGBA32 0xF4A460FF
/* Sea Green { R:46, G:139, B:87, A:255 } */
#define SG_SEA_GREEN_RGBA32 0x2E8B57FF
/* Seashell { R:255, G:245, B:238, A:255 } */
#define SG_SEASHELL_RGBA32 0xFFF5EEFF
/* Sienna { R:160, G:82, B:45, A:255 } */
#define SG_SIENNA_RGBA32 0xA0522DFF
/* Silver { R:192, G:192, B:192, A:255 } */
#define SG_SILVER_RGBA32 0xC0C0C0FF
/* Sky Blue { R:135, G:206, B:235, A:255 } */
#define SG_SKY_BLUE_RGBA32 0x87CEEBFF
/* Slate Blue { R:106, G:90, B:205, A:255 } */
#define SG_SLATE_BLUE_RGBA32 0x6A5ACDFF
/* Slate Gray { R:112, G:128, B:144, A:255 } */
#define SG_SLATE_GRAY_RGBA32 0x708090FF
/* Slate Grey { R:112, G:128, B:144, A:255 } */
#define SG_SLATE_GREY_RGBA32 0x708090FF
/* Snow { R:255, G:250, B:250, A:255 } */
#define SG_SNOW_RGBA32 0xFFFAFAFF
/* Spring Green { R:0, G:255, B:127, A:255 } */
#define SG_SPRING_GREEN_RGBA32 0x00FF7FFF
/* Steel Blue { R:70, G:130, B:180, A:255 } */
#define SG_STEEL_BLUE_RGBA32 0x4682B4FF
/* Tan { R:210, G:180, B:140, A:255 } */
#define SG_TAN_RGBA32 0xD2B48CFF
/* Teal { R:0, G:128, B:128, A:255 } */
#define SG_TEAL_RGBA32 0x008080FF
/* Thistle { R:216, G:191, B:216, A:255 } */
#define SG_THISTLE_RGBA32 0xD8BFD8FF
/* Tomato { R:255, G:99, B:71, A:255 } */
#define SG_TOMATO_RGBA32 0xFF6347FF
/* Transparent { R:0, G:0, B:0, A:0 } */
#define SG_TRANSPARENT_RGBA32 0x00000000
/* Turquoise { R:64, G:224, B:208, A:255 } */
#define SG_TURQUOISE_RGBA32 0x40E0D0FF
/* Violet { R:238, G:130, B:238, A:255 } */
#define SG_VIOLET_RGBA32 0xEE82EEFF
/* Wheat { R:245, G:222, B:179, A:255 } */
#define SG_WHEAT_RGBA32 0xF5DEB3FF
/* White { R:255, G:255, B:255, A:255 } */
#define SG_WHITE_RGBA32 0xFFFFFFFF
/* White Smoke { R:245, G:245, B:245, A:255 } */
#define SG_WHITE_SMOKE_RGBA32 0xF5F5F5FF
/* Yellow { R:255, G:255, B:0, A:255 } */
#define SG_YELLOW_RGBA32 0xFFFF00FF
/* Yellow Green { R:154, G:205, B:50, A:255 } */
#define SG_YELLOW_GREEN_RGBA32 0x9ACD32FF

#endif
