/*
    LLM maintained.

    d3dcompiler.h -- mock replacement for the Windows d3dcompiler.h.
    Provides just the pD3DCompile typedef and the D3DCOMPILE_* flag constants
    referenced by sokol_gfx.h. The runtime implementation is dispatched via
    LoadLibraryA("d3dcompiler_47.dll") + GetProcAddress("D3DCompile") — both
    supplied by the mock.
*/
#ifndef MOCK_D3DCOMPILER_H_INCLUDED
#define MOCK_D3DCOMPILER_H_INCLUDED

/* d3d11.h must have been included by sokol_gfx.h before this header,
   so ID3DBlob, D3D_SHADER_MACRO, ID3DInclude etc. are already visible. */
#include "d3d11.h"

#ifdef __cplusplus
extern "C" {
#endif

/* opaque include-handler pointer -- passed by sokol_gfx.h as
   D3D_COMPILE_STANDARD_FILE_INCLUDE which is treated as an ID3DInclude*. */
typedef struct ID3DInclude ID3DInclude;

/* opaque shader macro list -- sokol_gfx.h always passes NULL. */
typedef struct _D3D_SHADER_MACRO D3D_SHADER_MACRO;

/* magic sentinel used as pInclude to enable the built-in file loader.
   The mock treats it as an opaque flag; behaviour is a no-op. */
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(intptr_t)1)

/* Flags1 bits actually referenced by sokol_gfx.h. Values must match the
   real d3dcompiler flags so mixed builds behave the same. */
#define D3DCOMPILE_DEBUG                        (1u << 0)
#define D3DCOMPILE_SKIP_OPTIMIZATION            (1u << 2)
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR     (1u << 4)
#define D3DCOMPILE_OPTIMIZATION_LEVEL3          ((1u << 14) | (1u << 15))

/* D3DCompile function-pointer type, matching the real d3dcompiler ABI. */
typedef HRESULT (WINAPI *pD3DCompile)(
    LPCVOID                 pSrcData,
    SIZE_T                  SrcDataSize,
    LPCSTR                  pSourceName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude*            pInclude,
    LPCSTR                  pEntrypoint,
    LPCSTR                  pTarget,
    UINT                    Flags1,
    UINT                    Flags2,
    ID3DBlob**              ppCode,
    ID3DBlob**              ppErrorMsgs);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_D3DCOMPILER_H_INCLUDED */
