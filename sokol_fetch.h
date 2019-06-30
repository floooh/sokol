#pragma once
/*
    sokol_fetch.h -- asynchronous data loading/streaming

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_MALLOC(s)             - your own malloc function (default: malloc(s))
    SOKOL_FREE(p)               - your own free function (default: free(p))
    SOKOL_LOG(msg)              - your own logging function (default: puts(msg))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_API_DECL              - public function declaration prefix (default: extern)
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SFETCH_MAX_PATH             - max length of UTF-8 filesystem path / URL (default: 1024 bytes)
    SFETCH_MAX_USERDATA_UINT64  - max size of embedded userdata in number of uint64_t, userdata
                                  will be copied into an 8-byte aligned memory region associated
                                  with each in-flight request, default value is 16 (== 128 bytes)
    SFETCH_MAX_CHANNELS         - max number of IO channels (default is 16, also see sfetch_desc_t.num_channels)

    If sokol_fetch.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    TODO:
    =====
    - sftech_cancel()
    - Windows support
    - documentation
    - code cleanup
    - tests for pause/continue/cancel

    FEATURE OVERVIEW
    ================

    - Asynchronously load entire files, or stream files incrementally via
      HTTP (on web platform), or the local file system (on native platforms)

    - Request / response-callback model, user code "sends" a request
      to initiate a file-load, sokol_fetch.h calls the response callback
      on the same thread when data is ready, or user-code needs to respond

    - Not limited to the main-thread or a single-thread: A sokol-fetch
      "context" can live on any thread, and multiple context
      can operate side-by-side on different threads.

    - Memory management for data buffers is entirely the responsibility of
      the application, sokol_fetch.h will never allocate memory during
      operation.

    - Automatic "rate-limiting" guarantees that only a maximum number of
      requests is processed at any one time, allowing a "zero-allocation
      model", where all data is streamed into fixed-size, pre-allocated
      buffers.

    - Active Requests can be paused, continued and cancelled from anywhere
      in the user-thread which sent this request.

    - (Reasonably) memory-safe:
        - The sokol-fetch API never passes pointers back to the user, only
          integer-handles and structs by value.
        - Requests are identified through "generation-counted index-handles",
          allowing to detect dangling accesses, passing handles into the
          API which are not currently associated with a valid request will
          not do any harm (same idea as in sokol_gfx.h and other sokol-headers)
        - All data passed via pointers *into* the sokol-fetch API will be
          copied as needed (most notably the path/URL string, and any user-data
          associated with a request)

    API USAGE
    =========

    void sfetch_setup(const sfetch_desc_t* desc)
    --------------------------------------------
    First call sfetch_setup(const sfetch_desc_t*) on any thread before calling
    any other sokol-fetch functions on the same thread.

    sfetch_setup() takes a pointer to an sfetch_desc_t struct with setup
    parameters (any parameters that are not provided must be zero-initialized):

        - max_requests (uint32_t):
            The maximum number of requests that can be alive at any time, the
            default is 128.

        - num_channels (uint32_t):
            The number of "IO channels" used to parallelize and prioritize
            requests, the default is 1.

        - num_lanes (uint32_t):
            The number of "lanes" on a single channel. Each request this is
            currently 'inflight' on a channel occupies one lane until the
            request is finished. This is used for automated rate-limiting
            (search below for CHANNELS AND LANES for more details). The
            default number of lanes is 16.

    For example, to setup sokol-fetch for max 1024 active requests, 4 channels,
    and 8 lanes per channel in C99:

        sfetch_setup(&(sfetch_desc_t){
            .max_requests = 1024,
            .num_channels = 4,
            .num_lanes = 8
        });

    sfetch_setup() is the only place where sokol-fetch will allocate memory
    (you can override SOKOL_MALLOC and SOKOL_FREE before including the
    implementation to hook in your own memory allocation functions).

    sfetch_handle_t sfetch_send(const sfetch_request_t* request)
    ------------------------------------------------------------
    sokol-fetch is now ready for accepting fetch-requests. Call sfetch_send()
    to start a fetch operation, the function takes a pointer to an
    sfetch_request_t struct with request parameters and returns a
    sfetch_handle_t identifying the request for later calls:

        sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
            ...
        });

    sfetch_send() will return an invalid handle if no request can be allocated
    from the internal pool (more requests are in flight than sfetch_desc_t.max_requests).

    The sfetch_request_t struct contains the following parameters (any parameters
    that are not provided must be zero-initialized):

        - path (const char*, required)
            Pointer to an UTF-8 encoded C string describing the filesystem
            path or HTTP URL. The string will be copied into an internal data
            structure, and passed "as is" (apart from any required
            encoding-conversions) to fopen(), CreateFileW() or
            XMLHttpRequest. The maximum length of the string is defined by
            the SFETCH_MAX_PATH config define, the default is 1024 bytes
            including the 0-terminator.

        - callback (sfetch_callback_t, required)
            Pointer to a response-callback function which is called when the
            request needs "user code attention". Search below for REQUEST
            STATES AND THE RESPONSE CALLBACK for detailed information about
            handling responses in the response callback.

        - channel (uint32_t, optional)
            Index of the IO channel where the request should be processed.
            Channels are used to parallelize and prioritize requests relative
            to each other. Search below for CHANNELS AND LANES for more
            information.

        - buffer (sfetch_buffer_t, optional)
            This is a pointer/size pair describing a chunk of memory where
            data will be loaded into. Providing a buffer "upfront" in
            sfetch_request() is optional, and makes sense in the following
            two situations:
                (1) a maximum file size is known for the request, so that
                    is guaranteed that the entire file content will fit
                    into the buffer
                (2) ...or the file should be streamed in small chunks, with the
                    resonse-callback being called after each chunk to
                    'process' the partial file data
            Search below for BUFFER MANAGEMENT for more detailed information.

        - user_data, user_data_size (const void*, uint32_t, both optional)
            user_data and user_data_size describe an optional POD (plain-old-data)
            associated with the request which will be copied(!) into an internal
            memory block. The maximum default size of this memory block is
            128 bytes (but can be overriden by defining SFETCH_MAX_USERDATA_UINT64
            before including the notification, note that this define is in
            "number of uint64_t", not number of bytes). The user_data
            block is 8-byte aligned, and will be copied via memcpy() (so don't
            put any C++ "smart members" in there).

    NOTE that request handles are strictly thread-local and only unique
    within the thread the handle was created on, and all function calls
    involving a request handle must happen on that same thread.

    bool sfetch_handle_valid(sfetch_handle_t request)
    -------------------------------------------------
    This checks if the provided handle is valid, and is associated with
    a currently active request. It will return false if:

        - sfetch_send() returned an invalid handle because it couldn't allocate
          a new request from the internal request pool (because they're all
          in flight)
        - the request associated with the handle is no longer alive (because
          it either finished successfully, or the request failed for some
          reason)

    void sfetch_dowork(void)
    ------------------------
    Call sfetch_dowork(void) on in regular intervals (for instance once per frame)
    on the same thread as sfetch_setup() to "turn the gears". If you are sending
    requests but never hear back from them in the response callback function, then
    the most likely reason is that you forgot to add the call to sfetch_dowork()
    in the per-frame function.

    sfetch_dowork() roughly performs the following work:

        - any new requests that have been sent with sfetch_send() since the
          last call to sfetch_dowork() will be dispatched to their IO channels,
          permitting that any lanes on that specific IO channel are free
          (otherwise, incoming requests are waiting until another request
          request on the same channel is finished, freeing up a lane)

        - a state transition from "user side" to "IO side" happens for
          each new request that has been dispatched to a channel.

        - requests dispatched to a channel are either forwarded into that
          channel's worker thread (on native platforms), or cause an
          HTTP request to be sent via an asynchronous XMLHttpRequest
          (on the web platform)

        - for any requests for which their current async operation has finished,
          a state transition from "IO side" to "user side" is performed and
          the response callback is called

        - requests which are finished (either because the entire file content has
          been loaded, or they are in the FAILED state) are freed (this
          just changes their state in the 'request pool', no actual
          memory is freed)

        - requests which are not yet finished are fed back into the 'incoming'
          queue of their channel, and the whole cycle starts again

    void sfetch_cancel(sfetch_handle_t request)
    -------------------------------------------
    This cancels a request at the next possible convenience, puts
    it into the FAILED state and calls the response callback with
    (response.state == SFETCH_STATE_FAILED) and (response.finished == true)
    to give user-code a chance to do any cleanup work for the request.
    If sfetch_cancel() is called for a request that is no longer alive,
    nothing bad will happen (the call will simply do nothing).

    void sfetch_pause(sfetch_handle_t request)
    ------------------------------------------
    This pauses an active request at the next possible convenience, puts it
    into the PAUSED state. For all requests in PAUSED state, the response
    callback will be called in each call to sfetch_dowork() to give user-code
    a chance to CONTINUE the request (by calling sfetch_continue()). Pausing
    a request makes sense for dynamic rate-limiting in streaming scenarios
    (like video/audio streaming with a fixed number of streaming buffers. As
    soon as all available buffers are filled with download data, downloading
    more data must be prevented to allow video/audio playback to catch up and
    free up empty buffers for new download data.

    void sfetch_continue(sftech_handle_t request)
    ---------------------------------------------
    Continues a paused request, counterpart to the sfetch_pause() function.

    void sfetch_set_buffer(sfetch_handle_t request, const sfetch_buffer_t* buffer)
    ------------------------------------------------------------------------------
    Associates a new buffer (pointer/size pair) with an active request. The
    function can be called at any time from inside or outside the response
    callback. Search below for BUFFER MANAGEMENT for more detailed
    information.

    sfetch_desc_t sfetch_desc(void)
    -------------------------------
    sfetch_desc() returns a copy of the sfetch_desc_t struct that was
    passed to sfetch_setup(). Useful for checking the max_requests,
    num_channels and num_lanes items sokol-fetch was configured with.

    int sfetch_max_userdata_bytes(void)
    -----------------------------------
    This returns the value of the SFETCH_MAX_USERDATA_UINT64 implementation
    define, but in number of bytes (so SFETCH_MAX_USERDATA_UINT64*8).

    int sfetch_max_path(void)
    -------------------------
    Returns the value of the SFETCH_MAX_PATH implementation define.

    REQUEST STATES AND THE RESPONSE CALLBACK
    ========================================
    A request switches between states during its lifetime, and "ownership"
    of the request changes between an IO-thread (on native platforms only),
    and the thread where the request was sent from. You can think of
    a request as "ping-ponging" between the IO thread and user thread,
    any actual IO work is done on the IO thread, while invocations of the
    response-callback happen on the user-thread.

    State transitions and invoking the response-callback
    happens inside sfetch_dowork().

    An active request goes through the following states:

    ALLOCATED (user-thread)

        The request has been allocated in sfetch_send() and is
        waiting to be dispatched into its IO channel. When this
        happens, the request will transition into the OPENING state.

    OPENING (IO thread)

        The request is currently being opened on the IO thread. After the
        file has been opened, its overall content-size will be queried.

        If a buffer was provided in sfetch_send() the request will
        immediately transition into the FETCHING state and start loading
        data into the buffer.

        If no buffer was provided in sfetch_send(), the request will
        transition into the OPENED state.

        If opening the file failed, the request will transition into
        the FAILED state.

    OPENED (user thread)

        A request will go into the OPENED state after its file has been
        opened successfully, but not buffer was provided to load data
        into.

        In the OPENED state, the response-callback will be called so that
        the user-code can have a look at the file's content-size and
        provide a buffer for the request by calling sfetch_set_buffer().

        After the response callback has been called, and a buffer was provided,
        the request will transition into the FETCHING state.

        If no buffer was provided in the response callback, the request
        will transition into the FAILED state.

    FETCHING (IO thread)

        While a request in in the FETCHING state, data will be loaded into
        the user-provided buffer.

        If the buffer is full, or the entire file content has been loaded,
        the request will transition into the FETCHED state.

        If something went wrong during loading (less bytes could be
        read than expected), the request will transition into the FAILED
        state.

    FETCHED (user thread)

        The request goes into the FETCHED state either when the request's
        buffer has been completely filled with loaded data, or the entire
        file content has been loaded.

        The response callback will be called so that the user-code can
        process the loaded data.

        If all file data has been loaded, the 'finished' flag will be set
        in the response callback's sfetch_response_t argument.

        After the user callback returns, and all file data has been loaded
        (response.finished flag is set) the request has reached its end-of-life
        and will recycled.

        Otherwise, if there's still data to load (because the buffer size
        is smaller than the file's content-size), the request will switch
        back to the FETCHING state to load the next chunk of data.

        Note that it is ok to associate a different buffer or buffer-size
        with the request by calling sfetch_set_buffer() in the response-callback.

    FAILED (user thread)

        A request will transition in the FAILED state in the following situations:

            - during OPENING if the file doesn't exist or couldn't be
              opened for other reasons
            - during FETCHING when no buffer is currently associated
              with the request
            - during FETCHING if less than the expected number of bytes
              could be read
            - if a request has been cancelled via sfetch_cancel()

        The response callback will be called once after a request goes
        into the FAILED state, with the response.finished flag set to
        true. This gives the user-code a chance to cleanup any resources
        associated with the request.

    PAUSED (user thread)

        A request will transition into the PAUSED state after user-code
        calls the function sfetch_pause() on the request's handle. Usually
        this happens from within response-callback in streaming scenarios
        when the data streaming needs to wait for a data decoder (like
        a video/audio player) to catch up.

        While a request is in PAUSED state, the response-callback will be
        called in each sfetch_dowork(), so that the user-code can either
        continue the request by calling sfetch_continue(), or cancel
        the request by calling sfetch_cancel().

        When calling sfetch_continue() on a paused request, the request will
        transition into the FETCHING state. Otherwise if sfetch_cancel() is
        called, the request will switch into the FAILED state.


    CHANNELS AND LANES
    ==================

    BUFFER MANAGEMENT
    =================



    TEMP NOTE DUMP
    ==============

    - TL;DR:
        - asynchronous requests with response-callbacks
        - only reading, not writing (thus the name "sokol_fetch.h", instead
          of "sokol_io.h")
        - not limited to "main thread" or a single thread
        - "channels" for parallelization/prioritization, "lanes" for automatic rate-limiting
        - user-provided buffers, no allocations past initialization

    - Asynchronously load complete files, or stream data chunks from the local
      filesystem (on "native" platforms), or via HTTP (on WASM) while giving
      user-code full control over memory.

    - sfetch_setup() can be called on any thread, or multiple threads, each
      call sets up a complete thread-local context along with its own set of IO
      threads.

    - Call "sfetch_send(const sfetch_request_t* request)" to start
      an asynchronous fetch-request. At the minimum the request-struct contains a
      file path/URL and a 'response callback'. sfetch_send() returns an opaque
      'request handle' of type sfetch_handle_t.

    - The sfetch_dowork() function is called once per "frame" and pumps messages
      in and out of IO threads, and invokes response-callbacks.

    - An active request calls the user-provided response callback on the same
      thread it was sent once or multiple times when the request
      changes to a state which needs 'user-code attention' (you can think
      of a request as ping-ponging between the user-thread and an IO-thread
      multiple times, although the lib does its best to avoid any unnecessary
      'ping-ponging' to keep the 'latency' of a single request low).

    - No memory will be allocated after sfetch_setup() is called. More specifically, all
      data is loaded into user-provided buffers (which may be provided "on demand"
      in the OPENED state of a request (since only then the file size is known),
      or upfront (in streaming-scenarios, or a maximum file size is known upfront).

    - A provided buffer can be smaller than the file content, in that case,
      the response callback will be called multiple times with partially
      fetched data until the whole file content has been loaded. This is
      useful for streaming scenarios, or generally to limit overall memory usage
      if loaded data can be processed in chunks.

    - Requests are distributed to IO "channels" for parallelization and per-channel
      "lanes" for rate-limiting. Channel indices are user-provided in the
      request struct, lanes are assigned automatically.

    - Channels can be used to prioritize requests or separate requests by
      system. For instance, big and slow fetches can be issued to a different
      channel than small and fast downloads.

    - Per-channel lanes are used to limit the maximum number of requests that are
      processed on a channel. Each request on a channel occupies a lane until the
      entire request is completed.

    - Since channels and lanes guarantee that only a max number of requests is
      processed at any one time, user-code can provide a fixed number of upfront-
      allocated IO buffers.

    - On native platforms, each channel owns one thread where "traditional"
      blocking IO functions are called (fopen/fread/fclose on POSIX-ish
      platforms, CreateFileW etc... on Windows). On WASM, XmlHttpRequests on
      the browser thread are used, and the server is expected to support HTTP
      range-requests (POSIX-style threading support may be added later for WASM
      if it makes sense).

    - Fetch-requests go through states, and a request's response-callback is called
      during sfetch_dowork() when a request enters certain states
      (FIXME: this needs some ASCII flowgraph-graphics):
        - OPENING: File is currently being opened by the IO thread, if file
          doesn't exist, transition to FAILED, otherwise get the overall
          size of the file. If a buffer was provided upfront, immediately
          go into FETCHING state, otherwise OPENED.
        - OPENED: If no buffer was provided upfront, a request goes into the
          OPENED state and the response callback is called so that the user-code
          can provide a buffer via sfetch_set_buffer() from inside the callback.
          The overall file size is available in the response structure passed
          to the callback.
        - FETCHING: an IO thread is currently loading data into the provided buffer,
          may transition into FETCHED or FAILED
        - FETCHED: IO thread has completed loading data into the buffer, either
          because the buffer is full, or all file content has been loaded.
          The response callback will be called so that the user code can
          process the data. If the whole file content has been loaded,
          a 'finished' flag will be set in the callback's response argument.
        - FAILED: The IO request has failed, the response callback will be called
          to give user-code a chance to cleanup. This happens when:
            - OPENING the file has failed, in this case the request goes
              into the FAILED state, instead of OPENED
            - When some errors occurs during FETCHING after the file was
              successfully opened. In that case, the request will transition
              into FAILED instead of FETCHING.
        - PAUSED (TODO): a request may go into PAUSED state when the response
          callback calls the function sfetch_pause() for a request. This is
          useful for 'dynamic rate limiting' in streaming scenarios (e.g. when
          the

        NOTE how all states which are processed on the user-side in the
        response callback end with -ED (OPENED, FETCHED, PAUSED, FAILED),
        while all states which are currently processed on the IO-thread-side
        end with -ING (OPENING, FETCHING)

    - (TODO) A request can be cancelled both from in- or outside-the response callback.
      A cancelled request will go into the FAILED state and the response-callback will
      be called so the user-code has a chance to react.

    - Planned for "Version 2.0":
        - Pluggable request handler code, for instance to implement HTTP
          downloads on "native platforms" via libcurl etc..., or load from
          any other "data source".
        - A polling-API as an alternative to the response-callback. I postponed
          this because the callback-API simplifies handling some potentially
          situations (like accessing buffer data while a thread overwrites it,
          or 'forgotten' requests clogging up the system).

    zlib/libpng license

    Copyright (c) 2019 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_FETCH_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* configuration values for sfetch_setup() */
