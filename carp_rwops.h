
#ifndef CARP_RWOPS_INCLUDED
#define CARP_RWOPS_INCLUDED (1)

#ifdef __ANDROID__
#include "sokol/sokol_app.h"
#endif

#include <stdio.h>
#include <stdlib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
	
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
	
/* RWops Types */
#define CARP_RWOPS_UNKNOWN   0U  /**< Unknown stream type */
#define CARP_RWOPS_WINFILE   1U  /**< Win32 file */
#define CARP_RWOPS_STDFILE   2U  /**< Stdio file */
#define CARP_RWOPS_JNIFILE   3U  /**< Android asset */
#define CARP_RWOPS_MEMORY    4U  /**< Memory stream */
#define CARP_RWOPS_MEMORY_RO 5U  /**< Read-Only memory stream */

/**
 * This is the read/write operation structure -- very basic.
 */
typedef struct CARP_RWops
{
    /**
     *  Return the size of the file in this rwops, or -1 if unknown
     */
    long long(* size) (struct CARP_RWops* context);

    /**
     *  Seek to \c offset relative to \c whence, one of stdio's whence values:
     *  CARP_RW_SEEK_SET, CARP_RW_SEEK_CUR, CARP_RW_SEEK_END
     *
     *  \return the final offset in the data stream, or -1 on error.
     */
    long long(* seek) (struct CARP_RWops* context, long long offset, int whence);

    /**
     *  Read up to \c maxnum objects each of size \c size from the data
     *  stream to the area pointed at by \c ptr.
     *
     *  \return the number of objects read, or 0 at error or end of file.
     */
    size_t(* read) (struct CARP_RWops* context, void* ptr, size_t size, size_t maxnum);

    /**
     *  Write exactly \c num objects each of size \c size from the area
     *  pointed at by \c ptr to data stream.
     *
     *  \return the number of objects written, or 0 at error or end of file.
     */
    size_t(* write) (struct CARP_RWops* context, const void* ptr, size_t size, size_t num);

    /**
     *  Close and free an allocated CARP_RWops structure.
     *
     *  \return 0 if successful or -1 on write error when flushing data.
     */
    int (* close) (struct CARP_RWops* context);

    unsigned int type;
    union
    {
#ifdef __ANDROID__
        struct
        {
            void* asset;
		} androidio;
#endif
        struct
        {
            bool autoclose;
            FILE* fp;
        } stdio;
        struct
        {
            unsigned char* base;
            unsigned char* here;
            unsigned char* stop;
		} mem;
        struct
        {
            void* data1;
            void* data2;
        } unknown;
	} hidden;
} CARP_RWops;

extern const char* CARP_GetInternalDataPath();
extern const char* CARP_GetExternalDataPath();

extern CARP_RWops* CARP_RWFromFile(const char* file, const char* mode, int only_assets);
extern CARP_RWops* CARP_RWFromFP(FILE* fp, int autoclose);
extern CARP_RWops* CARP_RWFromMem(void* mem, int size);

extern CARP_RWops*  CARP_AllocRW(void);
extern void  CARP_FreeRW(CARP_RWops* area);

#define CARP_RW_SEEK_SET 0       /**< Seek from the beginning of data */
#define CARP_RW_SEEK_CUR 1       /**< Seek relative to current read point */
#define CARP_RW_SEEK_END 2       /**< Seek relative to the end of data */

/**
 *  Return the size of the file in this rwops, or -1 if unknown
 */
extern long long  CARP_RWsize(CARP_RWops* context);

/**
 *  Seek to \c offset relative to \c whence, one of stdio's whence values:
 *  CARP_RW_SEEK_SET, CARP_RW_SEEK_CUR, CARP_RW_SEEK_END
 *
 *  \return the final offset in the data stream, or -1 on error.
 */
extern long long  CARP_RWseek(CARP_RWops* context, long long offset, int whence);

/**
 *  Return the current offset in the data stream, or -1 on error.
 */
extern long long  CARP_RWtell(CARP_RWops* context);

/**
 *  Read up to \c maxnum objects each of size \c size from the data
 *  stream to the area pointed at by \c ptr.
 *
 *  \return the number of objects read, or 0 at error or end of file.
 */
extern size_t  CARP_RWread(CARP_RWops* context, void* ptr, size_t size, size_t maxnum);

/**
 *  Write exactly \c num objects each of size \c size from the area
 *  pointed at by \c ptr to data stream.
 *
 *  \return the number of objects written, or 0 at error or end of file.
 */
extern size_t  CARP_RWwrite(CARP_RWops* context, const void* ptr, size_t size, size_t num);

/**
 *  Close and free an allocated CARP_RWops structure.
 *
 *  \return 0 if successful or -1 on write error when flushing data.
 */
