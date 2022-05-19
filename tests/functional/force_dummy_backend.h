#if defined(SOKOL_GLES2)
#undef SOKOL_GLES2
#endif
#if defined(SOKOL_GLES3)
#undef SOKOL_GLES3
#endif
#if defined(SOKOL_GLCORE33)
#undef SOKOL_GLCORE33
#endif
#if defined(SOKOL_METAL)
#undef SOKOL_METAL
#endif
#if defined(SOKOL_D3D11)
#undef SOKOL_D3D11
#endif
#if defined(SOKOL_WGPU)
#undef SOKOL_WGPU
#endif
#define SOKOL_DUMMY_BACKEND

