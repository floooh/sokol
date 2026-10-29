#define SOKOL_IMPL
#define SAUDIO_USE_ALSA
#include "sokol_audio.h"

void use_audio_impl() {
    saudio_setup({});
}

int main() {
    return 0;
}
