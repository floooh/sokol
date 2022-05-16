#define SOKOL_IMPL
#include "sokol_audio.h"

void use_audio_impl(void) {
    saudio_setup(&(saudio_desc){0});
}
