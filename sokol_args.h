#if defined(SOKOL_IMPL) && !defined(SOKOL_ARGS_IMPL)
#define SOKOL_ARGS_IMPL
#endif
#ifndef SOKOL_ARGS_INCLUDED
/*
    sokol_args.h    -- cross-platform key/value arg-parsing for web and native

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_ARGS_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_LOG(msg)      - your own logging functions (default: puts(msg))
    SOKOL_CALLOC(n,s)   - your own calloc() implementation (default: calloc(n,s))
    SOKOL_FREE(p)       - your own free() implementation (default: free(p))
    SOKOL_ARGS_API_DECL - public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_ARGS_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    If sokol_args.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_ARGS_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    OVERVIEW
    ========
    sokol_args.h provides a simple unified argument parsing API for WebAssembly and
    native apps.

    When running as WebAssembly app, arguments are taken from the page URL:

    https://floooh.github.io/tiny8bit/kc85.html?type=kc85_3&mod=m022&snapshot=kc85/jungle.kcc

    The same arguments provided to a command line app:

    kc85 type=kc85_3 mod=m022 snapshot=kc85/jungle.kcc

    ARGUMENT FORMATTING
    ===================
    On the web platform, arguments must be formatted as a valid URL query string
    with 'percent encoding' used for special characters.

    Strings are expected to be UTF-8 encoded (although sokol_args.h doesn't
    contain any special UTF-8 handling). See below on how to obtain
    UTF-8 encoded argc/argv values on Windows when using WinMain() as
    entry point.

    On native platforms the following rules must be followed:

    Arguments have the general form

        key=value

    Key/value pairs are separated by 'whitespace', valid whitespace
    characters are space and tab.

    Whitespace characters in front and after the separating '=' character
    are ignored:

        key = value

    ...is the same as

        key=value

    The 'key' string must be a simple string without escape sequences or whitespace.

    Currently 'single keys' without values are not allowed, but may be
    in the future.

    The 'value' string can be quoted, and quoted value strings can contain
    whitespace:

        key = 'single-quoted value'
        key = "double-quoted value"

    Single-quoted value strings can contain double quotes, and vice-versa:

        key = 'single-quoted value "can contain double-quotes"'
        key = "double-quoted value 'can contain single-quotes'"

    Note that correct quoting can be tricky on some shells, since command
    shells may remove quotes, unless they're escaped.

    Value strings can contain a small selection of escape sequences:

        \n  - newline
        \r  - carriage return
        \t  - tab
        \\  - escaped backslash

    (more escape codes may be added in the future).

    CODE EXAMPLE
    ============

        int main(int argc, char* argv[]) {
            // initialize sokol_args with default parameters
            sargs_setup(&(sargs_desc){
                .argc = argc,
                .argv = argv
            });

            // check if a key exists...
            if (sargs_exists("bla")) {
                ...
            }

            // get value string for key, if not found, return empty string ""
            const char* val0 = sargs_value("bla");

            // get value string for key, or default string if key not found
            const char* val1 = sargs_value_def("bla", "default_value");

            // check if a key matches expected value
            if (sargs_equals("type", "kc85_4")) {
                ...
            }

            // check if a key's value is "true", "yes" or "on"
            if (sargs_boolean("joystick_enabled")) {
                ...
            }

            // iterate over keys and values
            for (int i = 0; i < sargs_num_args(); i++) {
                printf("key: %s, value: %s\n", sargs_key_at(i), sargs_value_at(i));
            }

            // lookup argument index by key string, will return -1 if key
            // is not found, sargs_key_at() and sargs_value_at() will return
            // an empty string for invalid indices
            int index = sargs_find("bla");
            printf("key: %s, value: %s\n", sargs_key_at(index), sargs_value_at(index));

            // shutdown sokol-args
            sargs_shutdown();
        }

    WINMAIN AND ARGC / ARGV
    =======================
    On Windows with WinMain() based apps, getting UTF8-encoded command line
    arguments is a bit more complicated:

    First call GetCommandLineW(), this returns the entire command line
    as UTF-16 string. Then call CommandLineToArgvW(), this parses the
    command line string into the usual argc/argv format (but in UTF-16).
    Finally convert the UTF-16 strings in argv[] into UTF-8 via
    WideCharToMultiByte().

    See the function _sapp_win32_command_line_to_utf8_argv() in sokol_app.h
    for example code how to do this (if you're using sokol_app.h, it will
    already convert the command line arguments to UTF-8 for you of course,
    so you can plug them directly into sokol_app.h).

    API DOCUMENTATION
    =================
    void sargs_setup(const sargs_desc* desc)
        Initialize sokol_args, desc contains the following configuration
        parameters:
            int argc        - the main function's argc parameter
            char** argv     - the main function's argv parameter
            int max_args    - max number of key/value pairs, default is 16
            int buf_size    - size of the internal string buffer, default is 16384

        Note that on the web, argc and argv will be ignored and the arguments
        will be taken from the page URL instead.

        sargs_setup() will allocate 2 memory chunks: one for keeping track
        of the key/value args of size 'max_args*8', and a string buffer
        of size 'buf_size'.

    void sargs_shutdown(void)
        Shutdown sokol-args and free any allocated memory.

    bool sargs_isvalid(void)
        Return true between sargs_setup() and sargs_shutdown()

    bool sargs_exists(const char* key)
        Test if a key arg exists.

    const char* sargs_value(const char* key)
        Return value associated with key. Returns an empty
        string ("") if the key doesn't exist.

    const char* sargs_value_def(const char* key, const char* default)
        Return value associated with key, or the provided default
        value if the value doesn't exist.

    bool sargs_equals(const char* key, const char* val);
        Return true if the value associated with key matches
        the 'val' argument.

    bool sargs_boolean(const char* key)
        Return true if the value string of 'key' is one
        of 'true', 'yes', 'on'.

    int sargs_find(const char* key)
        Find argument by key name and return its index, or -1 if not found.

    int sargs_num_args(void)
        Return number of key/value pairs.

    const char* sargs_key_at(int index)
        Return the key name of argument at index. Returns empty string if
        is index is outside range.

    const char* sargs_value_at(int index)
        Return the value of argument at index. Returns empty string
        if index is outside range.

    TODO
    ====
    - parsing errors?

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
#define SOKOL_ARGS_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_ARGS_API_DECL)
#define SOKOL_ARGS_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_ARGS_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_ARGS_IMPL)
#define SOKOL_ARGS_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_ARGS_API_DECL __declspec(dllimport)
#else
#define SOKOL_ARGS_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sargs_desc {
    int argc;
    char** argv;
    int max_args;
    int buf_size;
} sargs_desc;

/* setup sokol-args */
SOKOL_ARGS_API_DECL void sargs_setup(const sargs_desc* desc);
/* shutdown sokol-args */
SOKOL_ARGS_API_DECL void sargs_shutdown(void);
/* true between sargs_setup() and sargs_shutdown() */
SOKOL_ARGS_API_DECL bool sargs_isvalid(void);
/* test if an argument exists by key name */
SOKOL_ARGS_API_DECL bool sargs_exists(const char* key);
/* get value by key name, return empty string if key doesn't exist */
SOKOL_ARGS_API_DECL const char* sargs_value(const char* key);
/* get value by key name, return provided default if key doesn't exist */
SOKOL_ARGS_API_DECL const char* sargs_value_def(const char* key, const char* def);
/* return true if val arg matches the value associated with key */
SOKOL_ARGS_API_DECL bool sargs_equals(const char* key, const char* val);
/* return true if key's value is "true", "yes" or "on" */
SOKOL_ARGS_API_DECL bool sargs_boolean(const char* key);
/* get index of arg by key name, return -1 if not exists */
SOKOL_ARGS_API_DECL int sargs_find(const char* key);
/* get number of parsed arguments */
SOKOL_ARGS_API_DECL int sargs_num_args(void);
/* get key name of argument at index, or empty string */
SOKOL_ARGS_API_DECL const char* sargs_key_at(int index);
/* get value string of argument at index, or empty string */
SOKOL_ARGS_API_DECL const char* sargs_value_at(int index);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */
inline void sargs_setup(const sargs_desc& desc) { return sargs_setup(&desc); }

