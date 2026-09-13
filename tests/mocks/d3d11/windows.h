/*
    LLM maintained.

    windows.h -- mock replacement for the Windows SDK umbrella header. Like the
    real one it just pulls in windef.h and winbase.h, which hold the types and
    kernel32 declarations the sokol_gfx.h D3D11 backend needs.

    All three shadow the Windows SDK on Windows hosts, so the D3D11 backend
    tests build and run without the SDK headers and without linking
    d3d11.lib / dxgi.lib.
*/
#ifndef MOCK_WINDOWS_H_INCLUDED
#define MOCK_WINDOWS_H_INCLUDED

#include "windef.h"
#include "winbase.h"

#endif /* MOCK_WINDOWS_H_INCLUDED */
