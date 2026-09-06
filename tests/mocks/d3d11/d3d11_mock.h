/*
    LLM maintained.

    d3d11_mock.h -- public API of the mocked D3D11 runtime.
*/
#ifndef D3D11_MOCK_H_INCLUDED
#define D3D11_MOCK_H_INCLUDED

#include "d3d11.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Create/destroy a mocked D3D11 device + immediate context pair. Feed the
   returned pointers into sg_desc.environment.d3d11.{device,device_context}. */
extern ID3D11Device* d3d11_mock_create_device(void);
extern ID3D11DeviceContext* d3d11_mock_get_device_context(ID3D11Device* dev);
extern void d3d11_mock_destroy_device(ID3D11Device* dev);

/* Allocate a mock RTV/DSV suitable for filling sg_swapchain.d3d11.*_view.
   Ownership stays with the caller. */
extern ID3D11RenderTargetView* d3d11_mock_create_rtv(ID3D11Device* dev);
extern ID3D11DepthStencilView* d3d11_mock_create_dsv(ID3D11Device* dev);

/* Total number of mock COM objects currently alive (refcount>0). Useful in
   tests for leak checks. */
extern int d3d11_mock_live_object_count(void);

/* Reset the mock to a clean slate. */
extern void d3d11_mock_reset(void);

/* Fault injection -- when the counter is > 0, the next N Create* / D3DCompile
   calls return E_FAIL / NULL. The counter decrements per call. */
extern void d3d11_mock_fail_next_create(int n);
extern void d3d11_mock_fail_next_compile(int n);
extern void d3d11_mock_fail_d3dcompiler_dll(bool fail);

#ifdef __cplusplus
}
#endif

#endif /* D3D11_MOCK_H_INCLUDED */