#endif
#endif // SOKOL_ARGS_INCLUDED

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_ARGS_IMPL
#define SOKOL_ARGS_IMPL_INCLUDED (1)
#include <string.h> /* memset, strcmp */

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
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
    #if defined(__GNUC__) || defined(__clang__)
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
} _sargs_kvp_t;

/* sokol-args state */
typedef struct {
    int max_args;       /* number of key/value pairs in args array */
    int num_args;       /* number of valid items in args array */
    _sargs_kvp_t* args;   /* key/value pair array */
    int buf_size;       /* size of buffer in bytes */
    int buf_pos;        /* current buffer position */
    char* buf;          /* character buffer, first char is reserved and zero for 'empty string' */
    bool valid;
    uint32_t parse_state;
    char quote;         /* current quote char, 0 if not in a quote */
    bool in_escape;     /* currently in an escape sequence */
} _sargs_state_t;
static _sargs_state_t _sargs;

/*== PRIVATE IMPLEMENTATION FUNCTIONS ========================================*/

_SOKOL_PRIVATE void _sargs_putc(char c) {
    if ((_sargs.buf_pos+2) < _sargs.buf_size) {
        _sargs.buf[_sargs.buf_pos++] = c;
    }
}

_SOKOL_PRIVATE const char* _sargs_str(int index) {
    SOKOL_ASSERT((index >= 0) && (index < _sargs.buf_size));
    return &_sargs.buf[index];
}

