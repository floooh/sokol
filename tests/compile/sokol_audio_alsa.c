#define SOKOL_IMPL
#define SAUDIO_USE_ALSA
#include "sokol_audio.h"

void use_audio_impl(void) {
    saudio_setup(&(saudio_desc){0});
}

int main(void) {
    return 0;
}