extern int CARP_RWclose(CARP_RWops* context);

#ifdef __cplusplus
}
#endif
#endif

#ifdef CARP_RWOPS_IMPL
#ifndef CARP_RWOPS_IMPL_INCLUDE
#define CARP_RWOPS_IMPL_INCLUDE

#ifdef _WIN32
#include <Windows.h>
#endif

long long _carp_stdio_size(CARP_RWops* context)
{
    long long pos, size;
    pos = CARP_RWseek(context, 0, CARP_RW_SEEK_CUR);
    if (pos < 0) {
        return -1;
    }
    size = CARP_RWseek(context, 0, CARP_RW_SEEK_END);
    CARP_RWseek(context, pos, CARP_RW_SEEK_SET);
    return size;
}

long long _carp_stdio_seek(CARP_RWops* context, long long offset, int whence)
{
    int stdiowhence;

    switch (whence) {
    case CARP_RW_SEEK_SET:
        stdiowhence = SEEK_SET;
        break;
    case CARP_RW_SEEK_CUR:
        stdiowhence = SEEK_CUR;
        break;
    case CARP_RW_SEEK_END:
        stdiowhence = SEEK_END;
        break;
    default:
        return -1;
    }

    if (fseek(context->hidden.stdio.fp, (long)offset, stdiowhence) == 0) {
        long long pos = ftell(context->hidden.stdio.fp);
        if (pos < 0) {
            return -1;
        }
        return pos;
    }
    return -1;
}

size_t _carp_stdio_read(CARP_RWops* context, void* ptr, size_t size, size_t maxnum)
{
    return fread(ptr, size, maxnum, context->hidden.stdio.fp);
}

size_t _carp_stdio_write(CARP_RWops* context, const void* ptr, size_t size, size_t num)
{
    return fwrite(ptr, size, num, context->hidden.stdio.fp);
}

int _carp_stdio_close(CARP_RWops* context)
{
    int status = 0;
    if (context) {
        if (context->hidden.stdio.autoclose) {
            /* WARNING:  Check the return value here! */
            if (fclose(context->hidden.stdio.fp) != 0) {
                status = -1;
            }
        }
        CARP_FreeRW(context);
    }
    return status;
}

long long _carp_mem_size(CARP_RWops* context)
{
    return (long long)(context->hidden.mem.stop - context->hidden.mem.base);
}

long long _carp_mem_seek(CARP_RWops* context, long long offset, int whence)
{
    unsigned char* newpos;

    switch (whence) {
    case CARP_RW_SEEK_SET:
        newpos = context->hidden.mem.base + offset;
        break;
    case CARP_RW_SEEK_CUR:
        newpos = context->hidden.mem.here + offset;
        break;
    case CARP_RW_SEEK_END:
        newpos = context->hidden.mem.stop + offset;
        break;
    default:
        return -1;
    }
    if (newpos < context->hidden.mem.base) {
        newpos = context->hidden.mem.base;
    }
    if (newpos > context->hidden.mem.stop) {
        newpos = context->hidden.mem.stop;
    }
    context->hidden.mem.here = newpos;
    return (long long)(context->hidden.mem.here - context->hidden.mem.base);
}

size_t _carp_mem_read(CARP_RWops* context, void* ptr, size_t size, size_t maxnum)
{
    size_t total_bytes;
    size_t mem_available;

    total_bytes = (maxnum * size);
    if ((maxnum <= 0) || (size <= 0)
        || ((total_bytes / maxnum) != size)) {
        return 0;
    }

    mem_available = (context->hidden.mem.stop - context->hidden.mem.here);
    if (total_bytes > mem_available) {
        total_bytes = mem_available;
    }

    memcpy(ptr, context->hidden.mem.here, total_bytes);
    context->hidden.mem.here += total_bytes;

    return (total_bytes / size);
}

size_t _carp_mem_write(CARP_RWops* context, const void* ptr, size_t size, size_t num)
{
    if ((context->hidden.mem.here + (num * size)) > context->hidden.mem.stop) {
        num = (context->hidden.mem.stop - context->hidden.mem.here) / size;
    }
    memcpy(context->hidden.mem.here, ptr, num * size);
    context->hidden.mem.here += num * size;
    return num;
}

int _carp_mem_close(CARP_RWops* context)
{
    if (context) {
        CARP_FreeRW(context);
    }
    return 0;
}

#ifdef __ANDROID__

int _carp_Android_JNI_FileOpen(CARP_RWops* ctx, const char* fileName, const char* mode)
{
    AAsset* asset = NULL;
    ctx->hidden.androidio.asset = NULL;

    ANativeActivity* activity = (ANativeActivity*)sapp_android_get_native_activity();
    if (activity == NULL) return -1;
    if (activity->assetManager == NUL) return -1;
	
    asset = AAssetManager_open(activity->assetManager, fileName, AASSET_MODE_UNKNOWN);
    if (asset == NULL) {
        return -1;
    }

    ctx->hidden.androidio.asset = (void*)asset;
    return 0;
}

