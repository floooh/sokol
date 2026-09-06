/*
    LLM maintained.

    windows.h -- minimal Windows type/function stubs required by the D3D11
    mock. Only what sokol_gfx.h references in the D3D11 backend.

    On the real Windows platform, this header should not be reached because
    the real Windows SDK provides d3d11.h/d3dcompiler.h. This header is
    only intended for non-Windows hosts (Linux, macOS) to run the sokol-gfx
    D3D11 backend tests.
*/
#ifndef MOCK_WINDOWS_H_INCLUDED
#define MOCK_WINDOWS_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* calling conventions -- no-op on POSIX */
#ifndef WINAPI
#define WINAPI
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE
#endif
#ifndef __stdcall
#define __stdcall
#endif

/* basic integer/float types */
typedef int32_t  INT;
typedef uint32_t UINT;
typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint64_t UINT64;
typedef int32_t  LONG;
typedef uint32_t ULONG;
typedef uint64_t ULONG64;
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  BOOL;
typedef float    FLOAT;
typedef size_t   SIZE_T;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* string/pointer types */
typedef void*        LPVOID;
typedef const void*  LPCVOID;
typedef char*        LPSTR;
typedef const char*  LPCSTR;
typedef wchar_t*     LPWSTR;
typedef const wchar_t* LPCWSTR;

/* handle types (opaque) */
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HWND;

/* HRESULT status codes used by sokol_gfx */
typedef int32_t HRESULT;
#define S_OK            ((HRESULT)0)
#define S_FALSE         ((HRESULT)1)
#define E_FAIL          ((HRESULT)0x80004005)
#define E_INVALIDARG    ((HRESULT)0x80070057)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000E)
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) <  0)

/* GUID / IID */
typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID;
typedef GUID IID;
typedef const GUID* REFGUID;
typedef const IID* REFIID;

/* generic function pointer (used by sokol_gfx.h's GL path, harmless here) */
typedef int (WINAPI *PROC)(void);

/* dynamic-library loader stubs -- implemented by the mock */
extern HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName);
extern int WINAPI FreeLibrary(HMODULE hLibModule);
extern void* WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_WINDOWS_H_INCLUDED */
