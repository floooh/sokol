/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/utest.h
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/

#ifndef SHEREDOM_UTEST_H_INCLUDED
#define SHEREDOM_UTEST_H_INCLUDED

#ifdef _MSC_VER
/*
   Disable warning about not inlining 'inline' functions.
   TODO: We'll fix this later by not using fprintf within our macros, and
   instead use snprintf to a realloc'ed buffer.
*/
#pragma warning(disable : 4710)

/*
   Disable warning about inlining functions that are not marked 'inline'.
   TODO: add a UTEST_NOINLINE onto the macro generated functions to fix this.
*/
#pragma warning(disable : 4711)
#pragma warning(push, 1)
#endif

#if defined(_MSC_VER)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
#if defined(_M_IX86)
#define _X86_
#endif

#if defined(_M_AMD64)
#define _AMD64_
#endif

#pragma warning(push, 1)
#include <windef.h>
#include <winbase.h>
#pragma warning(pop)

#elif defined(__linux__)

/*
   slightly obscure include here - we need to include glibc's features.h, but
   we don't want to just include a header that might not be defined for other
   c libraries like musl. Instead we include limits.h, which we know on all
   glibc distributions includes features.h
*/
#include <limits.h>

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#include <time.h>

#if ((2 < __GLIBC__) || ((2 == __GLIBC__) && (17 <= __GLIBC_MINOR__)))
/* glibc is version 2.17 or above, so we can just use clock_gettime */
#define UTEST_USE_CLOCKGETTIME
#else
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

#elif defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#if defined(_MSC_VER)
#define UTEST_PRId64 "I64d"
#define UTEST_PRIu64 "I64u"
#define UTEST_INLINE __forceinline

#pragma section(".CRT$XCU", read)
#define UTEST_INITIALIZER(f)                                                   \
  static void __cdecl f(void);                                                 \
  __declspec(allocate(".CRT$XCU")) void(__cdecl * f##_)(void) = f;             \
  static void __cdecl f(void)
#else
#if defined(__linux__)
#if defined(__clang__)
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif

#define __STDC_FORMAT_MACROS 1

#if defined(__clang__)
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic pop
#endif
#endif
#endif

#include <inttypes.h>

#define UTEST_PRId64 PRId64
#define UTEST_PRIu64 PRIu64
#define UTEST_INLINE inline

#define UTEST_INITIALIZER(f)                                                   \
  static void f(void) __attribute__((constructor));                            \
  static void f(void)
#endif

#if defined(__cplusplus)
#define UTEST_CAST(type, x) static_cast<type>(x)
#define UTEST_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define UTEST_EXTERN extern "C"
#else
#define UTEST_CAST(type, x) ((type)x)
#define UTEST_PTR_CAST(type, x) ((type)x)
#define UTEST_EXTERN extern
#endif

#ifdef _MSC_VER
/*
    io.h contains definitions for some structures with natural padding. This is
    uninteresting, but for some reason MSVC's behaviour is to warn about
    including this system header. That *is* interesting
*/
#pragma warning(disable : 4820)
#pragma warning(push, 1)
#include <io.h>
#pragma warning(pop)
#define UTEST_COLOUR_OUTPUT() (_isatty(_fileno(stdout)))
#else
#include <unistd.h>
#define UTEST_COLOUR_OUTPUT() (isatty(STDOUT_FILENO))
#endif

static UTEST_INLINE int64_t utest_ns(void) {
#ifdef _MSC_VER
  LARGE_INTEGER counter;
  LARGE_INTEGER frequency;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&frequency);
  return UTEST_CAST(int64_t,
                    (counter.QuadPart * 1000000000) / frequency.QuadPart);
#elif defined(__linux)
  struct timespec ts;
  const clockid_t cid = CLOCK_REALTIME;
#if defined(UTEST_USE_CLOCKGETTIME)
  clock_gettime(cid, &ts);
#else
  syscall(SYS_clock_gettime, cid, &ts);
#endif
  return UTEST_CAST(int64_t, ts.tv_sec) * 1000 * 1000 * 1000 + ts.tv_nsec;
#elif __APPLE__
  return UTEST_CAST(int64_t, mach_absolute_time());
#else
  /* hack to prevent warning on unsupported platforms */
  return 1;
#endif
}

