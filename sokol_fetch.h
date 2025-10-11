#if defined(SOKOL_IMPL) && !defined(SOKOL_FETCH_IMPL)
#define SOKOL_FETCH_IMPL
#endif
#ifndef SOKOL_FETCH_INCLUDED
/*
    sokol_fetch.h -- asynchronous data loading/streaming

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_FETCH_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_ASSERT(c)             - your own assert macro (default: assert(c))
    SOKOL_UNREACHABLE()         - a guard macro for unreachable code (default: assert(false))
    SOKOL_FETCH_API_DECL        - public function declaration prefix (default: extern)
    SOKOL_API_DECL              - same as SOKOL_FETCH_API_DECL
    SOKOL_API_IMPL              - public function implementation prefix (default: -)
    SFETCH_MAX_PATH             - max length of UTF-8 filesystem path / URL (default: 1024 bytes)
    SFETCH_MAX_USERDATA_UINT64  - max size of embedded userdata in number of uint64_t, userdata
                                  will be copied into an 8-byte aligned memory region associated
                                  with each in-flight request, default value is 16 (== 128 bytes)
    SFETCH_MAX_CHANNELS         - max number of IO channels (default is 16, also see sfetch_desc_t.num_channels)

    If sokol_fetch.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_FETCH_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    NOTE: The following documentation talks a lot about "IO threads". Actual
    threads are only used on platforms where threads are available. The web
    version (emscripten/wasm) doesn't use POSIX-style threads, but instead
    asynchronous Javascript calls chained together by callbacks. The actual
    source code differences between the two approaches have been kept to
    a minimum though.

    FEATURE OVERVIEW
    ================

    - Asynchronously load complete files, or stream files incrementally via
      HTTP (on web platform), or the local file system (on native platforms)

    - Request / response-callback model, user code sends a request
      to initiate a file-load, sokol_fetch.h calls the response callback
      on the same thread when data is ready or user-code needs
      to respond otherwise

    - Not limited to the main-thread or a single thread: A sokol-fetch
      "context" can live on any thread, and multiple contexts
      can operate side-by-side on different threads.

    - Memory management for data buffers is under full control of user code.
      sokol_fetch.h won't allocate memory after it has been setup.

    - Automatic rate-limiting guarantees that only a maximum number of
      requests is processed at any one time, allowing a zero-allocation
      model, where all data is streamed into fixed-size, pre-allocated
      buffers.

    - Active Requests can be paused, continued and cancelled from anywhere
      in the user-thread which sent this request.


    TL;DR EXAMPLE CODE
    ==================
    This is the most-simple example code to load a single data file with a
    known maximum size:

    (1) initialize sokol-fetch with default parameters (but NOTE that the
        default setup parameters provide a safe-but-slow "serialized"
        operation). In order to see any logging output in case or errors
        you should always provide a logging function
        (such as 'slog_func' from sokol_log.h):

        sfetch_setup(&(sfetch_desc_t){ .logger.func = slog_func });

    (2) send a fetch-request to load a file from the current directory
        into a buffer big enough to hold the entire file content:

        static uint8_t buf[MAX_FILE_SIZE];

        sfetch_send(&(sfetch_request_t){
            .path = "my_file.txt",
            .callback = response_callback,
            .buffer = {
                .ptr = buf,
                .size = sizeof(buf)
            }
        });

        If 'buf' is a value (e.g. an array or struct item), the .buffer item can
        be initialized with the SFETCH_RANGE() helper macro:

        sfetch_send(&(sfetch_request_t){
            .path = "my_file.txt",
            .callback = response_callback,
            .buffer = SFETCH_RANGE(buf)
        });

    (3) write a 'response-callback' function, this will be called whenever
        the user-code must respond to state changes of the request
        (most importantly when data has been loaded):

        void response_callback(const sfetch_response_t* response) {
            if (response->fetched) {
                // data has been loaded, and is available via the
                // sfetch_range_t struct item 'data':
                const void* ptr = response->data.ptr;
                size_t num_bytes = response->data.size;
            }
            if (response->finished) {
                // the 'finished'-flag is the catch-all flag for when the request
                // is finished, no matter if loading was successful or failed,
                // so any cleanup-work should happen here...
                ...
                if (response->failed) {
                    // 'failed' is true in (addition to 'finished') if something
                    // went wrong (file doesn't exist, or less bytes could be
                    // read from the file than expected)
                }
            }
        }

    (4) pump the sokol-fetch message queues, and invoke response callbacks
        by calling:

        sfetch_dowork();

        In an event-driven app this should be called in the event loop. If you
        use sokol-app this would be in your frame_cb function.

    (5) finally, call sfetch_shutdown() at the end of the application:

    There's many other loading-scenarios, for instance one doesn't have to
    provide a buffer upfront, this can also happen in the response callback.

    Or it's possible to stream huge files into small fixed-size buffer,
    complete with pausing and continuing the download.

    It's also possible to improve the 'pipeline throughput' by fetching
    multiple files in parallel, but at the same time limit the maximum
    number of requests that can be 'in-flight'.

    For how this all works, please read the following documentation sections :)


    API DOCUMENTATION
    =================

    void sfetch_setup(const sfetch_desc_t* desc)
    --------------------------------------------
    First call sfetch_setup(const sfetch_desc_t*) on any thread before calling
    any other sokol-fetch functions on the same thread.

    sfetch_setup() takes a pointer to an sfetch_desc_t struct with setup
    parameters. Parameters which should use their default values must
    be zero-initialized:

        - max_requests (uint32_t):
            The maximum number of requests that can be alive at any time, the
            default is 128.

        - num_channels (uint32_t):
            The number of "IO channels" used to parallelize and prioritize
            requests, the default is 1.

        - num_lanes (uint32_t):
            The number of "lanes" on a single channel. Each request which is
            currently 'inflight' on a channel occupies one lane until the
            request is finished. This is used for automatic rate-limiting
            (search below for CHANNELS AND LANES for more details). The
            default number of lanes is 1.

    For example, to setup sokol-fetch for max 1024 active requests, 4 channels,
    and 8 lanes per channel in C99:

        sfetch_setup(&(sfetch_desc_t){
            .max_requests = 1024,
            .num_channels = 4,
            .num_lanes = 8
        });

    sfetch_setup() is the only place where sokol-fetch will allocate memory.

    NOTE that the default setup parameters of 1 channel and 1 lane per channel
    has a very poor 'pipeline throughput' since this essentially serializes
    IO requests (a new request will only be processed when the last one has
    finished), and since each request needs at least one roundtrip between
    the user- and IO-thread the throughput will be at most one request per
    frame. Search for LATENCY AND THROUGHPUT below for more information on
    how to increase throughput.

    NOTE that you can call sfetch_setup() on multiple threads, each thread
    will get its own thread-local sokol-fetch instance, which will work
    independently from sokol-fetch instances on other threads.

    void sfetch_shutdown(void)
    --------------------------
    Call sfetch_shutdown() at the end of the application to stop any
    IO threads and free all memory that was allocated in sfetch_setup().

    sfetch_handle_t sfetch_send(const sfetch_request_t* request)
    ------------------------------------------------------------
    Call sfetch_send() to start loading data, the function takes a pointer to an
    sfetch_request_t struct with request parameters and returns a
    sfetch_handle_t identifying the request for later calls. At least
    a path/URL and callback must be provided:

        sfetch_handle_t h = sfetch_send(&(sfetch_request_t){
            .path = "my_file.txt",
            .callback = my_response_callback
        });

    sfetch_send() will return an invalid handle if no request can be allocated
    from the internal pool because all available request items are 'in-flight'.

    The sfetch_request_t struct contains the following parameters (optional
    parameters that are not provided must be zero-initialized):

        - path (const char*, required)
            Pointer to an UTF-8 encoded C string describing the filesystem
            path or HTTP URL. The string will be copied into an internal data
            structure, and passed "as is" (apart from any required
            encoding-conversions) to fopen(), CreateFileW() or
            the html fetch API call. The maximum length of the string is defined by
            the SFETCH_MAX_PATH configuration define, the default is 1024 bytes
            including the 0-terminator byte.

        - callback (sfetch_callback_t, required)
            Pointer to a response-callback function which is called when the
            request needs "user code attention". Search below for REQUEST
            STATES AND THE RESPONSE CALLBACK for detailed information about
            handling responses in the response callback.

        - channel (uint32_t, optional)
            Index of the IO channel where the request should be processed.
            Channels are used to parallelize and prioritize requests relative
            to each other. Search below for CHANNELS AND LANES for more
            information. The default channel is 0.

        - chunk_size (uint32_t, optional)
            The chunk_size member is used for streaming data incrementally
            in small chunks. After 'chunk_size' bytes have been loaded into
            to the streaming buffer, the response callback will be called
            with the buffer containing the fetched data for the current chunk.
            If chunk_size is 0 (the default), than the whole file will be loaded.
            Please search below for CHUNK SIZE AND HTTP COMPRESSION for
            important information how streaming works if the web server
            is serving compressed data.

        - buffer (sfetch_range_t)
            This is a optional pointer/size pair describing a chunk of memory where
            data will be loaded into (if no buffer is provided upfront, this
            must happen in the response callback). If a buffer is provided,
            it must be big enough to either hold the entire file (if chunk_size
            is zero), or the *uncompressed* data for one downloaded chunk
            (if chunk_size is > 0).

        - user_data (sfetch_range_t)
            The user_data ptr/size range struct describe an optional POD blob
            (plain-old-data) associated with the request which will be copied(!)
            into an internal memory block. The maximum default size of this
            memory block is 128 bytes (but can be overridden by defining
            SFETCH_MAX_USERDATA_UINT64 before including the notification, note
            that this define is in "number of uint64_t", not number of bytes).
            The user-data block is 8-byte aligned, and will be copied via
            memcpy() (so don't put any C++ "smart members" in there).

    NOTE that request handles are strictly thread-local and only unique
    within the thread the handle was created on, and all function calls
    involving a request handle must happen on that same thread.

    bool sfetch_handle_valid(sfetch_handle_t request)
    -------------------------------------------------
    This checks if the provided request handle is valid, and is associated with
    a currently active request. It will return false if:

        - sfetch_send() returned an invalid handle because it couldn't allocate
          a new request from the internal request pool (because they're all
          in flight)
        - the request associated with the handle is no longer alive (because
          it either finished successfully, or the request failed for some
          reason)

    void sfetch_dowork(void)
    ------------------------
    Call sfetch_dowork(void) in regular intervals (for instance once per frame)
    on the same thread as sfetch_setup() to "turn the gears". If you are sending
    requests but never hear back from them in the response callback function, then
    the most likely reason is that you forgot to add the call to sfetch_dowork()
    in the per-frame function.

    sfetch_dowork() roughly performs the following work:

        - any new requests that have been sent with sfetch_send() since the
        last call to sfetch_dowork() will be dispatched to their IO channels
        and assigned a free lane. If all lanes on that channel are occupied
        by requests 'in flight', incoming requests must wait until
        a lane becomes available

        - for all new requests which have been enqueued on a channel which
        don't already have a buffer assigned the response callback will be
        called with (response->dispatched == true) so that the response
        callback can inspect the dynamically assigned lane and bind a buffer
        to the request (search below for CHANNELS AND LANE for more info)

        - a state transition from "user side" to "IO thread side" happens for
        each new request that has been dispatched to a channel.

        - requests dispatched to a channel are either forwarded into that
        channel's worker thread (on native platforms), or cause an HTTP
        request to be sent via an asynchronous fetch() call (on the web
        platform)

        - for all requests which have finished their current IO operation a
        state transition from "IO thread side" to "user side" happens,
        and the response callback is called so that the fetched data
        can be processed.

        - requests which are completely finished (either because the entire
        file content has been loaded, or they are in the FAILED state) are
        freed (this just changes their state in the 'request pool', no actual
        memory is freed)

        - requests which are not yet finished are fed back into the
        'incoming' queue of their channel, and the cycle starts again, this
        only happens for requests which perform data streaming (not load
        the entire file at once).

    void sfetch_cancel(sfetch_handle_t request)
    -------------------------------------------
    This cancels a request in the next sfetch_dowork() call and invokes the
    response callback with (response.failed == true) and (response.finished
    == true) to give user-code a chance to do any cleanup work for the
    request. If sfetch_cancel() is called for a request that is no longer
    alive, nothing bad will happen (the call will simply do nothing).

    void sfetch_pause(sfetch_handle_t request)
    ------------------------------------------
    This pauses an active request in the next sfetch_dowork() call and puts
    it into the PAUSED state. For all requests in PAUSED state, the response
    callback will be called in each call to sfetch_dowork() to give user-code
    a chance to CONTINUE the request (by calling sfetch_continue()). Pausing
    a request makes sense for dynamic rate-limiting in streaming scenarios
    (like video/audio streaming with a fixed number of streaming buffers. As
    soon as all available buffers are filled with download data, downloading
    more data must be prevented to allow video/audio playback to catch up and
    free up empty buffers for new download data.

    void sfetch_continue(sfetch_handle_t request)
    ---------------------------------------------
    Continues a paused request, counterpart to the sfetch_pause() function.

    void sfetch_bind_buffer(sfetch_handle_t request, sfetch_range_t buffer)
    ----------------------------------------------------------------------------------------
    This "binds" a new buffer (as pointer/size pair) to an active request. The
    function *must* be called from inside the response-callback, and there
    must not already be another buffer bound.

    void* sfetch_unbind_buffer(sfetch_handle_t request)
    ---------------------------------------------------
    This removes the current buffer binding from the request and returns
    a pointer to the previous buffer (useful if the buffer was dynamically
    allocated and it must be freed).

    sfetch_unbind_buffer() *must* be called from inside the response callback.

    The usual code sequence to bind a different buffer in the response
    callback might look like this:

        void response_callback(const sfetch_response_t* response) {
            if (response.fetched) {
                ...
                // switch to a different buffer (in the FETCHED state it is
                // guaranteed that the request has a buffer, otherwise it
                // would have gone into the FAILED state
                void* old_buf_ptr = sfetch_unbind_buffer(response.handle);
                free(old_buf_ptr);
                void* new_buf_ptr = malloc(new_buf_size);
                sfetch_bind_buffer(response.handle, new_buf_ptr, new_buf_size);
            }
            if (response.finished) {
                // unbind and free the currently associated buffer,
                // the buffer pointer could be null if the request has failed
                // NOTE that it is legal to call free() with a nullptr,
                // this happens if the request failed to open its file
                // and never goes into the OPENED state
                void* buf_ptr = sfetch_unbind_buffer(response.handle);
                free(buf_ptr);
            }
        }

    sfetch_desc_t sfetch_desc(void)
    -------------------------------
    sfetch_desc() returns a copy of the sfetch_desc_t struct passed to
    sfetch_setup(), with zero-initialized values replaced with
    their default values.

    int sfetch_max_userdata_bytes(void)
    -----------------------------------
    This returns the value of the SFETCH_MAX_USERDATA_UINT64 config
    define, but in number of bytes (so SFETCH_MAX_USERDATA_UINT64*8).

    int sfetch_max_path(void)
    -------------------------
    Returns the value of the SFETCH_MAX_PATH config define.


    REQUEST STATES AND THE RESPONSE CALLBACK
    ========================================
    A request goes through a number of states during its lifetime. Depending
    on the current state of a request, it will be 'owned' either by the
    "user-thread" (where the request was sent) or an IO thread.

    You can think of a request as "ping-ponging" between the IO thread and
    user thread, any actual IO work is done on the IO thread, while
    invocations of the response-callback happen on the user-thread.

    All state transitions and callback invocations happen inside the
    sfetch_dowork() function.

    An active request goes through the following states:

    ALLOCATED (user-thread)

        The request has been allocated in sfetch_send() and is
        waiting to be dispatched into its IO channel. When this
        happens, the request will transition into the DISPATCHED state.

    DISPATCHED (IO thread)

        The request has been dispatched into its IO channel, and a
        lane has been assigned to the request.

        If a buffer was provided in sfetch_send() the request will
        immediately transition into the FETCHING state and start loading
        data into the buffer.

        If no buffer was provided in sfetch_send(), the response
        callback will be called with (response->dispatched == true),
        so that the response callback can bind a buffer to the
        request. Binding the buffer in the response callback makes
        sense if the buffer isn't dynamically allocated, but instead
        a pre-allocated buffer must be selected from the request's
        channel and lane.

        Note that it isn't possible to get a file size in the response callback
        which would help with allocating a buffer of the right size, this is
        because it isn't possible in HTTP to query the file size before the
        entire file is downloaded (...when the web server serves files compressed).

        If opening the file failed, the request will transition into
        the FAILED state with the error code SFETCH_ERROR_FILE_NOT_FOUND.

    FETCHING (IO thread)

        While a request is in the FETCHING state, data will be loaded into
        the user-provided buffer.

        If no buffer was provided, the request will go into the FAILED
        state with the error code SFETCH_ERROR_NO_BUFFER.

        If a buffer was provided, but it is too small to contain the
        fetched data, the request will go into the FAILED state with
        error code SFETCH_ERROR_BUFFER_TOO_SMALL.

        If less data can be read from the file than expected, the request
        will go into the FAILED state with error code SFETCH_ERROR_UNEXPECTED_EOF.

        If loading data into the provided buffer works as expected, the
        request will go into the FETCHED state.

    FETCHED (user thread)

        The request goes into the FETCHED state either when the entire file
        has been loaded into the provided buffer (when request.chunk_size == 0),
        or a chunk has been loaded (and optionally decompressed) into the
        buffer (when request.chunk_size > 0).

        The response callback will be called so that the user-code can
        process the loaded data using the following sfetch_response_t struct members:

            - data.ptr: pointer to the start of fetched data
            - data.size: the number of bytes in the provided buffer
            - data_offset: the byte offset of the loaded data chunk in the
              overall file (this is only set to a non-zero value in a streaming
              scenario)

        Once all file data has been loaded, the 'finished' flag will be set
        in the response callback's sfetch_response_t argument.

        After the user callback returns, and all file data has been loaded
        (response.finished flag is set) the request has reached its end-of-life
        and will be recycled.

        Otherwise, if there's still data to load (because streaming was
        requested by providing a non-zero request.chunk_size), the request
        will switch back to the FETCHING state to load the next chunk of data.

        Note that it is ok to associate a different buffer or buffer-size
        with the request by calling sfetch_bind_buffer() in the response-callback.

        To check in the response callback for the FETCHED state, and
        independently whether the request is finished:

            void response_callback(const sfetch_response_t* response) {
                if (response->fetched) {
                    // request is in FETCHED state, the loaded data is available
                    // in .data.ptr, and the number of bytes that have been
                    // loaded in .data.size:
                    const void* data = response->data.ptr;
                    size_t num_bytes = response->data.size;
                }
                if (response->finished) {
                    // the finished flag is set either when all data
                    // has been loaded, the request has been cancelled,
                    // or the file operation has failed, this is where
                    // any required per-request cleanup work should happen
                }
            }


    FAILED (user thread)

        A request will transition into the FAILED state in the following situations:

            - if the file doesn't exist or couldn't be opened for other
              reasons (SFETCH_ERROR_FILE_NOT_FOUND)
            - if no buffer is associated with the request in the FETCHING state
              (SFETCH_ERROR_NO_BUFFER)
            - if the provided buffer is too small to hold the entire file
              (if request.chunk_size == 0), or the (potentially decompressed)
              partial data chunk (SFETCH_ERROR_BUFFER_TOO_SMALL)
            - if less bytes could be read from the file then expected
              (SFETCH_ERROR_UNEXPECTED_EOF)
            - if a request has been cancelled via sfetch_cancel()
              (SFETCH_ERROR_CANCELLED)

        The response callback will be called once after a request goes into
        the FAILED state, with the 'response->finished' and
        'response->failed' flags set to true.

        This gives the user-code a chance to cleanup any resources associated
        with the request.

        To check for the failed state in the response callback:

            void response_callback(const sfetch_response_t* response) {
                if (response->failed) {
                    // specifically check for the failed state...
                }
                // or you can do a catch-all check via the finished-flag:
                if (response->finished) {
                    if (response->failed) {
                        // if more detailed error handling is needed:
                        switch (response->error_code) {
                            ...
                        }
                    }
                }
            }

    PAUSED (user thread)

        A request will transition into the PAUSED state after user-code
        calls the function sfetch_pause() on the request's handle. Usually
        this happens from within the response-callback in streaming scenarios
        when the data streaming needs to wait for a data decoder (like
        a video/audio player) to catch up.

        While a request is in PAUSED state, the response-callback will be
        called in each sfetch_dowork(), so that the user-code can either
        continue the request by calling sfetch_continue(), or cancel
        the request by calling sfetch_cancel().

        When calling sfetch_continue() on a paused request, the request will
        transition into the FETCHING state. Otherwise if sfetch_cancel() is
        called, the request will switch into the FAILED state.

        To check for the PAUSED state in the response callback:

            void response_callback(const sfetch_response_t* response) {
                if (response->paused) {
                    // we can check here whether the request should
                    // continue to load data:
                    if (should_continue(response->handle)) {
                        sfetch_continue(response->handle);
                    }
                }
            }


    CHUNK SIZE AND HTTP COMPRESSION
    ===============================
    TL;DR: for streaming scenarios, the provided chunk-size must be smaller
    than the provided buffer-size because the web server may decide to
    serve the data compressed and the chunk-size must be given in 'compressed
    bytes' while the buffer receives 'uncompressed bytes'. It's not possible
    in HTTP to query the uncompressed size for a compressed download until
    that download has finished.

    With vanilla HTTP, it is not possible to query the actual size of a file
    without downloading the entire file first (the Content-Length response
    header only provides the compressed size). Furthermore, for HTTP
    range-requests, the range is given on the compressed data, not the
    uncompressed data. So if the web server decides to serve the data
    compressed, the content-length and range-request parameters don't
    correspond to the uncompressed data that's arriving in the sokol-fetch
    buffers, and there's no way from JS or WASM to either force uncompressed
    downloads (e.g. by setting the Accept-Encoding field), or access the
    compressed data.

    This has some implications for sokol_fetch.h, most notably that buffers
    can't be provided in the exactly right size, because that size can't
    be queried from HTTP before the data is actually downloaded.

    When downloading whole files at once, it is basically expected that you
    know the maximum files size upfront through other means (for instance
    through a separate meta-data-file which contains the file sizes and
    other meta-data for each file that needs to be loaded).

    For streaming downloads the situation is a bit more complicated. These
    use HTTP range-requests, and those ranges are defined on the (potentially)
    compressed data which the JS/WASM side doesn't have access to. However,
    the JS/WASM side only ever sees the uncompressed data, and it's not possible
    to query the uncompressed size of a range request before that range request
    has finished.

    If the provided buffer is too small to contain the uncompressed data,
    the request will fail with error code SFETCH_ERROR_BUFFER_TOO_SMALL.


    CHANNELS AND LANES
    ==================
    Channels and lanes are (somewhat artificial) concepts to manage
    parallelization, prioritization and rate-limiting.

    Channels can be used to parallelize message processing for better 'pipeline
    throughput', and to prioritize requests: user-code could reserve one
    channel for streaming downloads which need to run in parallel to other
    requests, another channel for "regular" downloads and yet another
    high-priority channel which would only be used for small files which need
    to start loading immediately.

    Each channel comes with its own IO thread and message queues for pumping
    messages in and out of the thread. The channel where a request is
    processed is selected manually when sending a message:

        sfetch_send(&(sfetch_request_t){
            .path = "my_file.txt",
            .callback = my_response_callback,
            .channel = 2
        });

    The number of channels is configured at startup in sfetch_setup() and
    cannot be changed afterwards.

    Channels are completely separate from each other, and a request will
    never "hop" from one channel to another.

    Each channel consists of a fixed number of "lanes" for automatic rate
    limiting:

    When a request is sent to a channel via sfetch_send(), a "free lane" will
    be picked and assigned to the request. The request will occupy this lane
    for its entire life time (also while it is paused). If all lanes of a
    channel are currently occupied, new requests will wait until a
    lane becomes unoccupied.

    Since the number of channels and lanes is known upfront, it is guaranteed
    that there will never be more than "num_channels * num_lanes" requests
    in flight at any one time.

    This guarantee eliminates unexpected load- and memory-spikes when
    many requests are sent in very short time, and it allows to pre-allocate
    a fixed number of memory buffers which can be reused for the entire
    "lifetime" of a sokol-fetch context.

    In the most simple scenario - when a maximum file size is known - buffers
    can be statically allocated like this:

        uint8_t buffer[NUM_CHANNELS][NUM_LANES][MAX_FILE_SIZE];

    Then in the user callback pick a buffer by channel and lane,
    and associate it with the request like this:

        void response_callback(const sfetch_response_t* response) {
            if (response->dispatched) {
                void* ptr = buffer[response->channel][response->lane];
                sfetch_bind_buffer(response->handle, ptr, MAX_FILE_SIZE);
            }
            ...
        }


    NOTES ON OPTIMIZING PIPELINE LATENCY AND THROUGHPUT
    ===================================================
    With the default configuration of 1 channel and 1 lane per channel,
    sokol_fetch.h will appear to have a shockingly bad loading performance
    if several files are loaded.

    This has two reasons:

        (1) all parallelization when loading data has been disabled. A new
        request will only be processed, when the last request has finished.

        (2) every invocation of the response-callback adds one frame of latency
        to the request, because callbacks will only be called from within
        sfetch_dowork()

    sokol-fetch takes a few shortcuts to improve step (2) and reduce
    the 'inherent latency' of a request:

        - if a buffer is provided upfront, the response-callback won't be
        called in the DISPATCHED state, but start right with the FETCHED state
        where data has already been loaded into the buffer

        - there is no separate CLOSED state where the callback is invoked
        separately when loading has finished (or the request has failed),
        instead the finished and failed flags will be set as part of
        the last FETCHED invocation

    This means providing a big-enough buffer to fit the entire file is the
    best case, the response callback will only be called once, ideally in
    the next frame (or two calls to sfetch_dowork()).

    If no buffer is provided upfront, one frame of latency is added because
    the response callback needs to be invoked in the DISPATCHED state so that
    the user code can bind a buffer.

    This means the best case for a request without an upfront-provided
    buffer is 2 frames (or 3 calls to sfetch_dowork()).

    That's about what can be done to improve the latency for a single request,
    but the really important step is to improve overall throughput. If you
    need to load thousands of files you don't want that to be completely
    serialized.

    The most important action to increase throughput is to increase the
    number of lanes per channel. This defines how many requests can be
    'in flight' on a single channel at the same time. The guiding decision
    factor for how many lanes you can "afford" is the memory size you want
    to set aside for buffers. Each lane needs its own buffer so that
    the data loaded for one request doesn't scribble over the data
    loaded for another request.

    Here's a simple example of sending 4 requests without upfront buffer
    on a channel with 1, 2 and 4 lanes, each line is one frame:

        1 LANE (8 frames):
            Lane 0:
            -------------
            REQ 0 DISPATCHED
            REQ 0 FETCHED
            REQ 1 DISPATCHED
            REQ 1 FETCHED
            REQ 2 DISPATCHED
            REQ 2 FETCHED
            REQ 3 DISPATCHED
            REQ 3 FETCHED

    Note how the request don't overlap, so they can all use the same buffer.

        2 LANES (4 frames):
            Lane 0:             Lane 1:
            ------------------------------------
            REQ 0 DISPATCHED    REQ 1 DISPATCHED
            REQ 0 FETCHED       REQ 1 FETCHED
            REQ 2 DISPATCHED    REQ 3 DISPATCHED
            REQ 2 FETCHED       REQ 3 FETCHED

    This reduces the overall time to 4 frames, but now you need 2 buffers so
    that requests don't scribble over each other.

        4 LANES (2 frames):
            Lane 0:             Lane 1:             Lane 2:             Lane 3:
            ----------------------------------------------------------------------------
            REQ 0 DISPATCHED    REQ 1 DISPATCHED    REQ 2 DISPATCHED    REQ 3 DISPATCHED
            REQ 0 FETCHED       REQ 1 FETCHED       REQ 2 FETCHED       REQ 3 FETCHED

    Now we're down to the same 'best-case' latency as sending a single
    request.

    Apart from the memory requirements for the streaming buffers (which is
    under your control), you can be generous with the number of lanes,
    they don't add any processing overhead.

    The last option for tweaking latency and throughput is channels. Each
    channel works independently from other channels, so while one
    channel is busy working through a large number of requests (or one
    very long streaming download), you can set aside a high-priority channel
    for requests that need to start as soon as possible.

    On platforms with threading support, each channel runs on its own
    thread, but this is mainly an implementation detail to work around
    the traditional blocking file IO functions, not for performance reasons.


    MEMORY ALLOCATION OVERRIDE
    ==========================
    You can override the memory allocation functions at initialization time
    like this:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        ...
            sfetch_setup(&(sfetch_desc_t){
                // ...
                .allocator = {
                    .alloc_fn = my_alloc,
                    .free_fn = my_free,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, malloc and free will be used.

    This only affects memory allocation calls done by sokol_fetch.h
    itself though, not any allocations in OS libraries.

    Memory allocation will only happen on the same thread where sfetch_setup()
    was called, so you don't need to worry about thread-safety.


    ERROR REPORTING AND LOGGING
    ===========================
    To get any logging information at all you need to provide a logging callback in the setup call,
    the easiest way is to use sokol_log.h:

        #include "sokol_log.h"

        sfetch_setup(&(sfetch_desc_t){
            // ...
            .logger.func = slog_func
        });

    To override logging with your own callback, first write a logging function like this:

        void my_log(const char* tag,                // e.g. 'sfetch'
                    uint32_t log_level,             // 0=panic, 1=error, 2=warn, 3=info
                    uint32_t log_item_id,           // SFETCH_LOGITEM_*
                    const char* message_or_null,    // a message string, may be nullptr in release mode
                    uint32_t line_nr,               // line number in sokol_fetch.h
                    const char* filename_or_null,   // source filename, may be nullptr in release mode
                    void* user_data)
        {
            ...
        }

    ...and then setup sokol-fetch like this:

        sfetch_setup(&(sfetch_desc_t){
            .logger = {
                .func = my_log,
                .user_data = my_user_data,
            }
        });

    The provided logging function must be reentrant (e.g. be callable from
    different threads).

    If you don't want to provide your own custom logger it is highly recommended to use
    the standard logger in sokol_log.h instead, otherwise you won't see any warnings or
    errors.


    FUTURE PLANS / V2.0 IDEA DUMP
    =============================
    - An optional polling API (as alternative to callback API)
    - Move buffer-management into the API? The "manual management"
      can be quite tricky especially for dynamic allocation scenarios,
      API support for buffer management would simplify cases like
      preventing that requests scribble over each other's buffers, or
      an automatic garbage collection for dynamically allocated buffers,
      or automatically falling back to dynamic allocation if static
      buffers aren't big enough.
    - Pluggable request handlers to load data from other "sources"
      (especially HTTP downloads on native platforms via e.g. libcurl
      would be useful)
    - I'm currently not happy how the user-data block is handled, this
      should getting and updating the user-data should be wrapped by
      API functions (similar to bind/unbind buffer)


    LICENSE
    =======
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
#include <stddef.h> // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_FETCH_API_DECL)
#define SOKOL_FETCH_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_FETCH_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_FETCH_IMPL)
#define SOKOL_FETCH_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_FETCH_API_DECL __declspec(dllimport)
#else
#define SOKOL_FETCH_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    sfetch_log_item_t

    Log items are defined via X-Macros, and expanded to an
    enum 'sfetch_log_item', and in debug mode only,
    corresponding strings.

    Used as parameter in the logging callback.
*/
#define _SFETCH_LOG_ITEMS \
    _SFETCH_LOGITEM_XMACRO(OK, "Ok") \
    _SFETCH_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SFETCH_LOGITEM_XMACRO(FILE_PATH_UTF8_DECODING_FAILED, "failed converting file path from UTF8 to wide") \
    _SFETCH_LOGITEM_XMACRO(SEND_QUEUE_FULL, "send queue full (adjust via sfetch_desc_t.max_requests)")  \
    _SFETCH_LOGITEM_XMACRO(REQUEST_CHANNEL_INDEX_TOO_BIG, "channel index too big (adjust via sfetch_desc_t.num_channels)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_PATH_IS_NULL, "file path is nullptr (sfetch_request_t.path)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_PATH_TOO_LONG, "file path is too long (SFETCH_MAX_PATH)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_CALLBACK_MISSING, "no callback provided (sfetch_request_t.callback)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_CHUNK_SIZE_GREATER_BUFFER_SIZE, "chunk size is greater buffer size (sfetch_request_t.chunk_size vs .buffer.size)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_USERDATA_PTR_IS_SET_BUT_USERDATA_SIZE_IS_NULL, "user data ptr is set but user data size is null (sfetch_request_t.user_data.ptr vs .size)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_USERDATA_PTR_IS_NULL_BUT_USERDATA_SIZE_IS_NOT, "user data ptr is null but size is not (sfetch_request_t.user_data.ptr vs .size)") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_USERDATA_SIZE_TOO_BIG, "user data size too big (see SFETCH_MAX_USERDATA_UINT64)") \
    _SFETCH_LOGITEM_XMACRO(CLAMPING_NUM_CHANNELS_TO_MAX_CHANNELS, "clamping num channels to SFETCH_MAX_CHANNELS") \
    _SFETCH_LOGITEM_XMACRO(REQUEST_POOL_EXHAUSTED, "request pool exhausted (tweak via sfetch_desc_t.max_requests)") \

