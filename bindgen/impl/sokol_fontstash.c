// LLM maintained.
#include "sokol_defines.h"
#include "sokol_gfx.h"
#if defined(IMPL)
#define SOKOL_FONTSTASH_IMPL
#define FONTSTASH_IMPLEMENTATION
#include <stdlib.h>
#include "sokol_gl.h"
#include "fontstash.h"
#else
typedef struct FONScontext FONScontext;
#endif
#include "sokol_fontstash.h"