typedef struct sfetch_desc_t {
    uint32_t _start_canary;
    uint32_t max_requests;          /* max number of active requests across all channels, default is 128 */
    uint32_t num_channels;          /* number of channels to fetch requests in parallel, default is 1 */
    uint32_t num_lanes;             /* max number of requests active on the same channel, default is 16 */
    uint32_t _end_canary;
} sfetch_desc_t;

/* a request handle to identify an active fetch request */
typedef struct sfetch_handle_t { uint32_t id; } sfetch_handle_t;

/* a request goes through the following states, ping-ponging between IO and user thread */
typedef enum sfetch_state_t {
    SFETCH_STATE_INITIAL = 0,   /* internal: request has just been initialized */
    SFETCH_STATE_ALLOCATED,     /* internal: request has been allocated from internal pool */

    SFETCH_STATE_OPENING,       /* IO thread: waiting to be opened */
    SFETCH_STATE_OPENED,        /* user thread: follow state of OPENING if no buffer was provided */
    SFETCH_STATE_FETCHING,      /* IO thread: waiting for data to be fetched */
    SFETCH_STATE_FETCHED,       /* user thread: fetched data available */
    SFETCH_STATE_PAUSED,        /* user thread: request has been paused via sfetch_pause() */
    SFETCH_STATE_FAILED,        /* user thread: follow state of OPENING or FETCHING if something went wrong */

    _SFETCH_STATE_NUM,
} sfetch_state_t;