#define _SFETCH_LOGITEM_XMACRO(item,msg) SFETCH_LOGITEM_##item,
typedef enum sfetch_log_item_t {
    _SFETCH_LOG_ITEMS
} sfetch_log_item_t;
#undef _SFETCH_LOGITEM_XMACRO

/*
    sfetch_logger_t

    Used in sfetch_desc_t to provide a custom logging and error reporting
    callback to sokol-fetch.
*/
typedef struct sfetch_logger_t {
    void (*func)(
        const char* tag,                // always "sfetch"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SFETCH_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_fetch.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} sfetch_logger_t;

/*
    sfetch_range_t

    A pointer-size pair struct to pass memory ranges into and out of sokol-fetch.
    When initialized from a value type (array or struct) you can use the
    SFETCH_RANGE() helper macro to build an sfetch_range_t struct.
*/
typedef struct sfetch_range_t {
    const void* ptr;
    size_t size;
} sfetch_range_t;

// disabling this for every includer isn't great, but the warnings are also quite pointless
#if defined(_MSC_VER)
#pragma warning(disable:4221)   // /W4 only: nonstandard extension used: 'x': cannot be initialized using address of automatic variable 'y'
#pragma warning(disable:4204)   // VS2015: nonstandard extension used: non-constant aggregate initializer
#endif
#if defined(__cplusplus)
#define SFETCH_RANGE(x) sfetch_range_t{ &x, sizeof(x) }
#else
#define SFETCH_RANGE(x) (sfetch_range_t){ &x, sizeof(x) }
#endif

/*
    sfetch_allocator_t

    Used in sfetch_desc_t to provide custom memory-alloc and -free functions
    to sokol_fetch.h. If memory management should be overridden, both the
    alloc and free function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct sfetch_allocator_t {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} sfetch_allocator_t;

/* configuration values for sfetch_setup() */
typedef struct sfetch_desc_t {
    uint32_t max_requests;          // max number of active requests across all channels (default: 128)
    uint32_t num_channels;          // number of channels to fetch requests in parallel (default: 1)
    uint32_t num_lanes;             // max number of requests active on the same channel (default: 1)
    sfetch_allocator_t allocator;   // optional memory allocation overrides (default: malloc/free)
    sfetch_logger_t logger;         // optional log function overrides (default: NO LOGGING!)
} sfetch_desc_t;

/* a request handle to identify an active fetch request, returned by sfetch_send() */
typedef struct sfetch_handle_t { uint32_t id; } sfetch_handle_t;

/* error codes */
typedef enum sfetch_error_t {
    SFETCH_ERROR_NO_ERROR,
    SFETCH_ERROR_FILE_NOT_FOUND,
    SFETCH_ERROR_NO_BUFFER,
    SFETCH_ERROR_BUFFER_TOO_SMALL,
    SFETCH_ERROR_UNEXPECTED_EOF,
    SFETCH_ERROR_INVALID_HTTP_STATUS,
    SFETCH_ERROR_CANCELLED,
    SFETCH_ERROR_JS_OTHER,          // check browser console for detailed error info
} sfetch_error_t;

/* the response struct passed to the response callback */
typedef struct sfetch_response_t {
    sfetch_handle_t handle;         // request handle this response belongs to
    bool dispatched;                // true when request is in DISPATCHED state (lane has been assigned)
    bool fetched;                   // true when request is in FETCHED state (fetched data is available)
    bool paused;                    // request is currently in paused state
    bool finished;                  // this is the last response for this request
    bool failed;                    // request has failed (always set together with 'finished')
    bool cancelled;                 // request was cancelled (always set together with 'finished')
    sfetch_error_t error_code;      // more detailed error code when failed is true
    uint32_t channel;               // the channel which processes this request
    uint32_t lane;                  // the lane this request occupies on its channel
    const char* path;               // the original filesystem path of the request
    void* user_data;                // pointer to read/write user-data area
    uint32_t data_offset;           // current offset of fetched data chunk in the overall file data
    sfetch_range_t data;            // the fetched data as ptr/size pair (data.ptr == buffer.ptr, data.size <= buffer.size)
    sfetch_range_t buffer;          // the user-provided buffer which holds the fetched data
} sfetch_response_t;

/* request parameters passed to sfetch_send() */
typedef struct sfetch_request_t {
    uint32_t channel;                                // index of channel this request is assigned to (default: 0)
    const char* path;                                // filesystem path or HTTP URL (required)
    void (*callback) (const sfetch_response_t*);     // response callback function pointer (required)
    uint32_t chunk_size;                             // number of bytes to load per stream-block (optional)
    sfetch_range_t buffer;                           // a memory buffer where the data will be loaded into (optional)
    sfetch_range_t user_data;                        // ptr/size of a POD user data block which will be memcpy'd (optional)
} sfetch_request_t;

/* setup sokol-fetch (can be called on multiple threads) */
SOKOL_FETCH_API_DECL void sfetch_setup(const sfetch_desc_t* desc);
/* discard a sokol-fetch context */
SOKOL_FETCH_API_DECL void sfetch_shutdown(void);
/* return true if sokol-fetch has been setup */
SOKOL_FETCH_API_DECL bool sfetch_valid(void);
/* get the desc struct that was passed to sfetch_setup() */
SOKOL_FETCH_API_DECL sfetch_desc_t sfetch_desc(void);
/* return the max userdata size in number of bytes (SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) */
SOKOL_FETCH_API_DECL int sfetch_max_userdata_bytes(void);
/* return the value of the SFETCH_MAX_PATH implementation config value */
SOKOL_FETCH_API_DECL int sfetch_max_path(void);

/* send a fetch-request, get handle to request back */
SOKOL_FETCH_API_DECL sfetch_handle_t sfetch_send(const sfetch_request_t* request);
/* return true if a handle is valid *and* the request is alive */
SOKOL_FETCH_API_DECL bool sfetch_handle_valid(sfetch_handle_t h);
/* do per-frame work, moves requests into and out of IO threads, and invokes response-callbacks */
SOKOL_FETCH_API_DECL void sfetch_dowork(void);

/* bind a data buffer to a request (request must not currently have a buffer bound, must be called from response callback */
SOKOL_FETCH_API_DECL void sfetch_bind_buffer(sfetch_handle_t h, sfetch_range_t buffer);
/* clear the 'buffer binding' of a request, returns previous buffer pointer (can be 0), must be called from response callback */
SOKOL_FETCH_API_DECL void* sfetch_unbind_buffer(sfetch_handle_t h);
/* cancel a request that's in flight (will call response callback with .cancelled + .finished) */
SOKOL_FETCH_API_DECL void sfetch_cancel(sfetch_handle_t h);
/* pause a request (will call response callback each frame with .paused) */
SOKOL_FETCH_API_DECL void sfetch_pause(sfetch_handle_t h);
/* continue a paused request */
SOKOL_FETCH_API_DECL void sfetch_continue(sfetch_handle_t h);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */
inline void sfetch_setup(const sfetch_desc_t& desc) { return sfetch_setup(&desc); }
inline sfetch_handle_t sfetch_send(const sfetch_request_t& request) { return sfetch_send(&request); }

#endif
#endif // SOKOL_FETCH_INCLUDED

// ██ ███    ███ ██████  ██      ███████ ███    ███ ███████ ███    ██ ████████  █████  ████████ ██  ██████  ███    ██
// ██ ████  ████ ██   ██ ██      ██      ████  ████ ██      ████   ██    ██    ██   ██    ██    ██ ██    ██ ████   ██
// ██ ██ ████ ██ ██████  ██      █████   ██ ████ ██ █████   ██ ██  ██    ██    ███████    ██    ██ ██    ██ ██ ██  ██
// ██ ██  ██  ██ ██      ██      ██      ██  ██  ██ ██      ██  ██ ██    ██    ██   ██    ██    ██ ██    ██ ██  ██ ██
// ██ ██      ██ ██      ███████ ███████ ██      ██ ███████ ██   ████    ██    ██   ██    ██    ██  ██████  ██   ████
//
// >>implementation
#ifdef SOKOL_FETCH_IMPL
#define SOKOL_FETCH_IMPL_INCLUDED (1)

#if defined(SOKOL_MALLOC) || defined(SOKOL_CALLOC) || defined(SOKOL_FREE)
#error "SOKOL_MALLOC/CALLOC/FREE macros are no longer supported, please use sfetch_desc_t.allocator to override memory allocation functions"
#endif

#include <stdlib.h> /* malloc, free */
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
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
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
    #ifndef NOMINMAX
    #define NOMINMAX
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4724) // potential mod by 0
#endif

// ███████ ████████ ██████  ██    ██  ██████ ████████ ███████
// ██         ██    ██   ██ ██    ██ ██         ██    ██
// ███████    ██    ██████  ██    ██ ██         ██    ███████
//      ██    ██    ██   ██ ██    ██ ██         ██         ██
// ███████    ██    ██   ██  ██████   ██████    ██    ███████
//
// >>structs
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
    bool pause;                 /* switch item to PAUSED state if true */
    bool cont;                  /* switch item back to FETCHING if true */
    bool cancel;                /* cancel the request, switch into FAILED state */
    /* transfer IO => user thread */
    uint32_t fetched_offset;    /* number of bytes fetched so far */
    uint32_t fetched_size;      /* size of last fetched chunk */
    sfetch_error_t error_code;
    bool finished;
    /* user thread only */
    size_t user_data_size;
    uint64_t user_data[SFETCH_MAX_USERDATA_UINT64];
} _sfetch_item_user_t;

