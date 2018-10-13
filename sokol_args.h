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
    SOKOL_LOG(msg)      - your own logging functions (default: puts(msg))
    SOKOL_CALLOC(n,s)   - your own calloc() implementation (default: calloc(n,s))
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

    void sargs_shutdown(void)
        Shutdown sokol-args and free any allocated memory.

    bool sargs_isvalid(void)
        Return true between sargs_setup() and sargs_shutdown()

    bool sargs_exists(const char* key)
        Test if a key arg exists.

    const char* sargs_value(const char* key)
        Return value associated with key. Returns an empty
        string ("") if the key doesn't exist, or has no value.

    const char* sargs_value_def(const char* key, const char* default)
        Return value associated with key, or the provided default
        value if the value doesn't exist or has no key.

    int sargs_find(const char* key)
        Find argument by key name and return its index, or -1 if not found.

    int sargs_num_args(void)
        Return number of key/value pairs.

    const char* sargs_key_at(int index)
        Return the key name of argument at index. Returns empty string if
        is index is outside range.

    const char* sargs_value_at(int index)
        Return the value of argument at index. Returns empty string
        if index is outside range, or the argumnet has no value.

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

typedef struct {
    int argc;
    const char** argv;
    int max_args;
    int buf_size;
} sargs_desc;

/* setup sokol-args */
extern void sargs_setup(const sargs_desc* desc);
/* shutdown sokol-args */
extern void sargs_shutdown(void);
/* true between sargs_setup() and sargs_shutdown() */
extern bool sargs_isvalid(void);
/* test if an argument exists by key name */
extern bool sargs_exists(const char* key);
/* get value by key name, return empty string if key doesn't exist */
extern const char* sargs_value(const char* key);
/* get value by key name, return provided default if key doesn't exist */
extern const char* sargs_value_def(const char* key, const char* def);
/* get index of arg by key name, return -1 if not exists */
extern int sargs_find(const char* key);
/* get number of parsed arguments */
extern int sargs_num_args(void);
/* get key name of argument at index, or empty string */
extern const char* sargs_key_at(int index);
/* get value string of argument at index, or empty string */
extern const char* sargs_value_at(int index);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL
#include <string.h>

#ifndef SOKOL_DEBUG
    #ifdef _DEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#if !defined(SOKOL_CALLOC) && !defined(SOKOL_FREE)
    #include <stdlib.h>
#endif
#if !defined(SOKOL_CALLOC)
    #define SOKOL_CALLOC(n,s) calloc(n,s)
#endif
#if !defined(SOKOL_FREE)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG 
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#define _sargs_def(val, def) (((val) == 0) ? (def) : (val))

#define _SARGS_MAX_ARGS_DEF (16)
#define _SARGS_BUF_SIZE_DEF (16*1024)

/* parser state */
#define _SARGS_EXPECT_KEY (1<<0)
#define _SARGS_EXPECT_SEP (1<<1)
#define _SARGS_EXPECT_VAL (1<<2)
#define _SARGS_PARSING_KEY (1<<3)
#define _SARGS_PARSING_VAL (1<<4)
#define _SARGS_ERROR (1<<5)

/* a key/value pair struct */
typedef struct {
    int key;        /* index to start of key string in buf */
    int val;        /* index to start of value string in buf */
} _sargs_kvp;

/* sokol-args state */
typedef struct {
    int max_args;       /* number of key/value pairs in args array */
    int num_args;       /* number of valid items in args array */
    _sargs_kvp* args;   /* key/value pair array */
    int buf_size;       /* size of buffer in bytes */
    int buf_pos;        /* current buffer position */
    char* buf;          /* character buffer, first char is reserved and zero for 'empty string' */
    bool valid;
    uint32_t parse_state;
} _sargs_state;
static _sargs_state _sargs;

/*== PRIVATE IMPLEMENTATION FUNCTIONS ========================================*/