typedef void (*utest_testcase_t)(int *, size_t);

struct utest_test_state_s {
  utest_testcase_t func;
  size_t index;
  char *name;
};

struct utest_state_s {
  struct utest_test_state_s *tests;
  size_t tests_length;
  FILE *output;
};

/* extern to the global state utest needs to execute */
UTEST_EXTERN struct utest_state_s utest_state;

#if defined(_MSC_VER)
#define UTEST_WEAK __forceinline
#else
#define UTEST_WEAK __attribute__((weak))
#endif

#if defined(_MSC_VER)
#define UTEST_UNUSED
#else
#define UTEST_UNUSED __attribute__((unused))
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvariadic-macros"
#endif
#define UTEST_PRINTF(...)                                                      \
  if (utest_state.output) {                                                    \
    fprintf(utest_state.output, __VA_ARGS__);                                  \
  }                                                                            \
  printf(__VA_ARGS__)
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#define UTEST_SNPRINTF(BUFFER, N, ...) _snprintf_s(BUFFER, N, N, __VA_ARGS__)
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvariadic-macros"
#endif
#define UTEST_SNPRINTF(...) snprintf(__VA_ARGS__)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#if defined(__cplusplus)
/* if we are using c++ we can use overloaded methods (its in the language) */
#define UTEST_OVERLOADABLE
#elif defined(__clang__)
/* otherwise, if we are using clang with c - use the overloadable attribute */
#define UTEST_OVERLOADABLE __attribute__((overloadable))
#endif

#if defined(UTEST_OVERLOADABLE)
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(float f);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(float f) {
  UTEST_PRINTF("%f", UTEST_CAST(double, f));
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(double d);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(double d) {
  UTEST_PRINTF("%f", d);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long double d);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long double d) {
  UTEST_PRINTF("%Lf", d);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(int i);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(int i) {
  UTEST_PRINTF("%d", i);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(unsigned int i);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(unsigned int i) {
  UTEST_PRINTF("%u", i);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long int i);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long int i) {
  UTEST_PRINTF("%ld", i);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long unsigned int i);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long unsigned int i) {
  UTEST_PRINTF("%lu", i);
}

/*
   long long is a c++11 extension
   TODO: grok for c++11 version here
*/
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long long int i);
UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long long int i) {
  UTEST_PRINTF("%lld", i);
}

UTEST_WEAK UTEST_OVERLOADABLE void utest_type_printer(long long unsigned int i);
UTEST_WEAK UTEST_OVERLOADABLE void
utest_type_printer(long long unsigned int i) {
  UTEST_PRINTF("%llu", i);
}
#endif
#else
/*
   we don't have the ability to print the values we got, so we create a macro
   to tell our users we can't do anything fancy
*/
#define utest_type_printer(...) UTEST_PRINTF("undef")
#endif