/* thread-side per-request state */
typedef struct {
    /* transfer IO => user thread */
    uint32_t fetched_offset;
    uint32_t fetched_size;
    sfetch_error_t error_code;
    bool failed;
    bool finished;
    /* IO thread only */
    #if _SFETCH_PLATFORM_EMSCRIPTEN
    uint32_t http_range_offset;
    #else
    _sfetch_file_handle_t file_handle;
    #endif
    uint32_t content_size;
} _sfetch_item_thread_t;

/* a request goes through the following states, ping-ponging between IO and user thread */
typedef enum _sfetch_state_t {
    _SFETCH_STATE_INITIAL,      /* internal: request has just been initialized */
    _SFETCH_STATE_ALLOCATED,    /* internal: request has been allocated from internal pool */
    _SFETCH_STATE_DISPATCHED,   /* user thread: request has been dispatched to its IO channel */
    _SFETCH_STATE_FETCHING,     /* IO thread: waiting for data to be fetched */
    _SFETCH_STATE_FETCHED,      /* user thread: fetched data available */
    _SFETCH_STATE_PAUSED,       /* user thread: request has been paused via sfetch_pause() */
    _SFETCH_STATE_FAILED,       /* user thread: follow state or FETCHING if something went wrong */
} _sfetch_state_t;