typedef struct sfetch_buffer_t {
    uint8_t* ptr;
    uint64_t size;
} sfetch_buffer_t;

typedef struct sfetch_response_t {
    sfetch_handle_t handle;         /* request handle this response belongs to */
    sfetch_state_t state;           /* current request state */
    bool finished;                  /* this is the last response for this request */
    uint32_t channel;               /* the IO channel where this request 'lives' */
    uint32_t lane;                  /* the IO lane in its channel this request was assigned to */
    const char* path;               /* the original filesystem path of the request */
    const void* user_data;          /* pointer to read-only(!) user-data area */
    uint64_t content_size;          /* overall file size in bytes*/
    uint64_t chunk_offset;          /* offset of fetched data chunk in file */
    sfetch_buffer_t chunk;          /* pointer and size of currently fetched chunk */
} sfetch_response_t;

typedef struct sfetch_return_t {
    sfetch_buffer_t buffer;         /* if not zeroed, set new data buffer associated with request */
} sfetch_return_t;

typedef void(*sfetch_callback_t)(sfetch_response_t);

typedef struct sfetch_request_t {
    uint32_t _start_canary;
    uint32_t channel;
    const char* path;
    sfetch_callback_t callback;
    sfetch_buffer_t buffer;         /* it's optional to provide a buffer upfront, can also happen in OPENED state */
    const void* user_data;          /* optional user-data will be memcpy'ed into a memory region set aside for each request */
    uint32_t user_data_size;
    uint32_t _end_canary;
} sfetch_request_t;

/* setup sokol-fetch (can be called on multiple threads) */
SOKOL_API_DECL void sfetch_setup(const sfetch_desc_t* desc);
/* discard a sokol-fetch context */
SOKOL_API_DECL void sfetch_shutdown(void);
/* return true if sokol-fetch has been setup */
SOKOL_API_DECL bool sfetch_valid(void);
/* get the desc struct that was passed to sfetch_setup() */
SOKOL_API_DECL sfetch_desc_t sfetch_desc(void);
/* return the max userdata size in number of bytes (SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) */
SOKOL_API_DECL int sfetch_max_userdata_bytes(void);
/* return the value of the SFETCH_MAX_PATH implementation config value */
SOKOL_API_DECL int sfetch_max_path(void);

/* send a fetch-request */
SOKOL_API_DECL sfetch_handle_t sfetch_send(const sfetch_request_t* request);
/* return true if a handle is valid *and* the request is alive */
SOKOL_API_DECL bool sfetch_handle_valid(sfetch_handle_t h);
/* do per-frame work, moves requests into and out of IO threads, and invokes callbacks */
SOKOL_API_DECL void sfetch_dowork(void);

/* update the IO buffer associated with a request (usually from inside response-callback) */
SOKOL_API_DECL void sfetch_set_buffer(sfetch_handle_t h, const sfetch_buffer_t* buf);
/* cancel a request that's in flight (will call response callback in FAILED state) */
SOKOL_API_DECL void sfetch_cancel(sfetch_handle_t h);
/* pause a request (will call response callback each frame in PAUSED state) */
SOKOL_API_DECL void sfetch_pause(sfetch_handle_t h);
/* continue a paused request */
SOKOL_API_DECL void sfetch_continue(sfetch_handle_t h);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef SOKOL_IMPL
#define SOKOL_FETCH_IMPL_INCLUDED (1)
#include <string.h> /* memset, memcpy */

#ifndef SFETCH_MAX_PATH
#define SFETCH_MAX_PATH (1024)
#endif
#ifndef SFETCH_MAX_USERDATA_UINT64
#define SFETCH_MAX_USERDATA_UINT64 (16)
#endif
#ifndef SFETCH_MAX_CHANNELS
#define SFETCH_MAX_CHANNELS (16)
#endif

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_MALLOC
    #include <stdlib.h>
    #define SOKOL_MALLOC(s) malloc(s)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG
        #include <stdio.h>
        #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
    #else
        #define SOKOL_LOG(s)
    #endif
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
    #define _SFETCH_PLATFORM_EMSCRIPTEN (1)
    #define _SFETCH_PLATFORM_WINDOWS (0)
    #define _SFETCH_PLATFORM_POSIX (0)
    #define _SFETCH_HAS_THREADS (0)
#elif defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #define _SFETCH_PLATFORM_WINDOWS (1)
    #define _SFETCH_PLATFORM_EMSCRIPTEN (0)
    #define _SFETCH_PLATFORM_POSIX (0)
    #define _SFETCH_HAS_THREADS (1)
#else
    #include <pthread.h>
    #include <stdio.h>  /* fopen, fread, fseek, fclose */
    #define _SFETCH_PLATFORM_POSIX (1)
    #define _SFETCH_PLATFORM_EMSCRIPTEN (0)
    #define _SFETCH_PLATFORM_WINDOWS (0)
    #define _SFETCH_HAS_THREADS (1)
#endif

/*=== private type definitions ===============================================*/
typedef struct _sfetch_path_t {
    char buf[SFETCH_MAX_PATH];
} _sfetch_path_t;

/* a thread with incoming and outgoing message queue syncing */
#if _SFETCH_PLATFORM_POSIX
typedef struct {
    pthread_t thread;
    pthread_cond_t incoming_cond;
    pthread_mutex_t incoming_mutex;
    pthread_mutex_t outgoing_mutex;
    pthread_mutex_t running_mutex;
    pthread_mutex_t stop_mutex;
    bool stop_requested;
    bool valid;
} _sfetch_thread_t;
#elif _SFETCH_PLATFORM_WINDOWS
typedef struct {
    HANDLE thread;
    HANDLE incoming_event;
    CRITICAL_SECTION incoming_critsec;
    CRITICAL_SECTION outgoing_critsec;
    CRITICAL_SECTION running_critsec;
    CRITICAL_SECTION stop_critsec;
    bool stop_requested;
    bool valid;
} _sfetch_thread_t;
#endif

/* file handle abstraction */
#if _SFETCH_PLATFORM_POSIX
typedef FILE* _sfetch_file_handle_t;
#define _SFETCH_INVALID_FILE_HANDLE (0)
typedef void*(*_sfetch_thread_func_t)(void*);
#elif _SFETCH_PLATFORM_WINDOWS
typedef HANDLE _sfetch_file_handle_t;
#define _SFETCH_INVALID_FILE_HANDLE (INVALID_HANDLE_VALUE)
typedef LPTHREAD_START_ROUTINE _sfetch_thread_func_t;
#endif

/* user-side per-request state */
typedef struct {
    /* transfer user => IO thread */
    sfetch_buffer_t buffer;
    bool pause;                 /* switch item to PAUSED state if true */
    bool cont;                  /* switch item back to FETCHING if true */
    /* transfer IO => user thread */
    uint64_t content_size;      /* overall file size */
    uint64_t fetched_size;      /* number of bytes fetched so far */
    uint64_t chunk_size;        /* size of last fetched chunk */
    bool finished;
    /* user thread only */
    int user_data_size;
    uint64_t user_data[SFETCH_MAX_USERDATA_UINT64];
} _sfetch_item_user_t;

/* thread-side per-request state */
typedef struct {
    /* transfer user => IO thread */
    sfetch_buffer_t buffer;
    /* transfer IO => user thread */
    uint64_t content_size;
    uint64_t fetched_size;
    uint64_t chunk_size;
    bool failed;
    bool finished;
    /* IO thread only */
    #if !_SFETCH_PLATFORM_EMSCRIPTEN
    _sfetch_file_handle_t file_handle;
    #endif
} _sfetch_item_thread_t;

/* an internal request item */
#define _SFETCH_INVALID_LANE (0xFFFFFFFF)
typedef struct {
    sfetch_handle_t handle;
    sfetch_state_t state;
    uint32_t channel;
    uint32_t lane;
    sfetch_callback_t callback;

    /* updated by IO-thread, off-limits to user thead */
    _sfetch_item_thread_t thread;

    /* accessible by user-thread, off-limits to IO thread */
    _sfetch_item_user_t user;

    /* big stuff at the end */
    _sfetch_path_t path;
} _sfetch_item_t;

/* a pool of internal per-request items */
typedef struct {
    uint32_t size;
    uint32_t free_top;
    _sfetch_item_t* items;
    uint32_t* free_slots;
    uint32_t* gen_ctrs;
    bool valid;
} _sfetch_pool_t;

/* a ringbuffer for pool-slot ids */
typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t num;
    uint32_t* buf;
} _sfetch_ring_t;

