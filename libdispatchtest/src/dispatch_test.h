#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <SDKDDKVer.h>
#include <Windows.h>

#include <math.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include "os_shims.h"
#include <dispatch/dispatch.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_start(const char* desc);
void test_stop(void);
void test_stop_after_delay(void *delay);

void _test_ptr_null(const char* file, long line, const char* desc, const void* ptr);
#define test_ptr_null(a,b) _test_ptr_null(__FILE__, __LINE__, a, b)

void _test_ptr_notnull(const char* file, long line, const char* desc, const void* ptr);
#define test_ptr_notnull(a,b) _test_ptr_notnull(__FILE__, __LINE__, a, b)

void _test_ptr(const char* file, long line, const char* desc, const void* actual, const void* expected);
#define test_ptr(a,b,c) _test_ptr(__FILE__, __LINE__, a, b, c)

void _test_long(const char* file, long line, const char* desc, long actual, long expected);
#define test_long(a,b,c) _test_long(__FILE__, __LINE__, a, b, c)

void _test_long_less_than(const char* file, long line, const char* desc, long actual, long max_expected);
#define test_long_less_than(a,b,c) _test_long_less_than(__FILE__, __LINE__, a, b, c)

void _test_time_less_than(const char* file, long line, const char* desc, dispatch_time_t actual, dispatch_time_t max_expected);
#define test_time_less_than(a,b,c) _test_time_less_than(__FILE__, __LINE__, a, b, c)

void _test_double_less_than_or_equal(const char* file, long line, const char* desc, double val, double max_expected);
#define test_double_less_than_or_equal(d, v, m) _test_double_less_than(__FILE__, __LINE__, d, v, m)

void _test_double_less_than(const char* file, long line, const char* desc, double val, double max_expected);
#define test_double_less_than(d, v, m) _test_double_less_than(__FILE__, __LINE__, d, v, m)

void _test_errno(const char* file, long line, const char* desc, long actual, long expected);
#define test_errno(a,b,c) _test_errno(__FILE__, __LINE__, a, b, c)

#ifdef __cplusplus
}
#endif
