#include "sokol_app.h"

#if defined(SOKOL_DUMMY_BACKEND)
int main() {
    return 0;
}
#else
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){0};
}
#endif