#if defined(__clang__)
#define UTEST_EXPECT(x, y, cond)                                               \
  {                                                                            \
    _Pragma("clang diagnostic push") _Pragma(                                  \
        "clang diagnostic ignored \"-Wlanguage-extension-token\"") typeof(y)   \
        xEval = (x);                                                           \
    typeof(y) yEval = (y);                                                     \
    _Pragma("clang diagnostic pop") if (!((xEval)cond(yEval))) {               \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      UTEST_PRINTF("  Expected : ");                                           \
      utest_type_printer(xEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      UTEST_PRINTF("    Actual : ");                                           \
      utest_type_printer(yEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      *utest_result = 1;                                                       \
    }                                                                          \
  }
#elif defined(__GNUC__)
#define UTEST_EXPECT(x, y, cond)                                               \
  {                                                                            \
    typeof(y) xEval = (x);                                                     \
    typeof(y) yEval = (y);                                                     \
    if (!((xEval)cond(yEval))) {                                               \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      UTEST_PRINTF("  Expected : ");                                           \
      utest_type_printer(xEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      UTEST_PRINTF("    Actual : ");                                           \
      utest_type_printer(yEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      *utest_result = 1;                                                       \
    }                                                                          \
  }
#else
#define UTEST_EXPECT(x, y, cond)                                               \
  {                                                                            \
    if (!((x)cond(y))) {                                                       \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      *utest_result = 1;                                                       \
    }                                                                          \
  }
#endif

#define EXPECT_TRUE(x)                                                         \
  if (!(x)) {                                                                  \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : true\n");                                       \
    UTEST_PRINTF("    Actual : %s\n", (x) ? "true" : "false");                 \
    *utest_result = 1;                                                         \
  }

#define EXPECT_FALSE(x)                                                        \
  if (x) {                                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : false\n");                                      \
    UTEST_PRINTF("    Actual : %s\n", (x) ? "true" : "false");                 \
    *utest_result = 1;                                                         \
  }

#define EXPECT_EQ(x, y) UTEST_EXPECT(x, y, ==)
#define EXPECT_NE(x, y) UTEST_EXPECT(x, y, !=)
#define EXPECT_LT(x, y) UTEST_EXPECT(x, y, <)
#define EXPECT_LE(x, y) UTEST_EXPECT(x, y, <=)
#define EXPECT_GT(x, y) UTEST_EXPECT(x, y, >)
#define EXPECT_GE(x, y) UTEST_EXPECT(x, y, >=)

#define EXPECT_STREQ(x, y)                                                     \
  if (0 != strcmp(x, y)) {                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : \"%s\"\n", x);                                  \
    UTEST_PRINTF("    Actual : \"%s\"\n", y);                                  \
    *utest_result = 1;                                                         \
  }

#define EXPECT_STRNE(x, y)                                                     \
  if (0 == strcmp(x, y)) {                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : \"%s\"\n", x);                                  \
    UTEST_PRINTF("    Actual : \"%s\"\n", y);                                  \
    *utest_result = 1;                                                         \
  }

#if defined(__clang__)
#define UTEST_ASSERT(x, y, cond)                                               \
  {                                                                            \
    _Pragma("clang diagnostic push") _Pragma(                                  \
        "clang diagnostic ignored \"-Wlanguage-extension-token\"") typeof(y)   \
        xEval = (x);                                                           \
    typeof(y) yEval = (y);                                                     \
    _Pragma("clang diagnostic pop") if (!((xEval)cond(yEval))) {               \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      UTEST_PRINTF("  Expected : ");                                           \
      utest_type_printer(xEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      UTEST_PRINTF("    Actual : ");                                           \
      utest_type_printer(yEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      *utest_result = 1;                                                       \
      return;                                                                  \
    }                                                                          \
  }
#elif defined(__GNUC__)
#define UTEST_ASSERT(x, y, cond)                                               \
  {                                                                            \
    typeof(y) xEval = (x);                                                     \
    typeof(y) yEval = (y);                                                     \
    if (!((xEval)cond(yEval))) {                                               \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      UTEST_PRINTF("  Expected : ");                                           \
      utest_type_printer(xEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      UTEST_PRINTF("    Actual : ");                                           \
      utest_type_printer(yEval);                                               \
      UTEST_PRINTF("\n");                                                      \
      *utest_result = 1;                                                       \
      return;                                                                  \
    }                                                                          \
  }
#else
#define UTEST_ASSERT(x, y, cond)                                               \
  {                                                                            \
    if (!((x)cond(y))) {                                                       \
      UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                    \
      *utest_result = 1;                                                       \
      return;                                                                  \
    }                                                                          \
  }
#endif

#define ASSERT_TRUE(x)                                                         \
  if (!(x)) {                                                                  \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : true\n");                                       \
    UTEST_PRINTF("    Actual : %s\n", (x) ? "true" : "false");                 \
    *utest_result = 1;                                                         \
    return;                                                                    \
  }

#define ASSERT_FALSE(x)                                                        \
  if (x) {                                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : false\n");                                      \
    UTEST_PRINTF("    Actual : %s\n", (x) ? "true" : "false");                 \
    *utest_result = 1;                                                         \
    return;                                                                    \
  }

#define ASSERT_EQ(x, y) UTEST_ASSERT(x, y, ==)
#define ASSERT_NE(x, y) UTEST_ASSERT(x, y, !=)
#define ASSERT_LT(x, y) UTEST_ASSERT(x, y, <)
#define ASSERT_LE(x, y) UTEST_ASSERT(x, y, <=)
#define ASSERT_GT(x, y) UTEST_ASSERT(x, y, >)
#define ASSERT_GE(x, y) UTEST_ASSERT(x, y, >=)

#define ASSERT_STREQ(x, y)                                                     \
  EXPECT_STREQ(x, y);                                                          \
  if (0 != strcmp(x, y)) {                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : \"%s\"\n", x);                                  \
    UTEST_PRINTF("    Actual : \"%s\"\n", y);                                  \
    *utest_result = 1;                                                         \
    return;                                                                    \
  }

#define ASSERT_STRNE(x, y)                                                     \
  EXPECT_STRNE(x, y);                                                          \
  if (0 == strcmp(x, y)) {                                                     \
    UTEST_PRINTF("%s:%u: Failure\n", __FILE__, __LINE__);                      \
    UTEST_PRINTF("  Expected : \"%s\"\n", x);                                  \
    UTEST_PRINTF("    Actual : \"%s\"\n", y);                                  \
    *utest_result = 1;                                                         \
    return;                                                                    \
  }

#define UTEST(SET, NAME)                                                       \
  UTEST_EXTERN struct utest_state_s utest_state;                               \
  static void utest_run_##SET##_##NAME(int *utest_result);                     \
  static void utest_##SET##_##NAME(int *utest_result, size_t utest_index) {    \
    (void)utest_index;                                                         \
    utest_run_##SET##_##NAME(utest_result);                                    \
  }                                                                            \
  UTEST_INITIALIZER(utest_register_##SET##_##NAME) {                           \
    const size_t index = utest_state.tests_length++;                           \
    const char *name_part = #SET "." #NAME;                                    \
    const size_t name_size = strlen(name_part) + 1;                            \
    char *name = UTEST_PTR_CAST(char *, malloc(name_size));                    \
    utest_state.tests =                                                        \
        UTEST_PTR_CAST(struct utest_test_state_s *,                            \
                       realloc(UTEST_PTR_CAST(void *, utest_state.tests),      \
                               sizeof(struct utest_test_state_s) *             \
                                   utest_state.tests_length));                 \
    utest_state.tests[index].func = &utest_##SET##_##NAME;                     \
    utest_state.tests[index].name = name;                                      \
    UTEST_SNPRINTF(name, name_size, "%s", name_part);                          \
  }                                                                            \
  void utest_run_##SET##_##NAME(int *utest_result)

#define UTEST_F_SETUP(FIXTURE)                                                 \
  static void utest_f_setup_##FIXTURE(int *utest_result,                       \
                                      struct FIXTURE *utest_fixture)

#define UTEST_F_TEARDOWN(FIXTURE)                                              \
  static void utest_f_teardown_##FIXTURE(int *utest_result,                    \
                                         struct FIXTURE *utest_fixture)

#define UTEST_F(FIXTURE, NAME)                                                 \
  UTEST_EXTERN struct utest_state_s utest_state;                               \
  static void utest_f_setup_##FIXTURE(int *, struct FIXTURE *);                \
  static void utest_f_teardown_##FIXTURE(int *, struct FIXTURE *);             \
  static void utest_run_##FIXTURE##_##NAME(int *, struct FIXTURE *);           \
  static void utest_f_##FIXTURE##_##NAME(int *utest_result,                    \
                                         size_t utest_index) {                 \
    struct FIXTURE fixture;                                                    \
    (void)utest_index;                                                         \
    memset(&fixture, 0, sizeof(fixture));                                      \
    utest_f_setup_##FIXTURE(utest_result, &fixture);                           \
    if (0 != *utest_result) {                                                  \
      return;                                                                  \
    }                                                                          \
    utest_run_##FIXTURE##_##NAME(utest_result, &fixture);                      \
    utest_f_teardown_##FIXTURE(utest_result, &fixture);                        \
  }                                                                            \
  UTEST_INITIALIZER(utest_register_##FIXTURE##_##NAME) {                       \
    const size_t index = utest_state.tests_length++;                           \
    const char *name_part = #FIXTURE "." #NAME;                                \
    const size_t name_size = strlen(name_part) + 1;                            \
    char *name = UTEST_PTR_CAST(char *, malloc(name_size));                    \
    utest_state.tests =                                                        \
        UTEST_PTR_CAST(struct utest_test_state_s *,                            \
                       realloc(UTEST_PTR_CAST(void *, utest_state.tests),      \
                               sizeof(struct utest_test_state_s) *             \
                                   utest_state.tests_length));                 \
    utest_state.tests[index].func = &utest_f_##FIXTURE##_##NAME;               \
    utest_state.tests[index].name = name;                                      \
    UTEST_SNPRINTF(name, name_size, "%s", name_part);                          \
  }                                                                            \
  void utest_run_##FIXTURE##_##NAME(int *utest_result,                         \
                                    struct FIXTURE *utest_fixture)

#define UTEST_I_SETUP(FIXTURE)                                                 \
  static void utest_i_setup_##FIXTURE(                                         \
      int *utest_result, struct FIXTURE *utest_fixture, size_t utest_index)

#define UTEST_I_TEARDOWN(FIXTURE)                                              \
  static void utest_i_teardown_##FIXTURE(                                      \
      int *utest_result, struct FIXTURE *utest_fixture, size_t utest_index)

#define UTEST_I(FIXTURE, NAME, INDEX)                                          \
  UTEST_EXTERN struct utest_state_s utest_state;                               \
  static void utest_run_##FIXTURE##_##NAME##_##INDEX(int *, struct FIXTURE *); \
  static void utest_i_##FIXTURE##_##NAME##_##INDEX(int *utest_result,          \
                                                   size_t index) {             \
    struct FIXTURE fixture;                                                    \
    memset(&fixture, 0, sizeof(fixture));                                      \
    utest_i_setup_##FIXTURE(utest_result, &fixture, index);                    \
    if (0 != *utest_result) {                                                  \
      return;                                                                  \
    }                                                                          \
    utest_run_##FIXTURE##_##NAME##_##INDEX(utest_result, &fixture);            \
    utest_i_teardown_##FIXTURE(utest_result, &fixture, index);                 \
  }                                                                            \
  UTEST_INITIALIZER(utest_register_##FIXTURE##_##NAME##_##INDEX) {             \
    size_t i;                                                                  \
    uint64_t iUp;                                                              \
    for (i = 0; i < (INDEX); i++) {                                            \
      const size_t index = utest_state.tests_length++;                         \
      const char *name_part = #FIXTURE "." #NAME;                              \
      const size_t name_size = strlen(name_part) + 32;                         \
      char *name = UTEST_PTR_CAST(char *, malloc(name_size));                  \
      utest_state.tests =                                                      \
          UTEST_PTR_CAST(struct utest_test_state_s *,                          \
                         realloc(UTEST_PTR_CAST(void *, utest_state.tests),    \
                                 sizeof(struct utest_test_state_s) *           \
                                     utest_state.tests_length));               \
      utest_state.tests[index].func = &utest_i_##FIXTURE##_##NAME##_##INDEX;   \
      utest_state.tests[index].index = i;                                      \
      utest_state.tests[index].name = name;                                    \
      iUp = UTEST_CAST(uint64_t, i);                                           \
      UTEST_SNPRINTF(name, name_size, "%s/%" UTEST_PRIu64, name_part, iUp);    \
    }                                                                          \
  }                                                                            \
  void utest_run_##FIXTURE##_##NAME##_##INDEX(int *utest_result,               \
                                              struct FIXTURE *utest_fixture)

