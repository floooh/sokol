#if !defined(__ANDROID__)
    #define SOKOL_NO_ENTRY
#endif
#if defined(_WIN32)
    #define SOKOL_WIN32_FORCE_MAIN
#endif
// FIXME: macOS Zig HACK without this, some C stdlib headers throw errors
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