/* an internal request item */
#define _SFETCH_INVALID_LANE (0xFFFFFFFF)
typedef struct {
    sfetch_handle_t handle;
    _sfetch_state_t state;
    uint32_t channel;
    uint32_t lane;
    uint32_t chunk_size;
    void (*callback) (const sfetch_response_t*);
    sfetch_range_t buffer;

    /* updated by IO-thread, off-limits to user thread */
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
    struct _sfetch_t* ctx;  // back-pointer to thread-local _sfetch state pointer, since this isn't accessible from the IO threads
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
    bool in_callback;
    sfetch_desc_t desc;
    _sfetch_pool_t pool;
    _sfetch_channel_t chn[SFETCH_MAX_CHANNELS];
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
#define _sfetch_def(val, def) (((val) == 0) ? (def) : (val))

// ██       ██████   ██████   ██████  ██ ███    ██  ██████
// ██      ██    ██ ██       ██       ██ ████   ██ ██
// ██      ██    ██ ██   ███ ██   ███ ██ ██ ██  ██ ██   ███
// ██      ██    ██ ██    ██ ██    ██ ██ ██  ██ ██ ██    ██
// ███████  ██████   ██████   ██████  ██ ██   ████  ██████
//
// >>logging
#if defined(SOKOL_DEBUG)
#define _SFETCH_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _sfetch_log_messages[] = {
    _SFETCH_LOG_ITEMS
};
#undef _SFETCH_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SFETCH_PANIC(code) _sfetch_log(SFETCH_LOGITEM_ ##code, 0, __LINE__)
#define _SFETCH_ERROR(code) _sfetch_log(SFETCH_LOGITEM_ ##code, 1, __LINE__)
#define _SFETCH_WARN(code) _sfetch_log(SFETCH_LOGITEM_ ##code, 2, __LINE__)
#define _SFETCH_INFO(code) _sfetch_log(SFETCH_LOGITEM_ ##code, 3, __LINE__)

