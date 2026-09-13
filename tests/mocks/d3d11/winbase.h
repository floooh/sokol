/*
    LLM maintained.

    winbase.h -- mock replacement for the Windows SDK header of the same name.

    Declares the few kernel32 entry points which the D3D11 mock and
    tests/functional/utest.h use. The real SDK reaches these through
    libloaderapi.h and profileapi.h, both included by winbase.h.

    Prefer including windows.h. This header carries the SDK name because
    utest.h includes <winbase.h> directly on MSVC and must not reach the SDK.
*/
#ifndef MOCK_WINBASE_H_INCLUDED
#define MOCK_WINBASE_H_INCLUDED

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* dynamic-library loader stubs -- implemented by the mock.
   The Win32 names are macro-renamed: on Windows the CRT already imports the
   real GetProcAddress/FreeLibrary from kernel32, and two definitions of the
   same symbol are a link error. sokol_gfx.h calls the Win32 names, so map
   them onto the mock's own. */
#define LoadLibraryA    d3d11_mock_LoadLibraryA
#define FreeLibrary     d3d11_mock_FreeLibrary
#define GetProcAddress  d3d11_mock_GetProcAddress

extern HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName);
extern int WINAPI FreeLibrary(HMODULE hLibModule);
extern void* WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

/* high-resolution timer -- not used by sokol_gfx.h, only by utest.h. Not
   mocked: on Windows this resolves to the real kernel32 function, which the
   CRT drags in anyway. */
extern BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
extern BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_WINBASE_H_INCLUDED */