size_t _carp_Android_JNI_FileRead(CARP_RWops* ctx, void* buffer, size_t size, size_t maxnum)
{
    size_t result;
    AAsset* asset = (AAsset*)ctx->hidden.androidio.asset;
    result = AAsset_read(asset, buffer, size * maxnum);

    if (result > 0) {
        /* Number of chuncks */
        return (result / size);
    }
    else {
        /* Error or EOF */
        return result;
    }
}

size_t _carp_Android_JNI_FileWrite(CARP_RWops* ctx, const void* buffer, size_t size, size_t num)
{
    // "Cannot write to Android package filesystem";
    return 0;
}

long long _carp_Android_JNI_FileSize(CARP_RWops* ctx)
{
    off64_t result;
    AAsset* asset = (AAsset*)ctx->hidden.androidio.asset;
    result = AAsset_getLength64(asset);
    return result;
}

long long _carp_Android_JNI_FileSeek(CARP_RWops* ctx, long long offset, int whence)
{
    off64_t result;
    AAsset* asset = (AAsset*)ctx->hidden.androidio.asset;
    result = AAsset_seek64(asset, offset, whence);
    return result;
}

int _carp_Android_JNI_FileClose(CARP_RWops* ctx)
{
    AAsset* asset = (AAsset*)ctx->hidden.androidio.asset;
    AAsset_close(asset);
    return 0;
}

#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>

/* For proper OS X applications, the resources are contained inside the application bundle.
 So the strategy is to first check the application bundle for the file, then fallback to the current working directory.
 Note: One additional corner-case is if the resource is in a framework's resource bundle instead of the app.
 We might want to use bundle identifiers, e.g. org.libsdl.sdl to get the bundle for the framework,
 but we would somehow need to know what the bundle identifiers we need to search are.
 Also, note the bundle layouts are different for iPhone and Mac.
*/
FILE* _carp_apple_OpenFPFromBundleOrFallback(const char* file, const char* mode)
{ @autoreleasepool
{
    FILE * fp = NULL;

    /* If the file mode is writable, skip all the bundle stuff because generally the bundle is read-only. */
    if (strcmp("r", mode) && strcmp("rb", mode)) {
        return fopen(file, mode);
    }

    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSString* resource_path = [[NSBundle mainBundle]resourcePath];

    NSString* ns_string_file_component = [file_manager stringWithFileSystemRepresentation : file length : strlen(file)];

    NSString* full_path_with_file_to_try = [resource_path stringByAppendingPathComponent : ns_string_file_component];
    if ([file_manager fileExistsAtPath : full_path_with_file_to_try]) {
        fp = fopen([full_path_with_file_to_try fileSystemRepresentation], mode);
    }
    else {
        fp = fopen(file, mode);
    }

    return fp;
}}

#ifdef TARGET_OS_IPHONE
void _carp_ios_GetResourcePath(const char* buffer, size_t len)
{
	
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* path = [paths objectAtIndex : 0];
    strcpy(buffer, [path UTF8String], len);
}
#endif

#endif

const char* CARP_GetInternalDataPath()
{
    static char path[128] = { 0 };
    path[1] = 0;
#ifdef __ANDROID__
    ANativeActivity* activity = (ANativeActivity*)sapp_android_get_native_activity();
    if (activity == NULL) return path;
    if (activity->assetManager == NUL) return path;
    return activity->internalDataPath;
#elif TARGET_OS_IPHONE
    _carp_ios_GetResourcePath(path, sizeof(path));
    return path;
#endif
    return path;
}
	
const char* CARP_GetExternalDataPath()
{
#ifdef __ANDROID__
    ANativeActivity* activity = (ANativeActivity*)sapp_android_get_native_activity();
    if (activity == NULL) return path;
    if (activity->assetManager == NULL) return path;
    return activity->externalDataPath;
#endif
    return CARP_GetInternalDataPath();
}
	