/* an IO channel with its own IO thread */
struct _sfetch_t;
typedef struct {
    struct _sfetch_t* ctx; /* this is a backpointer to the thread-local _sfetch_t state,
                              needed as argument for the worker thread */
    _sfetch_ring_t free_lanes;
    _sfetch_ring_t user_sent;
    _sfetch_ring_t user_incoming;
    _sfetch_ring_t user_outgoing;
    #if _SFETCH_HAS_THREADS
    _sfetch_ring_t thread_incoming;
    _sfetch_ring_t thread_outgoing;
    _sfetch_thread_t thread;
    #endif
    void (*request_handler)(struct _sfetch_t* ctx, uint32_t slot_id);
    bool valid;
} _sfetch_channel_t;

/* the sfetch global state */
typedef struct _sfetch_t {
    bool setup;
    bool valid;
    sfetch_desc_t desc;
    _sfetch_pool_t pool;
    _sfetch_channel_t chn[SFETCH_MAX_CHANNELS];
    sfetch_buffer_t null_buffer;
} _sfetch_t;
#if _SFETCH_HAS_THREADS
#if defined(_MSC_VER)
static __declspec(thread) _sfetch_t* _sfetch;
#else
static __thread _sfetch_t* _sfetch;
#endif
#else
static _sfetch_t* _sfetch;
#endif

/*=== general helper functions and macros =====================================*/
#define _sfetch_def(val, def) (((val) == 0) ? (def) : (val))

_SOKOL_PRIVATE _sfetch_t* _sfetch_ctx(void) {
    return _sfetch;
}

_SOKOL_PRIVATE void _sfetch_path_copy(_sfetch_path_t* dst, const char* src) {
    SOKOL_ASSERT(dst);
    if (src && (strlen(src) < SFETCH_MAX_PATH)) {
        #if defined(_MSC_VER)
        strncpy_s(dst->buf, SFETCH_MAX_PATH, src, (SFETCH_MAX_PATH-1));
        #else
        strncpy(dst->buf, src, SFETCH_MAX_PATH);
        #endif
        dst->buf[SFETCH_MAX_PATH-1] = 0;
    }
    else {
        memset(dst->buf, 0, SFETCH_MAX_PATH);
    }
}

_SOKOL_PRIVATE _sfetch_path_t _sfetch_path_make(const char* str) {
    _sfetch_path_t res;
    _sfetch_path_copy(&res, str);
    return res;
}

_SOKOL_PRIVATE uint32_t _sfetch_make_id(uint32_t index, uint32_t gen_ctr) {
    return (gen_ctr<<16) | (index & 0xFFFF);
}

_SOKOL_PRIVATE sfetch_handle_t _sfetch_make_handle(uint32_t slot_id) {
    sfetch_handle_t h;
    h.id = slot_id;
    return h;
}

_SOKOL_PRIVATE uint32_t _sfetch_slot_index(uint32_t slot_id) {
    return slot_id & 0xFFFF;
}

/*=== a circular message queue ===============================================*/
_SOKOL_PRIVATE uint32_t _sfetch_ring_wrap(const _sfetch_ring_t* rb, uint32_t i) {
    return i % rb->num;
}

_SOKOL_PRIVATE void _sfetch_ring_discard(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb);
    if (rb->buf) {
        SOKOL_FREE(rb->buf);
        rb->buf = 0;
    }
    rb->head = 0;
    rb->tail = 0;
    rb->num = 0;
}

_SOKOL_PRIVATE bool _sfetch_ring_init(_sfetch_ring_t* rb, uint32_t num_slots) {
    SOKOL_ASSERT(rb && (num_slots > 0));
    SOKOL_ASSERT(0 == rb->buf);
    rb->head = 0;
    rb->tail = 0;
    /* one slot reserved to detect full vs empty */
    rb->num = num_slots + 1;
    const size_t queue_size = rb->num * sizeof(sfetch_handle_t);
    rb->buf = (uint32_t*) SOKOL_MALLOC(queue_size);
    if (rb->buf) {
        memset(rb->buf, 0, queue_size);
        return true;
    }
    else {
        _sfetch_ring_discard(rb);
        return false;
    }
}

_SOKOL_PRIVATE bool _sfetch_ring_full(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->buf);
    return _sfetch_ring_wrap(rb, rb->head + 1) == rb->tail;
}

_SOKOL_PRIVATE bool _sfetch_ring_empty(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->buf);
    return rb->head == rb->tail;
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_count(const _sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->buf);
    uint32_t count;
    if (rb->head >= rb->tail) {
        count = rb->head - rb->tail;
    }
    else {
        count = (rb->head + rb->num) - rb->tail;
    }
    SOKOL_ASSERT(count < rb->num);
    return count;
}

_SOKOL_PRIVATE void _sfetch_ring_enqueue(_sfetch_ring_t* rb, uint32_t slot_id) {
    SOKOL_ASSERT(rb && rb->buf);
    SOKOL_ASSERT(!_sfetch_ring_full(rb));
    SOKOL_ASSERT(rb->head < rb->num);
    rb->buf[rb->head] = slot_id;
    rb->head = _sfetch_ring_wrap(rb, rb->head + 1);
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_dequeue(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb && rb->buf);
    SOKOL_ASSERT(!_sfetch_ring_empty(rb));
    SOKOL_ASSERT(rb->tail < rb->num);
    uint32_t slot_id = rb->buf[rb->tail];
    rb->tail = _sfetch_ring_wrap(rb, rb->tail + 1);
    return slot_id;
}

_SOKOL_PRIVATE uint32_t _sfetch_ring_peek(const _sfetch_ring_t* rb, uint32_t index) {
    SOKOL_ASSERT(rb && rb->buf);
    SOKOL_ASSERT(!_sfetch_ring_empty(rb));
    SOKOL_ASSERT(index < _sfetch_ring_count(rb));
    uint32_t rb_index = _sfetch_ring_wrap(rb, rb->tail + index);
    return rb->buf[rb_index];
}

/*=== request pool implementation ============================================*/
_SOKOL_PRIVATE void _sfetch_item_init(_sfetch_item_t* item, uint32_t slot_id, const sfetch_request_t* request) {
    SOKOL_ASSERT(item && (0 == item->handle.id));
    SOKOL_ASSERT(request && request->path);
    item->handle.id = slot_id;
    item->state = SFETCH_STATE_INITIAL;
    item->channel = request->channel;
    item->lane = _SFETCH_INVALID_LANE;
    item->user.buffer = request->buffer;
    item->path = _sfetch_path_make(request->path);
    item->callback = request->callback;
    #if !_SFETCH_PLATFORM_EMSCRIPTEN
    item->thread.file_handle = _SFETCH_INVALID_FILE_HANDLE;
    #endif
    if (request->user_data &&
        (request->user_data_size > 0) &&
        (request->user_data_size <= (SFETCH_MAX_USERDATA_UINT64*8)))
    {
        item->user.user_data_size = request->user_data_size;
        memcpy(item->user.user_data, request->user_data, request->user_data_size);
    }
}

_SOKOL_PRIVATE void _sfetch_item_discard(_sfetch_item_t* item) {
    SOKOL_ASSERT(item && (0 != item->handle.id));
    memset(item, 0, sizeof(_sfetch_item_t));
}

