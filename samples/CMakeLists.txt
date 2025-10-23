add_executable(triangle-bufferless-sapp sapp_turbo/triangle-bufferless-sapp.c)
target_compile_definitions(triangle-bufferless-sapp PRIVATE SOKOL_GLCORE=1)
target_link_libraries(triangle-bufferless-sapp PRIVATE sokol_turbo)