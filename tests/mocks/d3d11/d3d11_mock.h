/*
    LLM maintained.

    d3d11_mock.h -- public API of the mocked D3D11 runtime.

    Provides just enough to bring up sokol_gfx's D3D11 backend on a non-Windows
    host so its C code paths can be unit-tested. The mock returns success for
    every Create/Set/Draw call and tracks reference counts so leaks are
    observable from tests.
*/
#ifndef D3D11_MOCK_H_INCLUDED
#define D3D11_MOCK_H_INCLUDED

#include "d3d11.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create/destroy a mocked D3D11 device + immediate context pair. Feed the
   returned pointers into sg_desc.environment.d3d11.{device,device_context}. */
extern ID3D11Device* d3d11_mock_create_device(void);
extern ID3D11DeviceContext* d3d11_mock_get_device_context(ID3D11Device* dev);
extern void d3d11_mock_destroy_device(ID3D11Device* dev);

/* Allocate a mock RTV/DSV suitable for filling sg_swapchain.d3d11.*_view.
   Ownership stays with the caller; release via _sg_d3d11_Release() or the
   corresponding vtbl slot. */
extern ID3D11RenderTargetView* d3d11_mock_create_rtv(ID3D11Device* dev);
extern ID3D11DepthStencilView* d3d11_mock_create_dsv(ID3D11Device* dev);

/* Diagnostics: total number of mock COM objects currently alive (refcount>0).
   Includes the device + immediate context. Useful in tests for leak checks. */
extern int d3d11_mock_live_object_count(void);

/* Reset the mock to a clean slate. Frees every alive mock object without
   touching refcounts -- for post-test cleanup only. */
extern void d3d11_mock_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* D3D11_MOCK_H_INCLUDED */