_SOKOL_PRIVATE void _sfetch_pool_discard(_sfetch_pool_t* pool) {
    SOKOL_ASSERT(pool);
    if (pool->free_slots) {
        SOKOL_FREE(pool->free_slots);
        pool->free_slots = 0;
    }
    if (pool->gen_ctrs) {
        SOKOL_FREE(pool->gen_ctrs);
        pool->gen_ctrs = 0;
    }
    if (pool->items) {
        SOKOL_FREE(pool->items);
        pool->items = 0;
    }
    pool->size = 0;
    pool->free_top = 0;
    pool->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_pool_init(_sfetch_pool_t* pool, uint32_t num_items) {
    SOKOL_ASSERT(pool && (num_items > 0) && (num_items < ((1<<16)-1)));
    SOKOL_ASSERT(0 == pool->items);
    /* NOTE: item slot 0 is reserved for the special "invalid" item index 0*/
    pool->size = num_items + 1;
    pool->free_top = 0;
    const size_t items_size = pool->size * sizeof(_sfetch_item_t);
    pool->items = (_sfetch_item_t*) SOKOL_MALLOC(items_size);
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    const size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
    pool->gen_ctrs = (uint32_t*) SOKOL_MALLOC(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    /* NOTE: it's not a bug to only reserve num_items here */
    const size_t free_slots_size = num_items * sizeof(int);
    pool->free_slots = (uint32_t*) SOKOL_MALLOC(free_slots_size);
    if (pool->items && pool->free_slots) {
        memset(pool->items, 0, items_size);
        memset(pool->gen_ctrs, 0, gen_ctrs_size);
        /* never allocate the 0-th item, this is the reserved 'invalid item' */
        for (uint32_t i = pool->size - 1; i >= 1; i--) {
            pool->free_slots[pool->free_top++] = i;
        }
        pool->valid = true;
    }
    else {
        /* allocation error */
        _sfetch_pool_discard(pool);
    }
    return pool->valid;
}

_SOKOL_PRIVATE uint32_t _sfetch_pool_item_alloc(_sfetch_pool_t* pool, const sfetch_request_t* request) {
    SOKOL_ASSERT(pool && pool->valid);
    if (pool->free_top > 0) {
        uint32_t slot_index = pool->free_slots[--pool->free_top];
        SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
        uint32_t slot_id = _sfetch_make_id(slot_index, ++pool->gen_ctrs[slot_index]);
        _sfetch_item_init(&pool->items[slot_index], slot_id, request);
        pool->items[slot_index].state = SFETCH_STATE_ALLOCATED;
        return slot_id;
    }
    else {
        /* pool exhausted, return the 'invalid handle' */
        return _sfetch_make_id(0, 0);
    }
}

_SOKOL_PRIVATE void _sfetch_pool_item_free(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    uint32_t slot_index = _sfetch_slot_index(slot_id);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
    SOKOL_ASSERT(pool->items[slot_index].handle.id == slot_id);
    #if defined(SOKOL_DEBUG)
    /* debug check against double-free */
    for (uint32_t i = 0; i < pool->free_top; i++) {
        SOKOL_ASSERT(pool->free_slots[i] != slot_index);
    }
    #endif
    _sfetch_item_discard(&pool->items[slot_index]);
    pool->free_slots[pool->free_top++] = slot_index;
    SOKOL_ASSERT(pool->free_top <= (pool->size - 1));
}

/* return pointer to item by handle without matching id check */
_SOKOL_PRIVATE _sfetch_item_t* _sfetch_pool_item_at(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    uint32_t slot_index = _sfetch_slot_index(slot_id);
    SOKOL_ASSERT((slot_index > 0) && (slot_index < pool->size));
    return &pool->items[slot_index];
}

/* return pointer to item by handle with matching id check */
_SOKOL_PRIVATE _sfetch_item_t* _sfetch_pool_item_lookup(_sfetch_pool_t* pool, uint32_t slot_id) {
    SOKOL_ASSERT(pool && pool->valid);
    if (0 != slot_id) {
        _sfetch_item_t* item = _sfetch_pool_item_at(pool, slot_id);
        if (item->handle.id == slot_id) {
            return item;
        }
    }
    return 0;
}

/*=== PLATFORM WRAPPER FUNCTIONS =============================================*/
#if _SFETCH_PLATFORM_POSIX
_SOKOL_PRIVATE _sfetch_file_handle_t _sfetch_file_open(const _sfetch_path_t* path) {
    return fopen(path->buf, "rb");
}

_SOKOL_PRIVATE void _sfetch_file_close(_sfetch_file_handle_t h) {
    fclose(h);
}

_SOKOL_PRIVATE bool _sfetch_file_handle_valid(_sfetch_file_handle_t h) {
    return h != _SFETCH_INVALID_FILE_HANDLE;
}

_SOKOL_PRIVATE uint64_t _sfetch_file_size(_sfetch_file_handle_t h) {
    fseek(h, 0, SEEK_END);
    return ftell(h);
}

_SOKOL_PRIVATE bool _sfetch_file_read(_sfetch_file_handle_t h, uint64_t offset, uint64_t num_bytes, void* ptr) {
    fseek(h, offset, SEEK_SET);
    return num_bytes == fread(ptr, 1, num_bytes, h);
}

_SOKOL_PRIVATE bool _sfetch_thread_init(_sfetch_thread_t* thread, _sfetch_thread_func_t thread_func, void* thread_arg) {
    SOKOL_ASSERT(thread && !thread->valid && !thread->stop_requested);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->incoming_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->outgoing_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->running_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&thread->stop_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_cond_init(&thread->incoming_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    /* FIXME: in debug mode, the threads should be named */
    pthread_mutex_lock(&thread->running_mutex);
    int res = pthread_create(&thread->thread, 0, thread_func, thread_arg);
    thread->valid = (0 == res);
    pthread_mutex_unlock(&thread->running_mutex);
    return thread->valid;
}

_SOKOL_PRIVATE void _sfetch_thread_request_stop(_sfetch_thread_t* thread) {
    pthread_mutex_lock(&thread->stop_mutex);
    thread->stop_requested = true;
    pthread_mutex_unlock(&thread->stop_mutex);
}

_SOKOL_PRIVATE bool _sfetch_thread_stop_requested(_sfetch_thread_t* thread) {
    pthread_mutex_lock(&thread->stop_mutex);
    bool stop_requested = thread->stop_requested;
    pthread_mutex_unlock(&thread->stop_mutex);
    return stop_requested;
}

_SOKOL_PRIVATE void _sfetch_thread_join(_sfetch_thread_t* thread) {
    SOKOL_ASSERT(thread);
    if (thread->valid) {
        pthread_mutex_lock(&thread->incoming_mutex);
        _sfetch_thread_request_stop(thread);
        pthread_cond_signal(&thread->incoming_cond);
        pthread_mutex_unlock(&thread->incoming_mutex);
        pthread_join(thread->thread, 0);
        thread->valid = false;
    }
    pthread_mutex_destroy(&thread->stop_mutex);
    pthread_mutex_destroy(&thread->running_mutex);
    pthread_mutex_destroy(&thread->incoming_mutex);
    pthread_mutex_destroy(&thread->outgoing_mutex);
    pthread_cond_destroy(&thread->incoming_cond);
}

/* called when the thread-func is entered, this blocks the thread func until
   the _sfetch_thread_t object is fully initialized
*/
_SOKOL_PRIVATE void _sfetch_thread_entered(_sfetch_thread_t* thread) {
    pthread_mutex_lock(&thread->running_mutex);
}

/* called by the thread-func right before it is left */
_SOKOL_PRIVATE void _sfetch_thread_leaving(_sfetch_thread_t* thread) {
    pthread_mutex_unlock(&thread->running_mutex);
}

_SOKOL_PRIVATE void _sfetch_thread_enqueue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming, _sfetch_ring_t* src) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->buf);
    SOKOL_ASSERT(src && src->buf);
    if (!_sfetch_ring_empty(src)) {
        pthread_mutex_lock(&thread->incoming_mutex);
        while (!_sfetch_ring_full(incoming) && !_sfetch_ring_empty(src)) {
            _sfetch_ring_enqueue(incoming, _sfetch_ring_dequeue(src));
        }
        pthread_cond_signal(&thread->incoming_cond);
        pthread_mutex_unlock(&thread->incoming_mutex);
    }
}

_SOKOL_PRIVATE uint32_t _sfetch_thread_dequeue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->buf);
    pthread_mutex_lock(&thread->incoming_mutex);
    while (_sfetch_ring_empty(incoming) && !thread->stop_requested) {
        pthread_cond_wait(&thread->incoming_cond, &thread->incoming_mutex);
    }
    uint32_t item = 0;
    if (!thread->stop_requested) {
        item = _sfetch_ring_dequeue(incoming);
    }
    pthread_mutex_unlock(&thread->incoming_mutex);
    return item;
}

_SOKOL_PRIVATE bool _sfetch_thread_enqueue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, uint32_t item) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->buf);
    SOKOL_ASSERT(0 != item);
    pthread_mutex_lock(&thread->outgoing_mutex);
    bool result = false;
    if (!_sfetch_ring_full(outgoing)) {
        _sfetch_ring_enqueue(outgoing, item);
    }
    pthread_mutex_unlock(&thread->outgoing_mutex);
    return result;
}

_SOKOL_PRIVATE void _sfetch_thread_dequeue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, _sfetch_ring_t* dst) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->buf);
    SOKOL_ASSERT(dst && dst->buf);
    pthread_mutex_lock(&thread->outgoing_mutex);
    while (!_sfetch_ring_full(dst) && !_sfetch_ring_empty(outgoing)) {
        _sfetch_ring_enqueue(dst, _sfetch_ring_dequeue(outgoing));
    }
    pthread_mutex_unlock(&thread->outgoing_mutex);
}
#endif /* _SFETCH_PLATFORM_POSIX */

#if _SFETCH_PLATFORM_WINDOWS
_SOKOL_PRIVATE bool _sfetch_win32_utf8_to_wide(const char* src, wchar_t* dst, int dst_num_bytes) {
    SOKOL_ASSERT(src && dst && (dst_num_bytes > 1));
    memset(dst, 0, dst_num_bytes);
    const int dst_chars = dst_num_bytes / sizeof(wchar_t);
    const int dst_needed = MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0);
    if ((dst_needed > 0) && (dst_needed < dst_chars)) {
        MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_chars);
        return true;
    }
    else {
        /* input string doesn't fit into destination buffer */
        return false;
    }
}

_SOKOL_PRIVATE _sfetch_file_handle_t _sfetch_file_open(const _sfetch_path_t* path) {
    wchar_t w_path[SFETCH_MAX_PATH];
    if (!_sfetch_win32_utf8_to_wide(path->buf, w_path, sizeof(w_path))) {
        SOKOL_LOG("_sfetch_file_open: error converting UTF-8 path to wide string");
        return 0;
    }
    _sfetch_file_handle_t h = CreateFileW(
        w_path,                 /* lpFileName */
        GENERIC_READ,           /* dwDesiredAccess */
        FILE_SHARE_READ,        /* dwShareMode */
        NULL,                   /* lpSecurityAttributes */
        OPEN_EXISTING,          /* dwCreationDisposition */
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,    /* dwFlagsAndAttributes */
        NULL);                  /* hTemplateFile */
    return h;
}

_SOKOL_PRIVATE void _sfetch_file_close(_sfetch_file_handle_t h) {
    CloseHandle(h);
}

_SOKOL_PRIVATE bool _sfetch_file_handle_valid(_sfetch_file_handle_t h) {
    return h != _SFETCH_INVALID_FILE_HANDLE;
}

_SOKOL_PRIVATE uint64_t _sfetch_file_size(_sfetch_file_handle_t h) {
    LARGE_INTEGER file_size;
    file_size.QuadPart = 0;
    GetFileSizeEx(h, &file_size);
    return file_size.QuadPart;
}

_SOKOL_PRIVATE bool _sfetch_file_read(_sfetch_file_handle_t h, uint64_t offset, uint64_t num_bytes, void* ptr) {
    LARGE_INTEGER offset_li;
    offset_li.QuadPart = offset;
    BOOL seek_res = SetFilePointerEx(h, offset_li, NULL, FILE_BEGIN);
    if (seek_res) {
        DWORD bytes_read = 0;
        BOOL read_res = ReadFile(h, ptr, (DWORD)num_bytes, &bytes_read, NULL);
        return read_res && (bytes_read == num_bytes);
    }
    else {
        return false;
    }
}

