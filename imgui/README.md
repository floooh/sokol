# Sokol Debug Inspection UIs

This directory contains optional 'extension headers' which implement debug
inspection UIs for various Sokol headers implemented with [Dear
ImGui](https://github.com/ocornut/imgui).

## Integration Howto

These are the steps to add the debug inspection UIs to your own project.

### 1. Compile the implementation as C++ (or Objective-C++)

Dear ImGui is a C++ library, and offers a C++ API. This means the implementation
of the debug inspection headers is also written in C++, not the usual C, and
must be compiled in C++ or Objective-C++ mode.

### 2. Compile the Sokol headers with trace-hooks enabled

The debug inspection headers may need to *hook into* the Sokol APIs via
callback functions. These API callbacks are called **trace hooks** and must
be enabled by defining ```SOKOL_TRACE_HOOKS``` before including the
implementation.

### 3. Include "imgui.h" before the implementation

The debug inspection headers don't include ```imgui.h``` themselves,
instead the ImGui header must be included before the implementation.

### 3. Include the debug UI implementations after the Sokol implementations

The debug inspection headers need access to private data and functions of
their associated Sokol header. This means the implementation of the
debug inspection headers must be included **after** the implementation
of the their associated Sokol headers. I'd recomment putting all the
Sokol headers of a project into a single implementation file, together
with all *extension headers*. 

```cpp
// sokol-ui.cc
//
// Sokol header implementations and their debug inspection headers,
// compiled as C++ code and with trace-hooks enabled. On macOS/iOS 
// this would need to be an Objective-C++ file instead (.mm extension).
//
#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "imgui.h"
#include "sokol_app_imgui.h"
#include "sokol_gfx_imgui.h"
```

### 4. Provide your own ImGui renderer

(TODO)

### 5. Add UI init- and rendering-calls to your code

(TODO)



