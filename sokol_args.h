#pragma once
/*
    sokol_args.h    -- simple cross-platform key/value args parser

    WORK IN PROGRESS!

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)     - your own malloc() implementation (default: malloc(s))
    SOKOL_FREE(p)       - your own free() implementation (default: free(p))

    void sargs_setup(const sargs_desc* desc)
        Initialize sokol_args, desc conatins the following configuration
        parameters:
            int argc        - the main function's argc parameter
            char** argv     - the main function's argv parameter
            int max_args    - max number of key/value pairs, default is 16
            int buf_size    - size of the internal string buffer, default is 16384
        on native platform these are valid
        command line argument formats:

            key=value
            key="value"
            key='value'
            key:value
            key:"value"
            key:'value'
            key (without value)

        The value string can contain the following escape sequences:
            \\  - escape '\'
            \=  - escape '='
            \:  - escape ':'
            \"  - escape '"'
            \'  - escape '''
            \n, \r, \t  - newline, carriage return and tab
            \[space]    - escape a space character
            \[number]   - a decimal number (must fit in a byte)

        Any spaces between the end of key and the start of value are
        stripped. 
        
        On emscripten, the current page's URL args will be parsed, and the
        arguments given to sargs_setup() will be ignored.

    bool sargs_exists(const char* key)
        Test if a key arg exists.

    const char* sargs_value(const char* key)
        Return value associated with key. Returns an empty
        string ("") if the key doesn't exist, or has no value.

    const char* sargs_value_def(const char* key, const char* default)
        Return value associated with key, or the provided default
        value if the value doesn't exist or has no key.

    void sargs_shutdown(void)
        Shutdown sokol-args and free any allocated memory.

    LICENSE
    =======

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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL

#endif /* SOKOL_IMPL */