_SOKOL_PRIVATE bool _sfetch_thread_init(_sfetch_thread_t* thread, _sfetch_thread_func_t thread_func, void* thread_arg) {
    SOKOL_ASSERT(thread && !thread->valid && !thread->stop_requested);

    thread->incoming_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    SOKOL_ASSERT(NULL != thread->incoming_event);
    InitializeCriticalSection(&thread->incoming_critsec);
    InitializeCriticalSection(&thread->outgoing_critsec);
    InitializeCriticalSection(&thread->running_critsec);
    InitializeCriticalSection(&thread->stop_critsec);

    EnterCriticalSection(&thread->running_critsec);
    const SIZE_T stack_size = 512 * 1024;
    thread->thread = CreateThread(NULL, 512*1024, thread_func, thread_arg, 0, NULL);
    thread->valid = (NULL != thread->thread);
    LeaveCriticalSection(&thread->running_critsec);
    return thread->valid;
}

_SOKOL_PRIVATE void _sfetch_thread_request_stop(_sfetch_thread_t* thread) {
    EnterCriticalSection(&thread->stop_critsec);
    thread->stop_requested = true;
    LeaveCriticalSection(&thread->stop_critsec);
}

_SOKOL_PRIVATE bool _sfetch_thread_stop_requested(_sfetch_thread_t* thread) {
    EnterCriticalSection(&thread->stop_critsec);
    bool stop_requested = thread->stop_requested;
    LeaveCriticalSection(&thread->stop_critsec);
    return stop_requested;
}

_SOKOL_PRIVATE void _sfetch_thread_join(_sfetch_thread_t* thread) {
    if (thread->valid) {
        EnterCriticalSection(&thread->incoming_critsec);
        _sfetch_thread_request_stop(thread);
        BOOL set_event_res = SetEvent(thread->incoming_event);
        SOKOL_ASSERT(set_event_res);
        LeaveCriticalSection(&thread->incoming_critsec);
        WaitForSingleObject(thread->thread, INFINITE);
        CloseHandle(thread->thread);
        thread->valid = false;
    }
    CloseHandle(thread->incoming_event);
    DeleteCriticalSection(&thread->stop_critsec);
    DeleteCriticalSection(&thread->running_critsec);
    DeleteCriticalSection(&thread->outgoing_critsec);
    DeleteCriticalSection(&thread->incoming_critsec);
}

_SOKOL_PRIVATE void _sfetch_thread_entered(_sfetch_thread_t* thread) {
    EnterCriticalSection(&thread->running_critsec);
}

/* called by the thread-func right before it is left */
_SOKOL_PRIVATE void _sfetch_thread_leaving(_sfetch_thread_t* thread) {
    LeaveCriticalSection(&thread->running_critsec);
}

_SOKOL_PRIVATE void _sfetch_thread_enqueue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming, _sfetch_ring_t* src) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->buf);
    SOKOL_ASSERT(src && src->buf);
    if (!_sfetch_ring_empty(src)) {
        EnterCriticalSection(&thread->incoming_critsec);
        while (!_sfetch_ring_full(incoming) && !_sfetch_ring_empty(src)) {
            _sfetch_ring_enqueue(incoming, _sfetch_ring_dequeue(src));
        }
        LeaveCriticalSection(&thread->incoming_critsec);
        BOOL set_event_res = SetEvent(thread->incoming_event);
        SOKOL_ASSERT(set_event_res);
    }
}

_SOKOL_PRIVATE uint32_t _sfetch_thread_dequeue_incoming(_sfetch_thread_t* thread, _sfetch_ring_t* incoming) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(incoming && incoming->buf);
    EnterCriticalSection(&thread->incoming_critsec);
    while (_sfetch_ring_empty(incoming) && !thread->stop_requested) {
        LeaveCriticalSection(&thread->incoming_critsec);
        WaitForSingleObject(&thread->incoming_event, INFINITE);
        EnterCriticalSection(&thread->incoming_critsec);
    }
    uint32_t item = 0;
    if (!thread->stop_requested) {
        item = _sfetch_ring_dequeue(incoming);
    }
    LeaveCriticalSection(&thread->incoming_critsec);
    return item;
}

_SOKOL_PRIVATE bool _sfetch_thread_enqueue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, uint32_t item) {
    /* called from thread function */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->buf);
    EnterCriticalSection(&thread->outgoing_critsec);
    bool result = false;
    if (!_sfetch_ring_full(outgoing)) {
        _sfetch_ring_enqueue(outgoing, item);
    }
    LeaveCriticalSection(&thread->outgoing_critsec);
    return result;
}

_SOKOL_PRIVATE void _sfetch_thread_dequeue_outgoing(_sfetch_thread_t* thread, _sfetch_ring_t* outgoing, _sfetch_ring_t* dst) {
    /* called from user thread */
    SOKOL_ASSERT(thread && thread->valid);
    SOKOL_ASSERT(outgoing && outgoing->buf);
    SOKOL_ASSERT(dst && dst->buf);
    EnterCriticalSection(&thread->outgoing_critsec);
    while (!_sfetch_ring_full(dst) && !_sfetch_ring_empty(outgoing)) {
        _sfetch_ring_enqueue(dst, _sfetch_ring_dequeue(outgoing));
    }
    LeaveCriticalSection(&thread->outgoing_critsec);
}
#endif /* _SFETCH_PLATFORM_WINDOWS */

/*=== IO CHANNEL implementation ==============================================*/

/* per-channel request handler for native platforms accessing the local filesystem */
#if _SFETCH_HAS_THREADS
_SOKOL_PRIVATE void _sfetch_request_handler(_sfetch_t* ctx, uint32_t slot_id) {
    sfetch_state_t state;
    _sfetch_path_t* path;
    _sfetch_item_thread_t* thread;
    {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (!item) {
            return;
        }
        state = item->state;
        SOKOL_ASSERT((state == SFETCH_STATE_OPENING) || (state == SFETCH_STATE_FETCHING) || (state == SFETCH_STATE_PAUSED));
        path = &item->path;
        thread = &item->thread;
    }
    if (thread->failed) {
        // FIXME: should this be a hard error?
        return;
    }
    if (state == SFETCH_STATE_OPENING) {
        SOKOL_ASSERT(!_sfetch_file_handle_valid(thread->file_handle));
        SOKOL_ASSERT(path->buf[0]);
        SOKOL_ASSERT(thread->content_size == 0);
        SOKOL_ASSERT(thread->fetched_size == 0);
        SOKOL_ASSERT(thread->chunk_size == 0);
        thread->file_handle = _sfetch_file_open(path);
        if (_sfetch_file_handle_valid(thread->file_handle)) {
            thread->content_size = _sfetch_file_size(thread->file_handle);
            /* if we already have a buffer associated with the request, skip
                the OPENED state (which only exists so the user code can look at the
                file size and provide a matching buffer), and instead start fetching
                data data immediately
            */
            if (thread->buffer.ptr) {
                state = SFETCH_STATE_FETCHING;
            }
        }
        else {
            thread->failed = true;
            thread->finished = true;
        }
    }
    /* may fall through from OPENING if a buffer was provided upfront */
    if (state == SFETCH_STATE_FETCHING) {
        SOKOL_ASSERT(_sfetch_file_handle_valid(thread->file_handle));
        SOKOL_ASSERT(thread->content_size > thread->fetched_size);
        if ((thread->buffer.ptr == 0) || (thread->buffer.size == 0)) {
            thread->failed = true;
        }
        else {
            uint64_t bytes_to_read = thread->content_size - thread->fetched_size;
            if (bytes_to_read > thread->buffer.size) {
                bytes_to_read = thread->buffer.size;
            }
            const uint64_t offset = thread->fetched_size;
            if (_sfetch_file_read(thread->file_handle, offset, bytes_to_read, thread->buffer.ptr)) {
                thread->chunk_size = bytes_to_read;
                thread->fetched_size += bytes_to_read;
            }
            else {
                thread->failed = true;
            }
        }
        if (thread->failed || (thread->fetched_size >= thread->content_size)) {
            _sfetch_file_close(thread->file_handle);
            thread->file_handle = 0;
            thread->finished = true;
        }
    }
    /* ignore items in PAUSED state */
}

#if _SFETCH_PLATFORM_WINDOWS
_SOKOL_PRIVATE DWORD WINAPI _sfetch_channel_thread_func(LPVOID arg) {
#else
_SOKOL_PRIVATE void* _sfetch_channel_thread_func(void* arg) {
#endif
    _sfetch_channel_t* chn = (_sfetch_channel_t*) arg;
    _sfetch_thread_entered(&chn->thread);
    while (!_sfetch_thread_stop_requested(&chn->thread)) {
        /* block until work arrives */
        uint32_t slot_id = _sfetch_thread_dequeue_incoming(&chn->thread, &chn->thread_incoming);
        /* slot_id will be invalid if the thread was woken up to join */
        if (!_sfetch_thread_stop_requested(&chn->thread)) {
            SOKOL_ASSERT(0 != slot_id);
            chn->request_handler(chn->ctx, slot_id);
            SOKOL_ASSERT(!_sfetch_ring_full(&chn->thread_outgoing));
            _sfetch_thread_enqueue_outgoing(&chn->thread, &chn->thread_outgoing, slot_id);
        }
    }
    _sfetch_thread_leaving(&chn->thread);
    return 0;
}
#endif /* _SFETCH_HAS_THREADS */

#if _SFETCH_PLATFORM_EMSCRIPTEN
/*=== embedded Javascript helper functions ===================================*/
EM_JS(void, sfetch_js_send_head_request, (uint32_t slot_id, const char* path_cstr), {
    var path_str = UTF8ToString(path_cstr);
    var req = new XMLHttpRequest();
    req.open('HEAD', path_str);
    req.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            if (this.status == 200) {
                var content_length = this.getResponseHeader('Content-Length');
                __sfetch_emsc_head_response(slot_id, content_length);
            }
            else {
                __sfetch_emsc_failed(slot_id);
            }
        }
    };
    req.send();
});