UTEST_WEAK
int utest_should_filter_test(const char *filter, const char *testcase);
UTEST_WEAK int utest_should_filter_test(const char *filter,
                                        const char *testcase) {
  if (filter) {
    const char *filter_cur = filter;
    const char *testcase_cur = testcase;
    const char *filter_wildcard = 0;

    while (('\0' != *filter_cur) && ('\0' != *testcase_cur)) {
      if ('*' == *filter_cur) {
        /* store the position of the wildcard */
        filter_wildcard = filter_cur;

        /* skip the wildcard character */
        filter_cur++;

        while (('\0' != *filter_cur) && ('\0' != *testcase_cur)) {
          if ('*' == *filter_cur) {
            /*
               we found another wildcard (filter is something like *foo*) so we
               exit the current loop, and return to the parent loop to handle
               the wildcard case
            */
            break;
          } else if (*filter_cur != *testcase_cur) {
            /* otherwise our filter didn't match, so reset it */
            filter_cur = filter_wildcard;
          }

          /* move testcase along */
          testcase_cur++;

          /* move filter along */
          filter_cur++;
        }

        if (('\0' == *filter_cur) && ('\0' == *testcase_cur)) {
          return 0;
        }

        /* if the testcase has been exhausted, we don't have a match! */
        if ('\0' == *testcase_cur) {
          return 1;
        }
      } else {
        if (*testcase_cur != *filter_cur) {
          /* test case doesn't match filter */
          return 1;
        } else {
          /* move our filter and testcase forward */
          testcase_cur++;
          filter_cur++;
        }
      }
    }

    if (('\0' != *filter_cur) ||
        (('\0' != *testcase_cur) &&
         ((filter == filter_cur) || ('*' != filter_cur[-1])))) {
      /* we have a mismatch! */
      return 1;
    }
  }

  return 0;
}