CARP_RWops* CARP_RWFromFile(const char* file, const char* mode, int only_assets)
{
    CARP_RWops* rwops = NULL;
    if (!file || !*file || !mode || !*mode) {
        return NULL;
    }
#ifdef __ANDROID__
    if (!only_assets)
    {
        /* Try to open the file on the filesystem first */
        if (*file == '/') {
            FILE* fp = fopen(file, mode);
            if (fp) {
                return CARP_RWFromFP(fp, 1);
            }
        } else {
            /* Try opening it from internal storage if it's a relative path */
            char path[1024];
            FILE* fp;

            ANativeActivity* activity = (ANativeActivity*)sapp_android_get_native_activity();
            if (activity == NULL) return -1;
            if (activity->assetManager == NUL) return -1;

            snprintf(path, sizeof(path), "%s/%s", activity->internalDataPath, file);
            fp = fopen(path, mode);
            if (fp) {
                return CARP_RWFromFP(fp, 1);
            }
        }
    }

    /* Try to open the file from the asset system */
    rwops = CARP_AllocRW();
    if (!rwops)
        return NULL;            /* CARP_SetError already setup by CARP_AllocRW() */
    if (_carp_Android_JNI_FileOpen(rwops, file, mode) < 0) {
        CARP_FreeRW(rwops);
        return NULL;
    }
    rwops->size = _carp_Android_JNI_FileSize;
    rwops->seek = _carp_Android_JNI_FileSeek;
    rwops->read = _carp_Android_JNI_FileRead;
    rwops->write = _carp_Android_JNI_FileWrite;
    rwops->close = _carp_Android_JNI_FileClose;
    rwops->type = CARP_RWOPS_JNIFILE;

    return rwops;
#elif __APPLE__
    FILE* fp = _carp_apple_OpenFPFromBundleOrFallback(file, mode);
#elif _WIN32
    int wfile_len = MultiByteToWideChar(CP_UTF8, 0, file, -1, NULL, 0);
    wchar_t* wfile = (wchar_t*)malloc(sizeof(wchar_t) * wfile_len);
    memset(wfile, 0, sizeof(wchar_t) * wfile_len);
    MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, wfile_len);

    int wmode_len = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);
    wchar_t* wmode = (wchar_t*)malloc(sizeof(wchar_t) * wmode_len);
    memset(wmode, 0, sizeof(wchar_t) * wmode_len);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, wmode_len);
    
    FILE* fp = NULL;
    _wfopen_s(&fp, wfile, wmode);
    free(wfile);
    free(wmode);
#else
    FILE* fp = fopen(file, mode);
#endif
    if (fp != NULL) {
        rwops = CARP_RWFromFP(fp, 1);
    }
    return rwops;
}

CARP_RWops* CARP_RWFromFP(FILE* fp, int autoclose)
{
    CARP_RWops* rwops = NULL;

    rwops = CARP_AllocRW();
    if (rwops != NULL) {
        rwops->size = _carp_stdio_size;
        rwops->seek = _carp_stdio_seek;
        rwops->read = _carp_stdio_read;
        rwops->write = _carp_stdio_write;
        rwops->close = _carp_stdio_close;
        rwops->hidden.stdio.fp = fp;
        rwops->hidden.stdio.autoclose = autoclose;
        rwops->type = CARP_RWOPS_STDFILE;
    }
    return rwops;
}

CARP_RWops* CARP_RWFromMem(void* mem, int size)
{
    CARP_RWops* rwops = NULL;
    if (!mem || !size) {
        return rwops;
    }

    rwops = CARP_AllocRW();
    if (rwops != NULL) {
        rwops->size = _carp_mem_size;
        rwops->seek = _carp_mem_seek;
        rwops->read = _carp_mem_read;
        rwops->write = _carp_mem_write;
        rwops->close = _carp_mem_close;
        rwops->hidden.mem.base = (unsigned char*)mem;
        rwops->hidden.mem.here = rwops->hidden.mem.base;
        rwops->hidden.mem.stop = rwops->hidden.mem.base + size;
        rwops->type = CARP_RWOPS_MEMORY;
    }
    return rwops;
}

CARP_RWops* CARP_AllocRW(void)
{
    CARP_RWops* area;

    area = (CARP_RWops*)malloc(sizeof * area);
    if (area == NULL) {
        return NULL;
    }

    area->type = CARP_RWOPS_UNKNOWN;
    return area;
}

void CARP_FreeRW(CARP_RWops* area)
{
    free(area);
}

long long CARP_RWsize(CARP_RWops* context)
{
    return context->size(context);
}

long long CARP_RWseek(CARP_RWops* context, long long offset, int whence)
{
    return context->seek(context, offset, whence);
}

long long CARP_RWtell(CARP_RWops* context)
{
    return context->seek(context, 0, CARP_RW_SEEK_CUR);
}

size_t CARP_RWread(CARP_RWops* context, void* ptr, size_t size, size_t maxnum)
{
    return context->read(context, ptr, size, maxnum);
}

size_t CARP_RWwrite(CARP_RWops* context, const void* ptr, size_t size, size_t num)
{
    return context->write(context, ptr, size, num);
}

int CARP_RWclose(CARP_RWops* context)
{
    return context->close(context);
}
#endif
#endif