EM_JS(void, sfetch_js_send_range_request, (uint32_t slot_id, const char* path_cstr, int offset, int num_bytes, int content_length, void* buf_ptr), {
    var path_str = UTF8ToString(path_cstr);
    var req = new XMLHttpRequest();
    req.open('GET', path_str);
    req.responseType = 'arraybuffer';
    var need_range_request = !((offset == 0) && (num_bytes == content_length));
    if (need_range_request) {
        req.setRequestHeader('Range', 'bytes='+offset+'-'+(offset+num_bytes));
    }
    req.onreadystatechange = function() {
        if (this.readyState == this.DONE) {
            if ((this.status == 206) || ((this.status == 200) && !need_range_request)) {
                var u8_array = new Uint8Array(req.response);
                HEAPU8.set(u8_array, buf_ptr);
                __sfetch_emsc_range_response(slot_id, num_bytes);
            }
            else {
                __sfetch_emsc_failed(slot_id);
            }
        }
    };
    req.send();
});

/*=== emscripten specific C helper functions =================================*/
#ifdef __cplusplus
extern "C" {
#endif
void _sfetch_emsc_send_range_request(uint32_t slot_id, _sfetch_item_t* item) {
    SOKOL_ASSERT(item->thread.content_size > item->thread.fetched_size);
    if ((item->thread.buffer.ptr == 0) || (item->thread.buffer.size == 0)) {
        item->thread.failed = true;
    }
    else {
        /* send a regular HTTP range request to fetch the next chunk of data
            FIXME: need to figure out a way to use 64-bit sizes here
        */
        uint32_t bytes_to_read = item->thread.content_size - item->thread.fetched_size;
        if (bytes_to_read > item->thread.buffer.size) {
            bytes_to_read = item->thread.buffer.size;
        }
        const uint32_t offset = item->thread.fetched_size;
        sfetch_js_send_range_request(slot_id, item->path.buf, offset, bytes_to_read, item->thread.content_size, item->thread.buffer.ptr);
    }
}

/* called by JS when the initial HEAD request finished successfully */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_head_response(uint32_t slot_id, uint32_t content_length) {
    //printf("sfetch_emsc_head_response(slot_id=%d content_length=%d)\n", slot_id, content_length);
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.content_size = content_length;
            if (item->thread.buffer.ptr) {
                /* if a buffer was provided, continue immediate with fetching the first
                    chunk, instead of passing the request back to the channel
                */
                _sfetch_emsc_send_range_request(slot_id, item);
            }
            else {
                /* if no buffer was provided upfront pass back to the channel so
                    that the response-user-callback can be called
                */
                _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
            }
        }
    }
}

/* called by JS when a followup GET request finished successfully */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_range_response(uint32_t slot_id, uint32_t num_bytes_read) {
    //printf("sfetch_emsc_range_response(slot_id=%d, num_bytes_read=%d)\n", slot_id, num_bytes_read);
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.chunk_size = num_bytes_read;
            item->thread.fetched_size += num_bytes_read;
            if (item->thread.fetched_size >= item->thread.content_size) {
                item->thread.finished = true;
            }
            _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
        }
    }
}

/* called by JS when an error occured */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_failed(uint32_t slot_id) {
    //printf("sfetch_emsc_failed(slot_id=%d)\n", slot_id);
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.failed = true;
            item->thread.finished = true;
            _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
        }
    }
}
#ifdef __cplusplus
} /* extern "C" */
#endif

_SOKOL_PRIVATE void _sfetch_request_handler(_sfetch_t* ctx, uint32_t slot_id) {
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
    if (!item) {
        return;
    }
    if (item->thread.failed) {
        return;
    }
    if (item->state == SFETCH_STATE_OPENING) {
        SOKOL_ASSERT(item->path.buf[0]);
        /* We need to query the content-size first with a separate HEAD request,
            no matter if a buffer was provided or not (because sending a too big
            range request speculatively doesnt work). With the response, we can
            also check whether the server actually supports range requests
        */
        sfetch_js_send_head_request(slot_id, item->path.buf);
        /* see _sfetch_emsc_head_response() for the rest... */
    }
    if (item->state == SFETCH_STATE_FETCHING) {
        _sfetch_emsc_send_range_request(slot_id, item);
    }
    if (item->state == SFETCH_STATE_PAUSED) {
        /* paused items are just passed-through to the outgoing queue */
        _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
    }
    if (item->thread.failed) {
        item->thread.finished = true;
    }
}
#endif

_SOKOL_PRIVATE void _sfetch_channel_discard(_sfetch_channel_t* chn) {
    SOKOL_ASSERT(chn);
    #if _SFETCH_HAS_THREADS
        if (chn->valid) {
            _sfetch_thread_join(&chn->thread);
        }
        _sfetch_ring_discard(&chn->thread_incoming);
        _sfetch_ring_discard(&chn->thread_outgoing);
    #endif
    _sfetch_ring_discard(&chn->free_lanes);
    _sfetch_ring_discard(&chn->user_sent);
    _sfetch_ring_discard(&chn->user_incoming);
    _sfetch_ring_discard(&chn->user_outgoing);
    _sfetch_ring_discard(&chn->free_lanes);
    chn->valid = false;
}

_SOKOL_PRIVATE bool _sfetch_channel_init(_sfetch_channel_t* chn, _sfetch_t* ctx, uint32_t num_items, uint32_t num_lanes, void (*request_handler)(_sfetch_t* ctx, uint32_t)) {
    SOKOL_ASSERT(chn && (num_items > 0) && request_handler);
    SOKOL_ASSERT(!chn->valid);
    bool valid = true;
    chn->request_handler = request_handler;
    chn->ctx = ctx;
    valid &= _sfetch_ring_init(&chn->free_lanes, num_lanes);
    for (uint32_t lane = 0; lane < num_lanes; lane++) {
        _sfetch_ring_enqueue(&chn->free_lanes, lane);
    }
    valid &= _sfetch_ring_init(&chn->user_sent, num_items);
    valid &= _sfetch_ring_init(&chn->user_incoming, num_lanes);
    valid &= _sfetch_ring_init(&chn->user_outgoing, num_lanes);
    #if _SFETCH_HAS_THREADS
        valid &= _sfetch_ring_init(&chn->thread_incoming, num_lanes);
        valid &= _sfetch_ring_init(&chn->thread_outgoing, num_lanes);
    #endif
    if (valid) {
        chn->valid = true;
        #if _SFETCH_HAS_THREADS
        _sfetch_thread_init(&chn->thread, _sfetch_channel_thread_func, chn);
        #endif
        return true;
    }
    else {
        _sfetch_channel_discard(chn);
        return false;
    }
}

/* put a request into the channels sent-queue, this is where all new requests
   are stored until a lane becomes free.
*/
_SOKOL_PRIVATE bool _sfetch_channel_send(_sfetch_channel_t* chn, uint32_t slot_id) {
    SOKOL_ASSERT(chn && chn->valid);
    if (!_sfetch_ring_full(&chn->user_sent)) {
        _sfetch_ring_enqueue(&chn->user_sent, slot_id);
        return true;
    }
    else {
        SOKOL_LOG("sfetch_send: user_sent queue is full)");
        return false;
    }
}