_SOKOL_PRIVATE void _sargs_putc(char c) {
    if ((_sargs.buf_pos+2) < _sargs.buf_size) {
        _sargs.buf[_sargs.buf_pos++] = c;
    }
}

_SOKOL_PRIVATE void _sargs_expect_key(void) {
    _sargs.parse_state = _SARGS_EXPECT_KEY;
}

_SOKOL_PRIVATE bool _sargs_key_expected(void) {
    return 0 != (_sargs.parse_state & _SARGS_EXPECT_KEY);
}

_SOKOL_PRIVATE void _sargs_expect_val(void) {
    _sargs.parse_state = _SARGS_EXPECT_VAL;
}

_SOKOL_PRIVATE bool _sargs_val_expected(void) {
    return 0 != (_sargs.parse_state & _SARGS_EXPECT_VAL);
}

_SOKOL_PRIVATE void _sargs_expect_sep(void) {
    _sargs.parse_state = _SARGS_EXPECT_SEP;
}

_SOKOL_PRIVATE bool _sargs_sep_expected(void) {
    return 0 != (_sargs.parse_state & _SARGS_EXPECT_SEP);
}

_SOKOL_PRIVATE bool _sargs_any_expected(void) {
    return 0 != (_sargs.parse_state & (_SARGS_EXPECT_KEY | _SARGS_EXPECT_VAL | _SARGS_EXPECT_SEP));
}

_SOKOL_PRIVATE bool _sargs_is_whitespace(char c) {
    return (c == ' ') || (c == '\t');
}

_SOKOL_PRIVATE bool _sargs_is_separator(char c) {
    return (c == '=') || (c == ':');
}

_SOKOL_PRIVATE void _sargs_start_key(void) {
    SOKOL_ASSERT(_sargs.num_args < _sargs.max_args);
    _sargs.parse_state = _SARGS_PARSING_KEY;
    _sargs.args[_sargs.num_args].key = _sargs.buf_pos;
}

_SOKOL_PRIVATE void _sargs_end_key(void) {
    SOKOL_ASSERT(_sargs.num_args < _sargs.max_args);
    _sargs_putc(0);
    _sargs.parse_state = 0;
}

_SOKOL_PRIVATE bool _sargs_parsing_key(void) {
    return 0 != (_sargs.parse_state & _SARGS_PARSING_KEY);
}

_SOKOL_PRIVATE void _sargs_start_val(void) {
    SOKOL_ASSERT(_sargs.num_args < _sargs.max_args);
    _sargs.parse_state = _SARGS_PARSING_VAL;
    _sargs.args[_sargs.num_args].val = _sargs.buf_pos;
}

_SOKOL_PRIVATE void _sargs_end_val(void) {
    SOKOL_ASSERT(_sargs.num_args < _sargs.max_args);
    _sargs_putc(0);
    _sargs.num_args++;
    _sargs.parse_state = 0;
}

_SOKOL_PRIVATE bool _sargs_parsing_val(void) {
    return 0 != (_sargs.parse_state & _SARGS_PARSING_VAL);
}

_SOKOL_PRIVATE bool _sargs_parse_carg(const char* src) {
    char c;
    while (0 != (c = *src++)) {
        if (_sargs_any_expected()) {
            /* find start of key or value */
            if (!_sargs_is_whitespace(c)) {
                if (_sargs_key_expected()) {
                    /* start of new key */
                    _sargs_start_key();
                }
                else if (_sargs_val_expected()) {
                    /* start of value */
                    _sargs_start_val();
                }
                else {
                    /* separator */
                    if (_sargs_is_separator(c)) {
                        _sargs_expect_val();
                        continue;
                    }
                }
            }
            else {
                /* skip white space */
                continue;
            }
        }
        else if (_sargs_parsing_key()) {
            if (_sargs_is_whitespace(c) || _sargs_is_separator(c)) {
                _sargs_end_key();
                if (_sargs_is_separator(c)) {
                    _sargs_expect_val();
                }
                else {
                    _sargs_expect_sep();
                }
                continue;
            }
        }
        else if (_sargs_parsing_val()) {
            if (_sargs_is_whitespace(c)) {
                _sargs_end_val();
                _sargs_expect_key();
                continue;
            }
        }
        if (c >= 32) {
            _sargs_putc(c);
        }
    }
    if (_sargs_parsing_key()) {
        _sargs_end_key();
        _sargs_expect_sep();
    }
    else if (_sargs_parsing_val()) {
        _sargs_end_val();
        _sargs_expect_key();
    }
    return true;
}