/*-- argument parser functions ------------------*/
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

_SOKOL_PRIVATE bool _sargs_any_expected(void) {
    return 0 != (_sargs.parse_state & (_SARGS_EXPECT_KEY | _SARGS_EXPECT_VAL | _SARGS_EXPECT_SEP));
}

_SOKOL_PRIVATE bool _sargs_is_separator(char c) {
    return c == '=';
}

_SOKOL_PRIVATE bool _sargs_is_quote(char c) {
    if (0 == _sargs.quote) {
        return (c == '\'') || (c == '"');
    }
    else {
        return c == _sargs.quote;
    }
}

_SOKOL_PRIVATE void _sargs_begin_quote(char c) {
    _sargs.quote = c;
}

_SOKOL_PRIVATE void _sargs_end_quote(void) {
    _sargs.quote = 0;
}

_SOKOL_PRIVATE bool _sargs_in_quotes(void) {
    return 0 != _sargs.quote;
}

_SOKOL_PRIVATE bool _sargs_is_whitespace(char c) {
    return !_sargs_in_quotes() && ((c == ' ') || (c == '\t'));
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

_SOKOL_PRIVATE bool _sargs_is_escape(char c) {
    return '\\' == c;
}

_SOKOL_PRIVATE void _sargs_start_escape(void) {
    _sargs.in_escape = true;
}

_SOKOL_PRIVATE bool _sargs_in_escape(void) {
    return _sargs.in_escape;
}

_SOKOL_PRIVATE char _sargs_escape(char c) {
    switch (c) {
        case 'n':   return '\n';
        case 't':   return '\t';
        case 'r':   return '\r';
        case '\\':  return '\\';
        default:    return c;
    }
}

_SOKOL_PRIVATE void _sargs_end_escape(void) {
    _sargs.in_escape = false;
}

_SOKOL_PRIVATE bool _sargs_parsing_val(void) {
    return 0 != (_sargs.parse_state & _SARGS_PARSING_VAL);
}

_SOKOL_PRIVATE bool _sargs_parse_carg(const char* src) {
    char c;
    while (0 != (c = *src++)) {
        if (_sargs_in_escape()) {
            c = _sargs_escape(c);
            _sargs_end_escape();
        }
        else if (_sargs_is_escape(c)) {
            _sargs_start_escape();
            continue;
        }
        if (_sargs_any_expected()) {
            if (!_sargs_is_whitespace(c)) {
                /* start of key, value or separator */
                if (_sargs_key_expected()) {
                    /* start of new key */
                    _sargs_start_key();
                }
                else if (_sargs_val_expected()) {
                    /* start of value */
                    if (_sargs_is_quote(c)) {
                        _sargs_begin_quote(c);
                        continue;
                    }
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
                /* end of key string */
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
            if (_sargs_in_quotes()) {
                /* when in quotes, whitespace is a normal character
                   and a matching quote ends the value string
                */
                if (_sargs_is_quote(c)) {
                    _sargs_end_quote();
                    _sargs_end_val();
                    _sargs_expect_key();
                    continue;
                }
            }
            else if (_sargs_is_whitespace(c)) {
                /* end of value string (no quotes) */
                _sargs_end_val();
                _sargs_expect_key();
                continue;
            }
        }
        _sargs_putc(c);
    }
    if (_sargs_parsing_key()) {
        _sargs_end_key();
        _sargs_expect_sep();
    }
    else if (_sargs_parsing_val() && !_sargs_in_quotes()) {
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

/*-- EMSCRIPTEN IMPLEMENTATION -----------------------------------------------*/
#if defined(__EMSCRIPTEN__)

#ifdef __cplusplus
extern "C" {
#endif
EMSCRIPTEN_KEEPALIVE void _sargs_add_kvp(const char* key, const char* val) {
    SOKOL_ASSERT(_sargs.valid && key && val);
    if (_sargs.num_args >= _sargs.max_args) {
        return;
    }

    /* copy key string */
    char c;
    _sargs.args[_sargs.num_args].key = _sargs.buf_pos;
    const char* ptr = key;
    while (0 != (c = *ptr++)) {
        _sargs_putc(c);
    }
    _sargs_putc(0);

    /* copy value string */
    _sargs.args[_sargs.num_args].val = _sargs.buf_pos;
    ptr = val;
    while (0 != (c = *ptr++)) {
        _sargs_putc(c);
    }
    _sargs_putc(0);

    _sargs.num_args++;
}
#ifdef __cplusplus
} /* extern "C" */
#endif

/* JS function to extract arguments from the page URL */
EM_JS(void, sargs_js_parse_url, (void), {
    var params = new URLSearchParams(window.location.search).entries();
    for (var p = params.next(); !p.done; p = params.next()) {
        var key = p.value[0];
        var val = p.value[1];
        var res = ccall('_sargs_add_kvp', 'void', ['string','string'], [key,val]);
    }
});

#endif /* EMSCRIPTEN */

/*== PUBLIC IMPLEMENTATION FUNCTIONS =========================================*/
SOKOL_API_IMPL void sargs_setup(const sargs_desc* desc) {
    SOKOL_ASSERT(desc);
    memset(&_sargs, 0, sizeof(_sargs));
    _sargs.max_args = _sargs_def(desc->max_args, _SARGS_MAX_ARGS_DEF);
    _sargs.buf_size = _sargs_def(desc->buf_size, _SARGS_BUF_SIZE_DEF);
    SOKOL_ASSERT(_sargs.buf_size > 8);
    _sargs.args = (_sargs_kvp_t*) SOKOL_CALLOC((size_t)_sargs.max_args, sizeof(_sargs_kvp_t));
    _sargs.buf = (char*) SOKOL_CALLOC((size_t)_sargs.buf_size, sizeof(char));
    /* the first character in buf is reserved and always zero, this is the 'empty string' */
    _sargs.buf_pos = 1;
    _sargs.valid = true;

    /* parse argc/argv */
    _sargs_parse_cargs(desc->argc, (const char**) desc->argv);

    #if defined(__EMSCRIPTEN__)
        /* on emscripten, also parse the page URL*/
        sargs_js_parse_url();
    #endif
}

SOKOL_API_IMPL void sargs_shutdown(void) {
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

SOKOL_API_IMPL bool sargs_isvalid(void) {
    return _sargs.valid;
}

SOKOL_API_IMPL int sargs_find(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    for (int i = 0; i < _sargs.num_args; i++) {
        if (0 == strcmp(_sargs_str(_sargs.args[i].key), key)) {
            return i;
        }
    }
    return -1;
}

SOKOL_API_IMPL int sargs_num_args(void) {
    SOKOL_ASSERT(_sargs.valid);
    return _sargs.num_args;
}

SOKOL_API_IMPL const char* sargs_key_at(int index) {
    SOKOL_ASSERT(_sargs.valid);
    if ((index >= 0) && (index < _sargs.num_args)) {
        return _sargs_str(_sargs.args[index].key);
    }
    else {
        /* index 0 is always the empty string */
        return _sargs_str(0);
    }
}

SOKOL_API_IMPL const char* sargs_value_at(int index) {
    SOKOL_ASSERT(_sargs.valid);
    if ((index >= 0) && (index < _sargs.num_args)) {
        return _sargs_str(_sargs.args[index].val);
    }
    else {
        /* index 0 is always the empty string */
        return _sargs_str(0);
    }
}

SOKOL_API_IMPL bool sargs_exists(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    return -1 != sargs_find(key);
}

SOKOL_API_IMPL const char* sargs_value(const char* key) {
    SOKOL_ASSERT(_sargs.valid && key);
    return sargs_value_at(sargs_find(key));
}

SOKOL_API_IMPL const char* sargs_value_def(const char* key, const char* def) {
    SOKOL_ASSERT(_sargs.valid && key && def);
    int arg_index = sargs_find(key);
    if (-1 != arg_index) {
        return sargs_value_at(arg_index);
    }
    else {
        return def;
    }
}

SOKOL_API_IMPL bool sargs_equals(const char* key, const char* val) {
    SOKOL_ASSERT(_sargs.valid && key && val);
    return 0 == strcmp(sargs_value(key), val);
}

SOKOL_API_IMPL bool sargs_boolean(const char* key) {
    const char* val = sargs_value(key);
    return (0 == strcmp("true", val)) ||
           (0 == strcmp("yes", val)) ||
           (0 == strcmp("on", val));
}

#endif /* SOKOL_ARGS_IMPL */