static void _sfetch_log(sfetch_log_item_t log_item, uint32_t log_level, uint32_t line_nr) {
    if (_sfetch->desc.logger.func) {
        #if defined(SOKOL_DEBUG)
            const char* filename = __FILE__;
            const char* message = _sfetch_log_messages[log_item];
        #else
            const char* filename = 0;
            const char* message = 0;
        #endif
        _sfetch->desc.logger.func("sfetch", log_level, (uint32_t)log_item, message, line_nr, filename, _sfetch->desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// ███    ███ ███████ ███    ███  ██████  ██████  ██    ██
// ████  ████ ██      ████  ████ ██    ██ ██   ██  ██  ██
// ██ ████ ██ █████   ██ ████ ██ ██    ██ ██████    ████
// ██  ██  ██ ██      ██  ██  ██ ██    ██ ██   ██    ██
// ██      ██ ███████ ██      ██  ██████  ██   ██    ██
//
// >>memory
_SOKOL_PRIVATE void _sfetch_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void* _sfetch_malloc_with_allocator(const sfetch_allocator_t* allocator, size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (allocator->alloc_fn) {
        ptr = allocator->alloc_fn(size, allocator->user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SFETCH_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

_SOKOL_PRIVATE void* _sfetch_malloc(size_t size) {
    return _sfetch_malloc_with_allocator(&_sfetch->desc.allocator, size);
}

_SOKOL_PRIVATE void* _sfetch_malloc_clear(size_t size) {
    void* ptr = _sfetch_malloc(size);
    _sfetch_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _sfetch_free(void* ptr) {
    if (_sfetch->desc.allocator.free_fn) {
        _sfetch->desc.allocator.free_fn(ptr, _sfetch->desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

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
    } else {
        _sfetch_clear(dst->buf, SFETCH_MAX_PATH);
    }
}

_SOKOL_PRIVATE _sfetch_path_t _sfetch_path_make(const char* str) {
    _sfetch_path_t res;
    _sfetch_path_copy(&res, str);
    return res;
}

// ███    ███ ███████ ███████ ███████  █████   ██████  ███████      ██████  ██    ██ ███████ ██    ██ ███████
// ████  ████ ██      ██      ██      ██   ██ ██       ██          ██    ██ ██    ██ ██      ██    ██ ██
// ██ ████ ██ █████   ███████ ███████ ███████ ██   ███ █████       ██    ██ ██    ██ █████   ██    ██ █████
// ██  ██  ██ ██           ██      ██ ██   ██ ██    ██ ██          ██ ▄▄ ██ ██    ██ ██      ██    ██ ██
// ██      ██ ███████ ███████ ███████ ██   ██  ██████  ███████      ██████   ██████  ███████  ██████  ███████
//                                                                     ▀▀
// >>message queue
_SOKOL_PRIVATE uint32_t _sfetch_ring_wrap(const _sfetch_ring_t* rb, uint32_t i) {
    return i % rb->num;
}

_SOKOL_PRIVATE void _sfetch_ring_discard(_sfetch_ring_t* rb) {
    SOKOL_ASSERT(rb);
    if (rb->buf) {
        _sfetch_free(rb->buf);
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
    rb->buf = (uint32_t*) _sfetch_malloc_clear(queue_size);
    if (rb->buf) {
        return true;
    } else {
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
    } else {
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

// ██████  ███████  ██████  ██    ██ ███████ ███████ ████████     ██████   ██████   ██████  ██
// ██   ██ ██      ██    ██ ██    ██ ██      ██         ██        ██   ██ ██    ██ ██    ██ ██
// ██████  █████   ██    ██ ██    ██ █████   ███████    ██        ██████  ██    ██ ██    ██ ██
// ██   ██ ██      ██ ▄▄ ██ ██    ██ ██           ██    ██        ██      ██    ██ ██    ██ ██
// ██   ██ ███████  ██████   ██████  ███████ ███████    ██        ██       ██████   ██████  ███████
//                     ▀▀
// >>request pool
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

_SOKOL_PRIVATE void _sfetch_item_init(_sfetch_item_t* item, uint32_t slot_id, const sfetch_request_t* request) {
    SOKOL_ASSERT(item && (0 == item->handle.id));
    SOKOL_ASSERT(request && request->path);
    _sfetch_clear(item, sizeof(_sfetch_item_t));
    item->handle.id = slot_id;
    item->state = _SFETCH_STATE_INITIAL;
    item->channel = request->channel;
    item->chunk_size = request->chunk_size;
    item->lane = _SFETCH_INVALID_LANE;
    item->callback = request->callback;
    item->buffer = request->buffer;
    item->path = _sfetch_path_make(request->path);
    #if !_SFETCH_PLATFORM_EMSCRIPTEN
    item->thread.file_handle = _SFETCH_INVALID_FILE_HANDLE;
    #endif
    if (request->user_data.ptr &&
        (request->user_data.size > 0) &&
        (request->user_data.size <= (SFETCH_MAX_USERDATA_UINT64*8)))
    {
        item->user.user_data_size = request->user_data.size;
        memcpy(item->user.user_data, request->user_data.ptr, request->user_data.size);
    }
}

_SOKOL_PRIVATE void _sfetch_item_discard(_sfetch_item_t* item) {
    SOKOL_ASSERT(item && (0 != item->handle.id));
    _sfetch_clear(item, sizeof(_sfetch_item_t));
}

_SOKOL_PRIVATE void _sfetch_pool_discard(_sfetch_pool_t* pool) {
    SOKOL_ASSERT(pool);
    if (pool->free_slots) {
        _sfetch_free(pool->free_slots);
        pool->free_slots = 0;
    }
    if (pool->gen_ctrs) {
        _sfetch_free(pool->gen_ctrs);
        pool->gen_ctrs = 0;
    }
    if (pool->items) {
        _sfetch_free(pool->items);
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
    pool->items = (_sfetch_item_t*) _sfetch_malloc_clear(items_size);
    /* generation counters indexable by pool slot index, slot 0 is reserved */
    const size_t gen_ctrs_size = sizeof(uint32_t) * pool->size;
    pool->gen_ctrs = (uint32_t*) _sfetch_malloc_clear(gen_ctrs_size);
    SOKOL_ASSERT(pool->gen_ctrs);
    /* NOTE: it's not a bug to only reserve num_items here */
    const size_t free_slots_size = num_items * sizeof(int);
    pool->free_slots = (uint32_t*) _sfetch_malloc_clear(free_slots_size);
    if (pool->items && pool->free_slots) {
        /* never allocate the 0-th item, this is the reserved 'invalid item' */
        for (uint32_t i = pool->size - 1; i >= 1; i--) {
            pool->free_slots[pool->free_top++] = i;
        }
        pool->valid = true;
    } else {
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
        pool->items[slot_index].state = _SFETCH_STATE_ALLOCATED;
        return slot_id;
    } else {
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

// ██████   ██████  ███████ ██ ██   ██
// ██   ██ ██    ██ ██      ██  ██ ██
// ██████  ██    ██ ███████ ██   ███
// ██      ██    ██      ██ ██  ██ ██
// ██       ██████  ███████ ██ ██   ██
//
// >>posix
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

_SOKOL_PRIVATE uint32_t _sfetch_file_size(_sfetch_file_handle_t h) {
    fseek(h, 0, SEEK_END);
    return (uint32_t) ftell(h);
}

_SOKOL_PRIVATE bool _sfetch_file_read(_sfetch_file_handle_t h, uint32_t offset, uint32_t num_bytes, void* ptr) {
    fseek(h, (long)offset, SEEK_SET);
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

// ██     ██ ██ ███    ██ ██████   ██████  ██     ██ ███████
// ██     ██ ██ ████   ██ ██   ██ ██    ██ ██     ██ ██
// ██  █  ██ ██ ██ ██  ██ ██   ██ ██    ██ ██  █  ██ ███████
// ██ ███ ██ ██ ██  ██ ██ ██   ██ ██    ██ ██ ███ ██      ██
//  ███ ███  ██ ██   ████ ██████   ██████   ███ ███  ███████
//
// >>windows
#if _SFETCH_PLATFORM_WINDOWS
_SOKOL_PRIVATE bool _sfetch_win32_utf8_to_wide(const char* src, wchar_t* dst, int dst_num_bytes) {
    SOKOL_ASSERT(src && dst && (dst_num_bytes > 1));
    _sfetch_clear(dst, (size_t)dst_num_bytes);
    const int dst_chars = dst_num_bytes / (int)sizeof(wchar_t);
    const int dst_needed = MultiByteToWideChar(CP_UTF8, 0, src, -1, 0, 0);
    if ((dst_needed > 0) && (dst_needed < dst_chars)) {
        MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_chars);
        return true;
    } else {
        /* input string doesn't fit into destination buffer */
        return false;
    }
}

_SOKOL_PRIVATE _sfetch_file_handle_t _sfetch_file_open(const _sfetch_path_t* path) {
    wchar_t w_path[SFETCH_MAX_PATH];
    if (!_sfetch_win32_utf8_to_wide(path->buf, w_path, sizeof(w_path))) {
        _SFETCH_ERROR(FILE_PATH_UTF8_DECODING_FAILED);
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

_SOKOL_PRIVATE uint32_t _sfetch_file_size(_sfetch_file_handle_t h) {
    return GetFileSize(h, NULL);
}

_SOKOL_PRIVATE bool _sfetch_file_read(_sfetch_file_handle_t h, uint32_t offset, uint32_t num_bytes, void* ptr) {
    LARGE_INTEGER offset_li;
    offset_li.QuadPart = offset;
    BOOL seek_res = SetFilePointerEx(h, offset_li, NULL, FILE_BEGIN);
    if (seek_res) {
        DWORD bytes_read = 0;
        BOOL read_res = ReadFile(h, ptr, (DWORD)num_bytes, &bytes_read, NULL);
        return read_res && (bytes_read == num_bytes);
    } else {
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
    thread->thread = CreateThread(NULL, stack_size, thread_func, thread_arg, 0, NULL);
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
        _SOKOL_UNUSED(set_event_res);
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
        _SOKOL_UNUSED(set_event_res);
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
        WaitForSingleObject(thread->incoming_event, INFINITE);
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

//  ██████ ██   ██  █████  ███    ██ ███    ██ ███████ ██      ███████
// ██      ██   ██ ██   ██ ████   ██ ████   ██ ██      ██      ██
// ██      ███████ ███████ ██ ██  ██ ██ ██  ██ █████   ██      ███████
// ██      ██   ██ ██   ██ ██  ██ ██ ██  ██ ██ ██      ██           ██
//  ██████ ██   ██ ██   ██ ██   ████ ██   ████ ███████ ███████ ███████
//
// >>channels

/* per-channel request handler for native platforms accessing the local filesystem */
#if _SFETCH_HAS_THREADS
_SOKOL_PRIVATE void _sfetch_request_handler(_sfetch_t* ctx, uint32_t slot_id) {
    _sfetch_state_t state;
    _sfetch_path_t* path;
    _sfetch_item_thread_t* thread;
    sfetch_range_t* buffer;
    uint32_t chunk_size;
    {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (!item) {
            return;
        }
        state = item->state;
        SOKOL_ASSERT((state == _SFETCH_STATE_FETCHING) ||
                     (state == _SFETCH_STATE_PAUSED) ||
                     (state == _SFETCH_STATE_FAILED));
        path = &item->path;
        thread = &item->thread;
        buffer = &item->buffer;
        chunk_size = item->chunk_size;
    }
    if (thread->failed) {
        return;
    }
    if (state == _SFETCH_STATE_FETCHING) {
        if ((buffer->ptr == 0) || (buffer->size == 0)) {
            thread->error_code = SFETCH_ERROR_NO_BUFFER;
            thread->failed = true;
        } else {
            /* open file if not happened yet */
            if (!_sfetch_file_handle_valid(thread->file_handle)) {
                SOKOL_ASSERT(path->buf[0]);
                SOKOL_ASSERT(thread->fetched_offset == 0);
                SOKOL_ASSERT(thread->fetched_size == 0);
                thread->file_handle = _sfetch_file_open(path);
                if (_sfetch_file_handle_valid(thread->file_handle)) {
                    thread->content_size = _sfetch_file_size(thread->file_handle);
                } else {
                    thread->error_code = SFETCH_ERROR_FILE_NOT_FOUND;
                    thread->failed = true;
                }
            }
            if (!thread->failed) {
                uint32_t read_offset = 0;
                uint32_t bytes_to_read = 0;
                if (chunk_size == 0) {
                    /* load entire file */
                    if (thread->content_size <= buffer->size) {
                        bytes_to_read = thread->content_size;
                        read_offset = 0;
                    } else {
                        /* provided buffer to small to fit entire file */
                        thread->error_code = SFETCH_ERROR_BUFFER_TOO_SMALL;
                        thread->failed = true;
                    }
                } else {
                    if (chunk_size <= buffer->size) {
                        bytes_to_read = chunk_size;
                        read_offset = thread->fetched_offset;
                        if ((read_offset + bytes_to_read) > thread->content_size) {
                            bytes_to_read = thread->content_size - read_offset;
                        }
                    } else {
                        /* provided buffer to small to fit next chunk */
                        thread->error_code = SFETCH_ERROR_BUFFER_TOO_SMALL;
                        thread->failed = true;
                    }
                }
                if (!thread->failed) {
                    if (_sfetch_file_read(thread->file_handle, read_offset, bytes_to_read, (void*)buffer->ptr)) {
                        thread->fetched_size = bytes_to_read;
                        thread->fetched_offset += bytes_to_read;
                    } else {
                        thread->error_code = SFETCH_ERROR_UNEXPECTED_EOF;
                        thread->failed = true;
                    }
                }
            }
        }
        SOKOL_ASSERT(thread->fetched_offset <= thread->content_size);
        if (thread->failed || (thread->fetched_offset == thread->content_size)) {
            if (_sfetch_file_handle_valid(thread->file_handle)) {
                _sfetch_file_close(thread->file_handle);
                thread->file_handle = _SFETCH_INVALID_FILE_HANDLE;
            }
            thread->finished = true;
        }
    }
    /* ignore items in PAUSED or FAILED state */
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
EM_JS(void, sfetch_js_send_head_request, (uint32_t slot_id, const char* path_cstr), {
    const path_str = UTF8ToString(path_cstr);
    fetch(path_str, { method: 'HEAD' }).then((response) => {
        if (response.ok) {
            const content_length = response.headers.get('Content-Length');
            if (content_length === null) {
                console.warn(`sokol_fetch.h: HEAD ${path_str} response has no Content-Length`);
                __sfetch_emsc_failed_other(slot_id);
            } else {
                __sfetch_emsc_head_response(slot_id, Number(content_length));
            }
        } else {
            __sfetch_emsc_failed_http_status(slot_id, response.status);
        }
    }).catch((err) => {
        console.error(`sokol_fetch.h: HEAD ${path_str} failed with: `, err);
        __sfetch_emsc_failed_other(slot_id);
    });
})

/* if bytes_to_read != 0, a range-request will be sent, otherwise a normal request */
EM_JS(void, sfetch_js_send_get_request, (uint32_t slot_id, const char* path_cstr, uint32_t offset, uint32_t bytes_to_read, void* buf_ptr, uint32_t buf_size), {
    const path_str = UTF8ToString(path_cstr);
    const headers = new Headers();
    const range_request = bytes_to_read > 0;
    if (range_request) {
        headers.append('Range', `bytes=${offset}-${offset+bytes_to_read-1}`);
    }
    fetch(path_str, { method: 'GET', headers }).then((response) => {
        if (response.ok) {
            response.arrayBuffer().then((data) => {
                const u8_data = new Uint8Array(data);
                if (u8_data.length <= buf_size) {
                    HEAPU8.set(u8_data, buf_ptr);
                    __sfetch_emsc_get_response(slot_id, bytes_to_read, u8_data.length);
                } else {
                    __sfetch_emsc_failed_buffer_too_small(slot_id);
                }
            }).catch((err) => {
                console.error(`sokol_fetch.h: GET ${path_str} failed with: `, err);
                __sfetch_emsc_failed_other(slot_id);
            });
        } else {
            __sfetch_emsc_failed_http_status(slot_id, response.status);
        }
    }).catch((err) => {
        console.error(`sokol_fetch.h: GET ${path_str} failed with: `, err);
        __sfetch_emsc_failed_other(slot_id);
    });
})

/*=== emscripten specific C helper functions =================================*/
#ifdef __cplusplus
extern "C" {
#endif
void _sfetch_emsc_send_get_request(uint32_t slot_id, _sfetch_item_t* item) {
    if ((item->buffer.ptr == 0) || (item->buffer.size == 0)) {
        item->thread.error_code = SFETCH_ERROR_NO_BUFFER;
        item->thread.failed = true;
    } else {
        uint32_t offset = 0;
        uint32_t bytes_to_read = 0;
        if (item->chunk_size > 0) {
            /* send HTTP range request */
            SOKOL_ASSERT(item->thread.content_size > 0);
            SOKOL_ASSERT(item->thread.http_range_offset < item->thread.content_size);
            bytes_to_read = item->thread.content_size - item->thread.http_range_offset;
            if (bytes_to_read > item->chunk_size) {
                bytes_to_read = item->chunk_size;
            }
            SOKOL_ASSERT(bytes_to_read > 0);
            offset = item->thread.http_range_offset;
        }
        sfetch_js_send_get_request(slot_id, item->path.buf, offset, bytes_to_read, (void*)item->buffer.ptr, item->buffer.size);
    }
}

/* called by JS when an initial HEAD request finished successfully (only when streaming chunks) */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_head_response(uint32_t slot_id, uint32_t content_length) {
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            SOKOL_ASSERT(item->buffer.ptr && (item->buffer.size > 0));
            item->thread.content_size = content_length;
            _sfetch_emsc_send_get_request(slot_id, item);
        }
    }
}

/* called by JS when a followup GET request finished successfully */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_get_response(uint32_t slot_id, uint32_t range_fetched_size, uint32_t content_fetched_size) {
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.fetched_size = content_fetched_size;
            item->thread.fetched_offset += content_fetched_size;
            item->thread.http_range_offset += range_fetched_size;
            if (item->chunk_size == 0) {
                item->thread.finished = true;
            } else if (item->thread.http_range_offset >= item->thread.content_size) {
                item->thread.finished = true;
            }
            _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
        }
    }
}

/* called by JS when an error occurred */
EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_failed_http_status(uint32_t slot_id, uint32_t http_status) {
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            if (http_status == 404) {
                item->thread.error_code = SFETCH_ERROR_FILE_NOT_FOUND;
            } else {
                item->thread.error_code = SFETCH_ERROR_INVALID_HTTP_STATUS;
            }
            item->thread.failed = true;
            item->thread.finished = true;
            _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
        }
    }
}

EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_failed_buffer_too_small(uint32_t slot_id) {
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.error_code = SFETCH_ERROR_BUFFER_TOO_SMALL;
            item->thread.failed = true;
            item->thread.finished = true;
            _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
        }
    }
}

EMSCRIPTEN_KEEPALIVE void _sfetch_emsc_failed_other(uint32_t slot_id) {
    _sfetch_t* ctx = _sfetch_ctx();
    if (ctx && ctx->valid) {
        _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, slot_id);
        if (item) {
            item->thread.error_code = SFETCH_ERROR_JS_OTHER;
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
    if (item->state == _SFETCH_STATE_FETCHING) {
        if ((item->chunk_size > 0) && (item->thread.content_size == 0)) {
            /* if streaming download is requested, and the content-length isn't known
               yet, need to send a HEAD request first
             */
            sfetch_js_send_head_request(slot_id, item->path.buf);
        } else {
            /* otherwise, this is either a request to load the entire file, or
               to load the next streaming chunk
             */
            _sfetch_emsc_send_get_request(slot_id, item);
        }
    } else {
        /* just move all other items (e.g. paused or cancelled)
           into the outgoing queue, so they won't get lost
        */
        _sfetch_ring_enqueue(&ctx->chn[item->channel].user_outgoing, slot_id);
    }
    if (item->thread.failed) {
        item->thread.finished = true;
    }
}
#endif /* _SFETCH_PLATFORM_EMSCRIPTEN */

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
    } else {
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
    } else {
        _SFETCH_ERROR(SEND_QUEUE_FULL);
        return false;
    }
}

_SOKOL_PRIVATE void _sfetch_invoke_response_callback(_sfetch_item_t* item) {
    sfetch_response_t response;
    _sfetch_clear(&response, sizeof(response));
    response.handle = item->handle;
    response.dispatched = (item->state == _SFETCH_STATE_DISPATCHED);
    response.fetched = (item->state == _SFETCH_STATE_FETCHED);
    response.paused = (item->state == _SFETCH_STATE_PAUSED);
    response.finished = item->user.finished;
    response.failed = (item->state == _SFETCH_STATE_FAILED);
    response.cancelled = item->user.cancel;
    response.error_code = item->user.error_code;
    response.channel = item->channel;
    response.lane = item->lane;
    response.path = item->path.buf;
    response.user_data = item->user.user_data;
    response.data_offset = item->user.fetched_offset - item->user.fetched_size;
    response.data.ptr = item->buffer.ptr;
    response.data.size = item->user.fetched_size;
    response.buffer = item->buffer;
    item->callback(&response);
}

_SOKOL_PRIVATE void _sfetch_cancel_item(_sfetch_item_t* item) {
    item->state = _SFETCH_STATE_FAILED;
    item->user.finished = true;
    item->user.error_code = SFETCH_ERROR_CANCELLED;
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
        SOKOL_ASSERT(item->state == _SFETCH_STATE_ALLOCATED);
        // if the item was cancelled early, kick it out immediately
        if (item->user.cancel) {
            _sfetch_cancel_item(item);
            _sfetch_invoke_response_callback(item);
            _sfetch_pool_item_free(pool, slot_id);
            continue;
        }
        item->state = _SFETCH_STATE_DISPATCHED;
        item->lane = _sfetch_ring_dequeue(&chn->free_lanes);
        // if no buffer provided yet, invoke response callback to do so
        if (0 == item->buffer.ptr) {
            _sfetch_invoke_response_callback(item);
        }
        _sfetch_ring_enqueue(&chn->user_incoming, slot_id);
    }

    /* prepare incoming items for being moved into the IO thread */
    const uint32_t num_incoming = _sfetch_ring_count(&chn->user_incoming);
    for (uint32_t i = 0; i < num_incoming; i++) {
        const uint32_t slot_id = _sfetch_ring_peek(&chn->user_incoming, i);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(pool, slot_id);
        SOKOL_ASSERT(item);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_INITIAL);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_FETCHING);
        /* transfer input params from user- to thread-data */
        if (item->user.pause) {
            item->state = _SFETCH_STATE_PAUSED;
            item->user.pause = false;
        }
        if (item->user.cont) {
            if (item->state == _SFETCH_STATE_PAUSED) {
                item->state = _SFETCH_STATE_FETCHED;
            }
            item->user.cont = false;
        }
        if (item->user.cancel) {
            _sfetch_cancel_item(item);
        }
        switch (item->state) {
            case _SFETCH_STATE_DISPATCHED:
            case _SFETCH_STATE_FETCHED:
                item->state = _SFETCH_STATE_FETCHING;
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
    while (!_sfetch_ring_empty(&chn->user_outgoing)) {
        const uint32_t slot_id = _sfetch_ring_dequeue(&chn->user_outgoing);
        SOKOL_ASSERT(slot_id);
        _sfetch_item_t* item = _sfetch_pool_item_lookup(pool, slot_id);
        SOKOL_ASSERT(item && item->callback);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_INITIAL);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_ALLOCATED);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_DISPATCHED);
        SOKOL_ASSERT(item->state != _SFETCH_STATE_FETCHED);
        /* transfer output params from thread- to user-data */
        item->user.fetched_offset = item->thread.fetched_offset;
        item->user.fetched_size = item->thread.fetched_size;
        if (item->user.cancel) {
            _sfetch_cancel_item(item);
        } else {
            item->user.error_code = item->thread.error_code;
        }
        if (item->thread.finished) {
            item->user.finished = true;
        }
        /* state transition */
        if (item->thread.failed) {
            item->state = _SFETCH_STATE_FAILED;
        } else if (item->state == _SFETCH_STATE_FETCHING) {
            item->state = _SFETCH_STATE_FETCHED;
        }
        _sfetch_invoke_response_callback(item);

        /* when the request is finished, free the lane for another request,
           otherwise feed it back into the incoming queue
        */
        if (item->user.finished) {
            _sfetch_ring_enqueue(&chn->free_lanes, item->lane);
            _sfetch_pool_item_free(pool, slot_id);
        } else {
            _sfetch_ring_enqueue(&chn->user_incoming, slot_id);
        }
    }
}

_SOKOL_PRIVATE bool _sfetch_validate_request(_sfetch_t* ctx, const sfetch_request_t* req) {
    if (req->channel >= ctx->desc.num_channels) {
        _SFETCH_ERROR(REQUEST_CHANNEL_INDEX_TOO_BIG);
        return false;
    }
    if (!req->path) {
        _SFETCH_ERROR(REQUEST_PATH_IS_NULL);
        return false;
    }
    if (strlen(req->path) >= (SFETCH_MAX_PATH-1)) {
        _SFETCH_ERROR(REQUEST_PATH_TOO_LONG);
        return false;
    }
    if (!req->callback) {
        _SFETCH_ERROR(REQUEST_CALLBACK_MISSING);
        return false;
    }
    if (req->chunk_size > req->buffer.size) {
        _SFETCH_ERROR(REQUEST_CHUNK_SIZE_GREATER_BUFFER_SIZE);
        return false;
    }
    if (req->user_data.ptr && (req->user_data.size == 0)) {
        _SFETCH_ERROR(REQUEST_USERDATA_PTR_IS_SET_BUT_USERDATA_SIZE_IS_NULL);
        return false;
    }
    if (!req->user_data.ptr && (req->user_data.size > 0)) {
        _SFETCH_ERROR(REQUEST_USERDATA_PTR_IS_NULL_BUT_USERDATA_SIZE_IS_NOT);
        return false;
    }
    if (req->user_data.size > SFETCH_MAX_USERDATA_UINT64 * sizeof(uint64_t)) {
        _SFETCH_ERROR(REQUEST_USERDATA_SIZE_TOO_BIG);
        return false;
    }
    return true;
}

_SOKOL_PRIVATE sfetch_desc_t _sfetch_desc_defaults(const sfetch_desc_t* desc) {
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    sfetch_desc_t res = *desc;
    res.max_requests = _sfetch_def(desc->max_requests, 128);
    res.num_channels = _sfetch_def(desc->num_channels, 1);
    res.num_lanes = _sfetch_def(desc->num_lanes, 1);
    return res;
}

// ██████  ██    ██ ██████  ██      ██  ██████
// ██   ██ ██    ██ ██   ██ ██      ██ ██
// ██████  ██    ██ ██████  ██      ██ ██
// ██      ██    ██ ██   ██ ██      ██ ██
// ██       ██████  ██████  ███████ ██  ██████
//
// >>public
SOKOL_API_IMPL void sfetch_setup(const sfetch_desc_t* desc_) {
    SOKOL_ASSERT(desc_);
    SOKOL_ASSERT(0 == _sfetch);

    sfetch_desc_t desc = _sfetch_desc_defaults(desc_);
    _sfetch = (_sfetch_t*) _sfetch_malloc_with_allocator(&desc.allocator, sizeof(_sfetch_t));
    SOKOL_ASSERT(_sfetch);
    _sfetch_t* ctx = _sfetch_ctx();
    _sfetch_clear(ctx, sizeof(_sfetch_t));
    ctx->desc = desc;
    ctx->setup = true;
    ctx->valid = true;

    /* replace zero-init items with default values */
    if (ctx->desc.num_channels > SFETCH_MAX_CHANNELS) {
        ctx->desc.num_channels = SFETCH_MAX_CHANNELS;
        _SFETCH_WARN(CLAMPING_NUM_CHANNELS_TO_MAX_CHANNELS);
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
    _sfetch_free(ctx);
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
        _SFETCH_WARN(REQUEST_POOL_EXHAUSTED);
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
    ctx->in_callback = true;
    for (int pass = 0; pass < 2; pass++) {
        for (uint32_t chn_index = 0; chn_index < ctx->desc.num_channels; chn_index++) {
            _sfetch_channel_dowork(&ctx->chn[chn_index], &ctx->pool);
        }
    }
    ctx->in_callback = false;
}

SOKOL_API_IMPL void sfetch_bind_buffer(sfetch_handle_t h, sfetch_range_t buffer) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    SOKOL_ASSERT(ctx->in_callback);
    SOKOL_ASSERT(buffer.ptr && (buffer.size > 0));
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        SOKOL_ASSERT((0 == item->buffer.ptr) && (0 == item->buffer.size));
        item->buffer = buffer;
    }
}

SOKOL_API_IMPL void* sfetch_unbind_buffer(sfetch_handle_t h) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    SOKOL_ASSERT(ctx->in_callback);
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        void* prev_buf_ptr = (void*)item->buffer.ptr;
        item->buffer.ptr = 0;
        item->buffer.size = 0;
        return prev_buf_ptr;
    } else {
        return 0;
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

SOKOL_API_IMPL void sfetch_cancel(sfetch_handle_t h) {
    _sfetch_t* ctx = _sfetch_ctx();
    SOKOL_ASSERT(ctx && ctx->valid);
    _sfetch_item_t* item = _sfetch_pool_item_lookup(&ctx->pool, h.id);
    if (item) {
        item->user.cont = false;
        item->user.pause = false;
        item->user.cancel = true;
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif /* SOKOL_FETCH_IMPL */
