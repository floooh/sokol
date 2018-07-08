#pragma once
/*
    sokol_audio.h   -- minimalist cross-platform buffer-streaming audio API

    THIS IS HIGHLY EXPERIMENTAL AND WON'T BE FINISHED FOR A WHILE, 
    DON'T USE!

    In the beginning this is mainly a testbed to get the simplest possible
    glitch-free audio solution for WASM/asm.js, and to have that simplest-
    possible API for other platforms too.

    On non-emscripten platforms, essentially use the SoLoud backend
    code. If the minimal emscripten buffer-streaming works well,
    try to get a PR into SoLoud instead of the current 
    SDL-static backend (which has a fairly big JS shim).

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
    int sample_rate;
    int buffer_size;
    int num_channels;   /* 1 or 2 */
    void (*stream_cb)(float* buffer, int num_samples);
} saudio_desc;

extern void saudio_setup(const saudio_desc* desc);
extern void saudio_shutdown(void);
extern bool saudio_isvalid(void);
extern int saudio_sample_rate(void);    /* actual sample rate */
extern int saudio_buffer_size(void);    /* actual buffer size */
extern int saudio_num_channels(void);   /* actual number of channels */

#ifdef __cplusplus
} /* extern "C" */
#endif