static UTEST_INLINE int utest_strncmp(const char *a, const char *b, size_t n) {
  /* strncmp breaks on Wall / Werror on gcc/clang, so we avoid using it */
  unsigned i;

  for (i = 0; i < n; i++) {
    if (a[i] < b[i]) {
      return -1;
    } else if (a[i] > b[i]) {
      return 1;
    }
  }

  return 0;
}

static UTEST_INLINE FILE *utest_fopen(const char *filename, const char *mode) {
#ifdef _MSC_VER
  FILE *file;
  if (0 == fopen_s(&file, filename, mode)) {
    return file;
  } else {
    return 0;
  }
#else
  return fopen(filename, mode);
#endif
}

UTEST_WEAK int utest_main(int argc, const char *const argv[]);
UTEST_WEAK int utest_main(int argc, const char *const argv[]) {
  uint64_t failed = 0;
  size_t index = 0;
  size_t *failed_testcases = 0;
  size_t failed_testcases_length = 0;
  const char *filter = 0;
  uint64_t ran_tests = 0;

  enum colours { RESET, GREEN, RED };

  const int use_colours = UTEST_COLOUR_OUTPUT();
  const char *colours[] = {"\033[0m", "\033[32m", "\033[31m"};
  if (!use_colours) {
    for (index = 0; index < sizeof colours / sizeof colours[0]; index++) {
      colours[index] = "";
    }
  }
  /* loop through all arguments looking for our options */
  for (index = 1; index < UTEST_CAST(size_t, argc); index++) {
    const char help_str[] = "--help";
    const char filter_str[] = "--filter=";
    const char output_str[] = "--output=";

    if (0 == utest_strncmp(argv[index], help_str, strlen(help_str))) {
      printf("utest.h - the single file unit testing solution for C/C++!\n"
             "Command line Options:\n"
             "  --help            Show this message and exit.\n"
             "  --filter=<filter> Filter the test cases to run (EG. MyTest*.a "
             "would run MyTestCase.a but not MyTestCase.b).\n"
             "  --output=<output> Output an xunit XML file to the file "
             "specified in <output>.\n");
      goto cleanup;
    } else if (0 ==
               utest_strncmp(argv[index], filter_str, strlen(filter_str))) {
      /* user wants to filter what test cases run! */
      filter = argv[index] + strlen(filter_str);
    } else if (0 ==
               utest_strncmp(argv[index], output_str, strlen(output_str))) {
      utest_state.output = utest_fopen(argv[index] + strlen(output_str), "w+");
    }
  }

  for (index = 0; index < utest_state.tests_length; index++) {
    if (utest_should_filter_test(filter, utest_state.tests[index].name)) {
      continue;
    }

    ran_tests++;
  }

  printf("%s[==========]%s Running %" UTEST_PRIu64 " test cases.\n",
         colours[GREEN], colours[RESET], UTEST_CAST(uint64_t, ran_tests));

  if (utest_state.output) {
    fprintf(utest_state.output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(utest_state.output,
            "<testsuites tests=\"%" UTEST_PRIu64 "\" name=\"All\">\n",
            UTEST_CAST(uint64_t, ran_tests));
    fprintf(utest_state.output,
            "<testsuite name=\"Tests\" tests=\"%" UTEST_PRIu64 "\">\n",
            UTEST_CAST(uint64_t, ran_tests));
  }

  for (index = 0; index < utest_state.tests_length; index++) {
    int result = 0;
    int64_t ns = 0;

    if (utest_should_filter_test(filter, utest_state.tests[index].name)) {
      continue;
    }

    printf("%s[ RUN      ]%s %s\n", colours[GREEN], colours[RESET],
           utest_state.tests[index].name);

    if (utest_state.output) {
      fprintf(utest_state.output, "<testcase name=\"%s\">",
              utest_state.tests[index].name);
    }

    ns = utest_ns();
    utest_state.tests[index].func(&result, utest_state.tests[index].index);
    ns = utest_ns() - ns;

    if (utest_state.output) {
      fprintf(utest_state.output, "</testcase>\n");
    }

    if (0 != result) {
      const size_t failed_testcase_index = failed_testcases_length++;
      failed_testcases = UTEST_PTR_CAST(
          size_t *, realloc(UTEST_PTR_CAST(void *, failed_testcases),
                            sizeof(size_t) * failed_testcases_length));
      failed_testcases[failed_testcase_index] = index;
      failed++;
      printf("%s[  FAILED  ]%s %s (%" UTEST_PRId64 "ns)\n", colours[RED],
             colours[RESET], utest_state.tests[index].name, ns);
    } else {
      printf("%s[       OK ]%s %s (%" UTEST_PRId64 "ns)\n", colours[GREEN],
             colours[RESET], utest_state.tests[index].name, ns);
    }
  }

  printf("%s[==========]%s %" UTEST_PRIu64 " test cases ran.\n", colours[GREEN],
         colours[RESET], ran_tests);
  printf("%s[  PASSED  ]%s %" UTEST_PRIu64 " tests.\n", colours[GREEN],
         colours[RESET], ran_tests - failed);

  if (0 != failed) {
    printf("%s[  FAILED  ]%s %" UTEST_PRIu64 " tests, listed below:\n",
           colours[RED], colours[RESET], failed);
    for (index = 0; index < failed_testcases_length; index++) {
      printf("%s[  FAILED  ]%s %s\n", colours[RED], colours[RESET],
             utest_state.tests[failed_testcases[index]].name);
    }
  }

  if (utest_state.output) {
    fprintf(utest_state.output, "</testsuite>\n</testsuites>\n");
  }

cleanup:
  for (index = 0; index < utest_state.tests_length; index++) {
    free(UTEST_PTR_CAST(void *, utest_state.tests[index].name));
  }

  free(UTEST_PTR_CAST(void *, failed_testcases));
  free(UTEST_PTR_CAST(void *, utest_state.tests));

  if (utest_state.output) {
    fclose(utest_state.output);
  }

  return UTEST_CAST(int, failed);
}

/*
   we need, in exactly one source file, define the global struct that will hold
   the data we need to run utest. This macro allows the user to declare the
   data without having to use the UTEST_MAIN macro, thus allowing them to write
   their own main() function.
*/
#define UTEST_STATE() struct utest_state_s utest_state = {0, 0, 0}

/*
   define a main() function to call into utest.h and start executing tests! A
   user can optionally not use this macro, and instead define their own main()
   function and manually call utest_main. The user must, in exactly one source
   file, use the UTEST_STATE macro to declare a global struct variable that
   utest requires.
*/
#define UTEST_MAIN()                                                           \
  UTEST_STATE();                                                               \
  int main(int argc, const char *const argv[]) {                               \
    return utest_main(argc, argv);                                             \
  }

#endif /* SHEREDOM_UTEST_H_INCLUDED */
