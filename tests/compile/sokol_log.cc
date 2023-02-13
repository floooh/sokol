#define SOKOL_IMPL
#include "sokol_log.h"

void use_sokol_log(void) {
    slog_func("bla", 1, 123, "123", 42, "bla.c", 0);
}