_SOKOL_PRIVATE bool _sargs_parse_cargs(int argc, const char** argv) {
    _sargs_expect_key();
    bool retval = true;
    for (int i = 1; i < argc; i++) {
        retval &= _sargs_parse_carg(argv[i]);
    }
    _sargs.parse_state = 0;
    return retval;
}

_SOKOL_PRIVATE const char* _sargs_str(int index) {
    SOKOL_ASSERT((index >= 0) && (index < _sargs.buf_size));
    return &_sargs.buf[index];
}

/*== PUBLIC IMPLEMENTATION FUNCTIONS =========================================*/
void sargs_setup(const sargs_desc* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sargs, 0, sizeof(_sargs));
    _sargs.max_args = _sargs_def(desc->max_args, _SARGS_MAX_ARGS_DEF);
    _sargs.buf_size = _sargs_def(desc->buf_size, _SARGS_BUF_SIZE_DEF);
    SOKOL_ASSERT(_sargs.buf_size > 8);
    _sargs.args = (_sargs_kvp*) SOKOL_CALLOC(_sargs.max_args, sizeof(_sargs_kvp));
    _sargs.buf = (char*) SOKOL_CALLOC(_sargs.buf_size, sizeof(char));
    /* the first character in buf is reserved and always zero, this is the 'empty string' */
    _sargs.buf_pos = 1;
    _sargs_parse_cargs(desc->argc, desc->argv);
    /* FIXME: parse args */
    _sargs.valid = true;
}

void sargs_shutdown(void) {
    SOKOL_ASSERT(_sargs.valid);
    if (_sargs.args) {
        SOKOL_FREE(_sargs.args);
        _sargs.args = 0;
    }
    if (_sargs.buf) {
        SOKOL_FREE(_sargs.buf);
        _sargs.buf = 0;
    }
    _sargs.valid = false;
}

bool sargs_isvalid(void) {
    return _sargs.valid;
}

int sargs_find(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    for (int i = 0; i < _sargs.num_args; i++) {
        if (0 == strcmp(_sargs_str(_sargs.args[i].key), key)) {
            return i;
        }
    }
    return -1;
}

int sargs_num_args(void) {
    SOKOL_ASSERT(_sargs.valid);
    return _sargs.num_args;
}

const char* sargs_key_at(int index) {
    SOKOL_ASSERT(_sargs.valid);
    if ((index >= 0) && (index < _sargs.num_args)) {
        return _sargs_str(_sargs.args[index].key);
    }
    else {
        /* index 0 is always the empty string */
        return _sargs_str(0);
    }
}

const char* sargs_value_at(int index) {
    SOKOL_ASSERT(_sargs.valid);
    if ((index >= 0) && (index < _sargs.num_args)) {
        return _sargs_str(_sargs.args[index].val);
    }
    else {
        /* index 0 is always the empty string */
        return _sargs_str(0);
    }
}

bool sargs_exists(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    return -1 != sargs_find(key);
}

const char* sargs_value(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    return sargs_value_at(sargs_find(key));
}

const char* sargs_value_def(const char* key, const char* def) {
    SOKOL_ASSERT(_sargs.valid && key && def);
    int arg_index = sargs_find(key);
    if (-1 != arg_index) {
        return sargs_value_at(arg_index);
    }
    else {
        return def;
    }
}

#endif /* SOKOL_IMPL */
