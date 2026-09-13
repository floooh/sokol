/*
    LLM maintained.

    windef.h -- mock replacement for the Windows SDK header of the same name.

    Holds the basic Win32 types which the sokol_gfx.h D3D11 backend and
    tests/functional/utest.h reference. The real SDK spreads these over
    windef.h, winnt.h, winerror.h and guiddef.h; the mock folds them into one
    file because windef.h pulls in all of those anyway.

    Prefer including windows.h. This header carries the SDK name because
    utest.h includes <windef.h> directly on MSVC and must not reach the SDK.
*/
#ifndef MOCK_WINDEF_H_INCLUDED
#define MOCK_WINDEF_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* calling conventions -- keep the real __stdcall on MSVC so the COM vtable
   ABI matches; no-op on POSIX. Never redefine the __stdcall keyword on MSVC. */
#if defined(_MSC_VER)
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE __stdcall
#endif
#else
#ifndef WINAPI
#define WINAPI
#endif
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE
#endif
#ifndef __stdcall
#define __stdcall
#endif
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

/* 64-bit counter type -- not used by sokol_gfx.h, but utest.h needs it for
   the QueryPerformanceCounter() calls declared in winbase.h. */
typedef union _LARGE_INTEGER {
    struct { uint32_t LowPart; int32_t HighPart; } u;
    int64_t QuadPart;
} LARGE_INTEGER;

#ifdef __cplusplus
}
#endif

#endif /* MOCK_WINDEF_H_INCLUDED */