/* per-frame channel stuff: move requests in and out of the IO threads, call response callbacks */
_SOKOL_PRIVATE void _sfetch_channel_dowork(_sfetch_channel_t* chn, _sfetch_pool_t* pool) {

    /* move items from sent- to incoming-queue permitting free lanes */
    const uint32_t num_sent = _sfetch_ring_count(&chn->user_sent);
    const uint32_t avail_lanes = _sfetch_ring_count(&chn->free_lanes);
    const uint32_t num_move = (num_sent < avail_lanes) ? num_sent : avail_lanes;
    for (uint32_t i = 0; i < num_move; i++) {
        const uint32_t slot_id = _sfetch_ring_dequeue(&chn->user_sent);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(pool, slot_id);
        SOKOL_ASSERT(item);
        item->lane = _sfetch_ring_dequeue(&chn->free_lanes);
        _sfetch_ring_enqueue(&chn->user_incoming, slot_id);
    }

    /* prepare incoming items for being moved into the IO thread */
    const uint32_t num_incoming = _sfetch_ring_count(&chn->user_incoming);
    for (uint32_t i = 0; i < num_incoming; i++) {
        const uint32_t slot_id = _sfetch_ring_peek(&chn->user_incoming, i);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(pool, slot_id);
        SOKOL_ASSERT(item);
        SOKOL_ASSERT(item->state != SFETCH_STATE_INITIAL);
        SOKOL_ASSERT(item->state != SFETCH_STATE_OPENING);
        SOKOL_ASSERT(item->state != SFETCH_STATE_FETCHING);
        /* transfer input params from user- to thread-data */
        item->thread.buffer = item->user.buffer;
        if (item->user.pause) {
            item->state = SFETCH_STATE_PAUSED;
            item->user.pause = false;
        }
        if (item->user.cont) {
            if (item->state == SFETCH_STATE_PAUSED) {
                item->state = SFETCH_STATE_FETCHED;
            }
            item->user.cont = false;
        }
        switch (item->state) {
            case SFETCH_STATE_ALLOCATED:
                item->state = SFETCH_STATE_OPENING;
                break;
            case SFETCH_STATE_OPENED:
            case SFETCH_STATE_FETCHED:
                item->state = SFETCH_STATE_FETCHING;
                break;
            default: break;
        }
    }

    #if _SFETCH_HAS_THREADS
        /* move new items into the IO threads and processed items out of IO threads */
        _sfetch_thread_enqueue_incoming(&chn->thread, &chn->thread_incoming, &chn->user_incoming);
        _sfetch_thread_dequeue_outgoing(&chn->thread, &chn->thread_outgoing, &chn->user_outgoing);
    #else
        /* without threading just directly dequeue items from the user_incoming queue and
           call the request handler, the user_outgoing queue will be filled as the
           asynchronous HTTP requests sent by the request handler are completed
        */
        while (!_sfetch_ring_empty(&chn->user_incoming)) {
            uint32_t slot_id = _sfetch_ring_dequeue(&chn->user_incoming);
            _sfetch_request_handler(chn->ctx, slot_id);
        }
    #endif

    /* drain the outgoing queue, prepare items for invoking the response
       callback, and finally call the response callback, free finished items
    */
    sfetch_response_t response;
    memset(&response, 0, sizeof(response));
    while (!_sfetch_ring_empty(&chn->user_outgoing)) {
        const uint32_t slot_id = _sfetch_ring_dequeue(&chn->user_outgoing);
        SOKOL_ASSERT(slot_id);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(pool, slot_id);
        SOKOL_ASSERT(item && item->callback);
        SOKOL_ASSERT(item->state != SFETCH_STATE_INITIAL);
        SOKOL_ASSERT(item->state != SFETCH_STATE_ALLOCATED);
        SOKOL_ASSERT(item->state != SFETCH_STATE_OPENED);
        SOKOL_ASSERT(item->state != SFETCH_STATE_FETCHED);
        /* transfer output params from thread- to user-data */
        item->user.content_size = item->thread.content_size;
        item->user.fetched_size = item->thread.fetched_size;
        item->user.chunk_size  = item->thread.chunk_size;
        if (item->thread.finished) {
            item->user.finished = true;
        }
        /* state transition */
        if (item->thread.failed) {
            item->state = SFETCH_STATE_FAILED;
        }
        else {
            switch (item->state) {
                case SFETCH_STATE_OPENING:
                    /* if the request already had a buffer provided, the
                       OPENING state already has fetched data and we shortcut
                       to the first FETCHED state to shorten the time a request occupies
                       a lane, otherwise, invoke the callback with OPENED state
                       so it can provide a buffer
                    */
                    if (item->user.fetched_size > 0) {
                        item->state = SFETCH_STATE_FETCHED;
                    }
                    else {
                        item->state = SFETCH_STATE_OPENED;
                    }
                    break;
                case SFETCH_STATE_FETCHING:
                    item->state = SFETCH_STATE_FETCHED;
                    break;
                default:
                    break;
            }
        }
        /* invoke response callback */
        response.handle.id = slot_id;
        response.finished = item->user.finished;
        response.state = item->state;
        response.channel = item->channel;
        response.lane = item->lane;
        response.path = item->path.buf;
        response.user_data = item->user.user_data;
        response.content_size = item->user.content_size;
        response.chunk_offset = item->user.fetched_size - item->user.chunk_size;
        response.chunk.ptr = item->user.buffer.ptr;
        response.chunk.size = item->user.chunk_size;
        item->callback(response);

        /* when the request is finish, free the lane for another request,
           otherwise feed it back into the incoming queue
        */
        if (item->user.finished) {
            _sfetch_ring_enqueue(&chn->free_lanes, item->lane);
            _sfetch_pool_item_free(pool, slot_id);
        }
        else {
            _sfetch_ring_enqueue(&chn->user_incoming, slot_id);
        }
    }
}

/*=== private high-level functions ===========================================*/
_SOKOL_PRIVATE bool _sfetch_validate_request(_sfetch_t* ctx, const sfetch_request_t* req) {
    #if defined(SOKOL_DEBUG)
    if (req->channel >= ctx->desc.num_channels) {
        SOKOL_LOG("_sfetch_validate_request: request.num_channels too big!");
        return false;
    }
    if (!req->path) {
        SOKOL_LOG("_sfetch_validate_request: request.path is null!");
        return false;
    }
    if (strlen(req->path) >= (SFETCH_MAX_PATH-1)) {
        SOKOL_LOG("_sfetch_validate_request: request.path is too long (must be < SFETCH_MAX_PATH-1)");
        return false;
    }
    if (!req->callback) {
        SOKOL_LOG("_sfetch_validate_request: request.callback missing");
        return false;
    }
    if (req->user_data && (req->user_data_size == 0)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data is set, but req.user_data_size is null");
        return false;
    }
    if (!req->user_data && (req->user_data_size > 0)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data is null, but req.user_data_size is not");
        return false;
    }
    if (req->user_data_size > SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) {
        SOKOL_LOG("_sfetch_validate_request: request.user_data_size is too big (see SFETCH_MAX_USERDATA_UINT64");
        return false;
    }
    #endif
    return true;
}

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL void sfetch_setup(const sfetch_desc_t* desc) {
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->_start_canary == 0) && (desc->_end_canary == 0));
    SOKOL_ASSERT(0 == _sfetch);
    _sfetch = SOKOL_MALLOC(sizeof(_sfetch_t));
    SOKOL_ASSERT(_sfetch);
    memset(_sfetch, 0, sizeof(_sfetch_t));
    _sfetch_t* ctx = _sfetch_ctx();
    ctx->desc = *desc;
    ctx->setup = true;
    ctx->valid = true;

    /* replace zero-init items with default values */
    ctx->desc.max_requests = _sfetch_def(ctx->desc.max_requests, 128);
    ctx->desc.num_channels = _sfetch_def(ctx->desc.num_channels, 1);
    ctx->desc.num_lanes = _sfetch_def(ctx->desc.num_lanes, 16);
    if (ctx->desc.num_channels > SFETCH_MAX_CHANNELS) {
        ctx->desc.num_channels = SFETCH_MAX_CHANNELS;
        SOKOL_LOG("sfetch_setup: clamping num_channels to SFETCH_MAX_CHANNELS");
    }

    /* setup the global request item pool */
    ctx->valid &= _sfetch_pool_init(&ctx->pool, ctx->desc.max_requests);

    /* setup IO channels (one thread per channel) */
    for (uint32_t i = 0; i < ctx->desc.num_channels; i++) {
        ctx->valid &= _sfetch_channel_init(&ctx->chn[i], ctx, ctx->desc.max_requests, ctx->desc.num_lanes, _sfetch_request_handler);
    }
}

SOKOL_API_IMPL void sfetch_shutdown(void) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->setup);
    ctx->valid = false;
    /* IO threads must be shutdown first */
    for (uint32_t i = 0; i < ctx->desc.num_channels; i++) {
        if (ctx->chn[i].valid) {
            _sfetch_channel_discard(&ctx->chn[i]);
        }
    }
    _sfetch_pool_discard(&ctx->pool);
    ctx->setup = false;
    SOKOL_FREE(ctx);
    _sfetch = 0;
}

SOKOL_API_IMPL bool sfetch_valid(void) {
    _sfetch_t* ctx = _sfetch_ctx();
    return ctx && ctx->valid;
}

SOKOL_API_IMPL sfetch_desc_t sfetch_desc(void) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    return ctx->desc;
}

SOKOL_API_IMPL int sfetch_max_userdata_bytes(void) {
    return SFETCH_MAX_USERDATA_UINT64 * 8;
}

SOKOL_API_IMPL int sfetch_max_path(void) {
    return SFETCH_MAX_PATH;
}

SOKOL_API_IMPL bool sfetch_handle_valid(sfetch_handle_t h) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    /* shortcut invalid handle */
    if (h.id == 0) {
        return false;
    }
    return 0 != _sfetch_pool_item_lookup(&ctx->pool, h.id);
}

SOKOL_API_IMPL sfetch_handle_t sfetch_send(const sfetch_request_t* request) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->setup);
    SOKOL_ASSERT(request && (request->_start_canary == 0) && (request->_end_canary == 0));

    const sfetch_handle_t invalid_handle = _sfetch_make_handle(0);
    if (!ctx->valid) {
        return invalid_handle;
    }
    if (!_sfetch_validate_request(ctx, request)) {
        return invalid_handle;
    }
    SOKOL_ASSERT(request->channel < ctx->desc.num_channels);

    uint32_t slot_id = _sfetch_pool_item_alloc(&ctx->pool, request);
    if (0 == slot_id) {
        SOKOL_LOG("sfetch_send: request pool exhausted (too many active requests)");
        return invalid_handle;
    }
    if (!_sfetch_channel_send(&ctx->chn[request->channel], slot_id)) {
        /* send failed because the channels sent-queue overflowed */
        _sfetch_pool_item_free(&ctx->pool, slot_id);
        return invalid_handle;
    }
    return _sfetch_make_handle(slot_id);
}

SOKOL_API_IMPL void sfetch_dowork(void) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->setup);
    if (!ctx->valid) {
        return;
    }
    /* we're pumping each channel 2x so that unfinished request items coming out the
       IO threads can be moved back into the IO-thread immediately without
       having to wait a frame
     */
    for (int pass = 0; pass < 2; pass++) {
        for (uint32_t chn_index = 0; chn_index < ctx->desc.num_channels; chn_index++) {
            _sfetch_channel_dowork(&ctx->chn[chn_index], &ctx->pool);
        }
    }
}

SOKOL_API_IMPL void sfetch_set_buffer(sfetch_handle_t h, const sfetch_buffer_t* buf) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        // FIXME: should we simply allow overwriting if a buffer was already
        // set? This would allow more 'streaming strategies' like ping-ponging
        // between separate buffers, but also encourage memory leaks!
        item->user.buffer = *buf;
    }
}

SOKOL_API_IMPL void sfetch_pause(sfetch_handle_t h) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        item->user.pause = true;
        item->user.cont = false;
    }
}

SOKOL_API_IMPL void sfetch_continue(sfetch_handle_t h) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        item->user.cont = true;
        item->user.pause = false;
    }
}
#endif /* SOKOL_IMPL */

