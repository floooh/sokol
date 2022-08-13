#if defined(SOKOL_IMPL) && !defined(SOKOL_SPINE_IMPL)
#define SOKOL_SPINE_IMPL
#endif
#ifndef SOKOL_SPINE_INCLUDED
/*
    sokol_spine.h -- a sokol-gfx renderer for the spine-c runtime
                     (see https://github.com/EsotericSoftware/spine-runtimes/tree/4.1/spine-c)

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_SPINE_IMPL

    before you include this file in *one* C or C++ file to create the
    implementation.

    The following defines are used by the implementation to select the
    platform-specific embedded shader code (these are the same defines as
    used by sokol_gfx.h and sokol_app.h):

    SOKOL_GLCORE33
    SOKOL_GLES2
    SOKOL_GLES3
    SOKOL_D3D11
    SOKOL_METAL

    ...optionally provide the following macros to override defaults:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_SPINE_API_DECL    - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_SPINE_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)
    SOKOL_LOG(msg)      - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE() - a guard macro for unreachable code (default: assert(false))

    If sokol_spine.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_SPINE_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Include the following headers before including sokol_spine.h:

        sokol_gfx.h

    Include the following headers before include the sokol_spine.h *IMPLEMENTATION*:

        spine/spine.h

    FIXME FIXME FIXME
    =================

    LICENSE
    =======
    zlib/libpng license

    Copyright (c) 2022 Andre Weissflog

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
#define SOKOL_SPINE_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t

#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before sokol_spine.h"
#endif

#if defined(SOKOL_API_DECL) && !defined(SOKOL_SPINE_API_DECL)
#define SOKOL_SPINE_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_SPINE_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_SPINE_IMPL)
#define SOKOL_SPINE_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_SPINE_API_DECL __declspec(dllimport)
#else
#define SOKOL_SPINE_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// FIXME: track events, event call back (in instance desc)

typedef struct sspine_context { uint32_t id; } sspine_context;
typedef struct sspine_atlas { uint32_t id; } sspine_atlas;
typedef struct sspine_skeleton { uint32_t id; } sspine_skeleton;
typedef struct sspine_instance { uint32_t id; } sspine_instance;

typedef struct sspine_bone { sspine_instance instance; int index; } sspine_bone;
typedef struct sspine_slot { sspine_instance instance; int index; } sspine_slot;
typedef struct sspine_anim { sspine_instance instance; int index; } sspine_anim;

typedef struct sspine_range { const void* ptr; size_t size; } sspine_range;
typedef struct sspine_vec2 { float x, y; } sspine_vec2;
typedef sg_color sspine_color;

typedef struct sspine_bone_transform {
    sspine_vec2 position;
    float rotation;         // in degrees
    sspine_vec2 scale;
    sspine_vec2 shear;      // in degrees
} sspine_bone_transform;

typedef struct sspine_context_desc {
    int max_vertices;
    int max_commands;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
} sspine_context_desc;

typedef struct sspine_image_desc {
    sg_pixel_format format;
    sg_filter min_filter;
    sg_filter mag_filter;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    int width;
    int height;
} sspine_image_desc;

typedef struct sspine_load_image_callback {
    sg_image (*callback)(const char* path, const sspine_image_desc* desc, void* user_data);
    void* user_data;
} sspine_load_image_callback;

typedef struct sspine_atlas_desc {
    sspine_range data;
    const char* dir;
    sspine_load_image_callback load_image_cb;
} sppine_atlas_desc;

typedef struct sspine_skeleton_desc {
    sspine_atlas atlas;
    const char* json_data;
    sspine_range binary_data;
} sspine_skeleton_desc;

typedef struct sspine_instance_desc {
    sspine_skeleton skeleton;
} sspine_instance_desc;

typedef struct sspine_allocator {
    void* (*alloc)(size_t size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* user_data;
} sspine_allocator;


typedef struct sspine_desc {
    int max_vertices;
    int max_commands;
    int context_pool_size;
    int atlas_pool_size;
    int skeleton_pool_size;
    int instance_pool_size;
    sg_pixel_format color_format;
    sg_pixel_format depth_format;
    int sample_count;
    sspine_allocator allocator;
} sspine_desc;

// setup/shutdown
SOKOL_SPINE_API_DECL void sspine_setup(const sspine_desc* desc);
SOKOL_SPINE_API_DECL void sspine_shutdown(void);

// context functions
SOKOL_SPINE_API_DECL sspine_context sspine_make_context(const sspine_context_desc* desc);
SOKOL_SPINE_API_DECL void sspine_destroy_context(sspine_context ctx);
SOKOL_SPINE_API_DECL void sspine_set_context(sspine_context ctx);
SOKOL_SPINE_API_DECL sspine_context sspine_get_context(void);
SOKOL_SPINE_API_DECL sspine_context sspine_default_context(void);

// create and destroy spine objects
SOKOL_SPINE_API_DECL sspine_atlas sspine_make_atlas(const sspine_atlas_desc* desc);
SOKOL_SPINE_API_DECL sspine_skeleton sspine_make_skeleton(const sspine_skeleton_desc* desc);
SOKOL_SPINE_API_DECL sspine_instance sspine_make_instance(const sspine_instance_desc* desc);
SOKOL_SPINE_API_DECL void sspine_destroy_atlas(sspine_atlas atlas);
SOKOL_SPINE_API_DECL void sspine_destroy_skeleton(sspine_skeleton skeleton);
SOKOL_SPINE_API_DECL void sspine_destroy_instance(sspine_instance instance);

// instance transform functions
SOKOL_SPINE_API_DECL void sspine_set_position(sspine_instance instance, sspine_vec2 position);
SOKOL_SPINE_API_DECL void sspine_set_scale(sspine_instance instance, sspine_vec2 scale);
SOKOL_SPINE_API_DECL void sspine_set_color(sspine_instance instance, sspine_color color);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_get_position(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_get_scale(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_color sspine_get_color(sspine_instance instance);

// find instance items by name
SOKOL_SPINE_API_DECL sspine_bone sspine_find_bone(sspine_instance instance, const char* name);
SOKOL_SPINE_API_DECL sspine_slot sspine_find_slot(sspine_instance instance, const char* name);
SOKOL_SPINE_API_DECL sspine_anim sspine_find_anim(sspine_instance instance, const char* name);

// instance animation functions
SOKOL_SPINE_API_DECL void sspine_update_animation(sspine_instance instance, float delta);
SOKOL_SPINE_API_DECL void sspine_clear_animation_tracks(sspine_instance instance);
SOKOL_SPINE_API_DECL void sspine_clear_animation_track(sspine_instance instance, int track_index);
SOKOL_SPINE_API_DECL void sspine_set_animation(sspine_instance instance, int track_index, sspine_anim anim, bool loop);
SOKOL_SPINE_API_DECL void sspine_set_animation_by_name(sspine_instance instance, int track_index, const char* anim_name, bool loop);
SOKOL_SPINE_API_DECL void sspine_add_animation(sspine_instance instance, int track_index, sspine_anim anim, bool loop, float delay);
SOKOL_SPINE_API_DECL void sspine_add_animation_by_name(sspine_instance instance, int track_index, const char* anim_name, bool loop, float delay);
SOKOL_SPINE_API_DECL void sspine_set_empty_animation(sspine_instance instance, int track_index, float mix_duration);
SOKOL_SPINE_API_DECL void sspine_add_empty_animation(sspine_instance instance, int track_index, float mix_duration, float delay);

// iterate over instance items
SOKOL_SPINE_API_DECL int sspine_num_bones(sspine_instance instance);
SOKOL_SPINE_API_DECL int sspine_num_slots(sspine_instance instance);
SOKOL_SPINE_API_DECL sspine_bone sspine_bone_at(sspine_instance instance, int index);
SOKOL_SPINE_API_DECL sspine_slot sspine_slot_at(sspine_instance instance, int index);

// check if a instanace item handle is valid
SOKOL_SPINE_API_DECL bool sspine_bone_valid(sspine_bone bone);
SOKOL_SPINE_API_DECL bool sspine_slot_valid(sspine_slot slot);

// direct bone manipulation, transforms are in bone-local space, angles are in degrees
SOKOL_SPINE_API_DECL void sspine_bone_set_transform(sspine_bone bone, const sspine_bone_transform* transform);
SOKOL_SPINE_API_DECL void sspine_bone_set_position(sspine_bone bone, sspine_vec2 position);
SOKOL_SPINE_API_DECL void sspine_bone_set_rotation(sspine_bone bone, float rotation);
SOKOL_SPINE_API_DECL void sspine_bone_set_scale(sspine_bone bone, sspine_vec2 scale);
SOKOL_SPINE_API_DECL void sspine_bone_set_shear(sspine_bone bone, sspine_vec2 shear);
SOKOL_SPINE_API_DECL sspine_bone_transform sspine_bone_get_transform(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_position(sspine_bone bone);
SOKOL_SPINE_API_DECL float sspine_bone_get_rotation(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_scale(sspine_bone bone);
SOKOL_SPINE_API_DECL sspine_vec2 sspine_bone_get_shear(sspine_bone bone);

// direct slot manipulation
SOKOL_SPINE_API_DECL void sspine_slot_set_color(sspine_slot slot, sspine_color color);
SOKOL_SPINE_API_DECL sspine_color sspine_slot_get_color(sspine_slot slot);

#ifdef __cplusplus
} // extern "C"
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef SOKOL_SPINE_IMPL
#define SOKOL_SPINE_IMPL_INCLUDED (1)

#if !defined(SPINE_SPINE_H_)
#error "Please include spine/spine.h before sokol_spine.h"
#endif


// FIXME FIXME FIXME

#endif // SOKOL_SPINE_IMPL
