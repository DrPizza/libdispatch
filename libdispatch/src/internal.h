/*
 * Copyright (c) 2008-2009 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

/*
 * IMPORTANT: This header file describes INTERNAL interfaces to libdispatch
 * which are subject to change in future releases of Mac OS X. Any applications
 * relying on these interfaces WILL break.
 */

#ifndef __DISPATCH_INTERNAL__
#define __DISPATCH_INTERNAL__

#include "platform/platform.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#define __DISPATCH_BUILDING_DISPATCH__
#define __DISPATCH_INDIRECT__

#include "dispatch/dispatch.h"
#include "dispatch/base.h"
#include "dispatch/atomic.h"
#include "dispatch/time.h"
#include "dispatch/queue.h"
#include "dispatch/object.h"
#include "dispatch/source.h"
#include "dispatch/group.h"
#include "dispatch/semaphore.h"
#include "dispatch/once.h"
#include "dispatch/interop.h"
#include "dispatch/benchmark.h"

/* private.h uses #include_next and must be included last to avoid picking
 * up installed headers. */
#include "queue_private.h"
#include "source_private.h"
#include "private.h"

#ifndef DISPATCH_NO_LEGACY
#include "legacy.h"
#endif
/* More #includes at EOF (dependent on the contents of internal.h) ... */

/* The "_debug" library build */
#ifndef DISPATCH_DEBUG
#define DISPATCH_DEBUG 0
#endif

#ifdef __GNUC__
#define DISPATCH_NOINLINE __attribute__((noinline))
#elif _MSC_VER
#define DISPATCH_NOINLINE __declspec(noinline)
#endif

#ifdef __GNUC__
#define DISPATCH_INLINE __attribute__((always_inline)) inline
#elif _MSC_VER
#define DISPATCH_INLINE __forceinline
#endif

#ifdef __GNUC__
#define DISPATCH_UNUSED __attribute__((unused))
#else
#define DISPATCH_UNUSED
#endif

// workaround 6368156
#ifdef NSEC_PER_SEC
#undef NSEC_PER_SEC
#endif
#ifdef USEC_PER_SEC
#undef USEC_PER_SEC
#endif
#ifdef NSEC_PER_USEC
#undef NSEC_PER_USEC
#endif
#define NSEC_PER_SEC 1000000000ull
#define USEC_PER_SEC 1000000ull
#define NSEC_PER_USEC 1000ull

/* I wish we had __builtin_expect_range() */
#if __GNUC__
#define fastpath(x)	((typeof(x))__builtin_expect((long)(x), ~0l))
#define slowpath(x)	((typeof(x))__builtin_expect((long)(x), 0l))
#else
#define fastpath(x) (x)
#define slowpath(x) (x)
#endif

#if __GNUC__
void _dispatch_bug(size_t line, long val) __attribute__((__noinline__));
void _dispatch_abort(size_t line, long val) __attribute__((__noinline__,__noreturn__));
void _dispatch_log(const char *msg, ...) __attribute__((__noinline__,__format__(printf,1,2)));
void _dispatch_logv(const char *msg, va_list) __attribute__((__noinline__,__format__(printf,1,0)));
#else
DISPATCH_NOINLINE void _dispatch_bug(size_t line, long val);
DISPATCH_NOINLINE __declspec(noreturn) void _dispatch_abort(size_t line, long val);
DISPATCH_NOINLINE void _dispatch_log(/*_Printf_format_string_*/const char *msg, ...);
DISPATCH_NOINLINE void _dispatch_logv(/*_Printf_format_string_*/const char *msg, va_list);
#endif

/*
 * For reporting bugs within libdispatch when using the "_debug" version of the library.
 */
#ifdef __GNUC__
#define dispatch_assert(e)	do {	\
		if (__builtin_constant_p(e)) {	\
			char __compile_time_assert__[(bool)(e) ? 1 : -1] __attribute__((unused));	\
		} else {	\
			typeof(e) _e = fastpath(e); /* always eval 'e' */	\
			if (DISPATCH_DEBUG && !_e) {	\
				_dispatch_abort(__LINE__, (long)_e);	\
			}	\
		}	\
	} while (0)
/* A lot of API return zero upon success and not-zero on fail. Let's capture and log the non-zero value */
#define dispatch_assert_zero(e)	do {	\
		if (__builtin_constant_p(e)) {	\
			char __compile_time_assert__[(bool)(!(e)) ? 1 : -1] __attribute__((unused));	\
		} else {	\
			typeof(e) _e = slowpath(e); /* always eval 'e' */	\
			if (DISPATCH_DEBUG && _e) {	\
				_dispatch_abort(__LINE__, (long)_e);	\
			}	\
		}	\
	} while (0)
#else
#define dispatch_assert(e)	do {	\
		uintptr_t _e = (uintptr_t)fastpath(e); /* always eval 'e' */	\
		if (DISPATCH_DEBUG && !_e) {	\
			_dispatch_abort(__LINE__, (long)_e);	\
		}	\
	} while (0)
/* A lot of API return zero upon success and not-zero on fail. Let's capture and log the non-zero value */
#define dispatch_assert_zero(e)	do {	\
		uintptr_t _e = (uintptr_t)slowpath(e); /* always eval 'e' */	\
		if (DISPATCH_DEBUG && _e) {	\
			_dispatch_abort(__LINE__, (long)_e);	\
		}	\
	} while (0)
#endif
/*
 * For reporting bugs or impedance mismatches between libdispatch and external subsystems.
 * These do NOT abort(), and are always compiled into the product.
 *
 * In particular, we wrap all system-calls with assume() macros.
 */
#ifdef __GNUC__
#define dispatch_assume(e)	({	\
		typeof(e) _e = fastpath(e); /* always eval 'e' */	\
		if (!_e) {	\
			if (__builtin_constant_p(e)) {	\
				char __compile_time_assert__[(e) ? 1 : -1];	\
				(void)__compile_time_assert__;	\
			}	\
			_dispatch_bug(__LINE__, (long)_e);	\
		}	\
		_e;	\
	})
/* A lot of API return zero upon success and not-zero on fail. Let's capture and log the non-zero value */
#define dispatch_assume_zero(e)	({	\
		typeof(e) _e = slowpath(e); /* always eval 'e' */	\
		if (_e) {	\
			if (__builtin_constant_p(e)) {	\
				char __compile_time_assert__[(e) ? -1 : 1];	\
				(void)__compile_time_assert__;	\
			}	\
			_dispatch_bug(__LINE__, (long)_e);	\
		}	\
		_e;	\
	})
#else
#define dispatch_assume(e)	do{	\
		uintptr_t _e = (uintptr_t)fastpath(e); /* always eval 'e' */	\
		if (!_e) {	\
			_dispatch_bug(__LINE__, (long)_e);	\
		}	\
		_e;	\
	}while(0)
/* A lot of API return zero upon success and not-zero on fail. Let's capture and log the non-zero value */
#define dispatch_assume_zero(e)	do{	\
		uintptr_t _e = (uintptr_t)slowpath(e); /* always eval 'e' */	\
		if (_e) {	\
			_dispatch_bug(__LINE__, (long)_e);	\
		}	\
		_e;	\
	}while(0)
#endif
/*
 * For reporting bugs in clients when using the "_debug" version of the library.
 */
#ifdef __GNUC__
#define dispatch_debug_assert(e, msg, args...)	do {	\
		if (__builtin_constant_p(e)) {	\
			char __compile_time_assert__[(bool)(e) ? 1 : -1] __attribute__((unused));	\
		} else {	\
			typeof(e) _e = fastpath(e); /* always eval 'e' */	\
			if (DISPATCH_DEBUG && !_e) {	\
				_dispatch_log("%s() 0x%lx: " msg, __func__, (long)_e, ##args);	\
				abort();	\
			}	\
		}	\
	} while (0)
#else
#define dispatch_debug_assert(e, msg, ...)	do {	\
		uintptr_t _e = (uintptr_t)fastpath(e); /* always eval 'e' */	\
		if (DISPATCH_DEBUG && !_e) {	\
			_dispatch_log("%s() 0x%lx: " msg, __func__, (long)_e, __VA_ARGS__);	\
			abort();	\
		}	\
	} while (0)
#endif

#if __GNUC__
#define DO_CAST(x) ((struct dispatch_object_s *)(x)._do)
#else
#define DO_CAST(x) ((struct dispatch_object_s *)(x))
#endif

#ifdef __BLOCKS__
dispatch_block_t _dispatch_Block_copy(dispatch_block_t block);
void _dispatch_call_block_and_release(void *block);
void _dispatch_call_block_and_release2(void *block, void *ctxt);
#endif /* __BLOCKS__ */

void dummy_function(void);
long dummy_function_r0(void);

/* Make sure the debug statments don't get too stale */
#ifdef __GNUC__
#define _dispatch_debug(x, args...)	\
({	\
	if (DISPATCH_DEBUG) {	\
		_dispatch_log("libdispatch: %u\t%p\t" x, __LINE__,	\
		    (void *)_dispatch_thread_self(), ##args);	\
	}	\
})
#else
#define _dispatch_debug(x, ...)	\
do{	\
	if (DISPATCH_DEBUG) {	\
		_dispatch_log("libdispatch: %u\t%p\t" x, __LINE__,	\
		    (void *)_dispatch_thread_self(), __VA_ARGS__);	\
	}	\
}while(0)
#endif

uint64_t _dispatch_get_nanoseconds(void);

#ifndef DISPATCH_NO_LEGACY
dispatch_source_t
_dispatch_source_create2(dispatch_source_t ds,
	dispatch_source_attr_t attr,
	void *context,
	dispatch_source_handler_function_t handler);
#endif

void _dispatch_run_timers(void);
// Returns howsoon with updated time value, or NULL if no timers active.
struct timespec *_dispatch_get_next_timer_fire(struct timespec *howsoon);

dispatch_semaphore_t _dispatch_get_thread_semaphore(void);
void _dispatch_put_thread_semaphore(dispatch_semaphore_t);

bool _dispatch_source_testcancel(dispatch_source_t);

uint64_t _dispatch_timeout(dispatch_time_t when);
#if USE_POSIX_SEM
struct timespec _dispatch_timeout_ts(dispatch_time_t when);
#endif

__private_extern__ bool _dispatch_safe_fork;

__private_extern__ struct _dispatch_hw_config_s {
	uint32_t cc_max_active;
	uint32_t cc_max_logical;
	uint32_t cc_max_physical;
} _dispatch_hw_config;

/* #includes dependent on internal.h */
#include "object_internal.h"
#include "hw_shims.h"
#include "os_shims.h"
#include "queue_internal.h"
#include "semaphore_internal.h"
#include "source_internal.h"
#include "interop_internal.h"

#if USE_APPLE_CRASHREPORTER_INFO

#if HAVE_MACH
// MIG_REPLY_MISMATCH means either:
// 1) A signal handler is NOT using async-safe API. See the sigaction(2) man page for more info.
// 2) A hand crafted call to mach_msg*() screwed up. Use MIG.
#define DISPATCH_VERIFY_MIG(x) do {	\
		if ((x) == MIG_REPLY_MISMATCH) {	\
			__crashreporter_info__ = "MIG_REPLY_MISMATCH";	\
			_dispatch_hardware_crash();	\
		}	\
	} while (0)
#endif

#if defined(__x86_64__) || defined(__i386__)
// total hack to ensure that return register of a function is not trashed
#define DISPATCH_CRASH(x)	do {	\
		asm("mov	%1, %0" : "=m" (__crashreporter_info__) : "c" ("BUG IN LIBDISPATCH: " x));	\
		_dispatch_hardware_crash();	\
	} while (0)

#define DISPATCH_CLIENT_CRASH(x)	do {	\
		asm("mov	%1, %0" : "=m" (__crashreporter_info__) : "c" ("BUG IN CLIENT OF LIBDISPATCH: " x));	\
		_dispatch_hardware_crash();	\
	} while (0)

#else /* !(defined(__x86_64__) || defined(__i386__)) */

#define DISPATCH_CRASH(x)	do {	\
		__crashreporter_info__ = "BUG IN LIBDISPATCH: " x;	\
		_dispatch_hardware_crash();	\
	} while (0)

#define DISPATCH_CLIENT_CRASH(x)	do {	\
		__crashreporter_info__ = "BUG IN CLIENT OF LIBDISPATCH: " x;	\
		_dispatch_hardware_crash();	\
	} while (0)
#endif /* defined(__x86_64__) || defined(__i386__) */

#else /* !USE_APPLE_CRASHREPORTER_INFO */

#if HAVE_MACH
#define DISPATCH_VERIFY_MIG(x) do {	\
		if ((x) == MIG_REPLY_MISMATCH) {	\
			_dispatch_hardware_crash();	\
		}	\
	} while (0)
#endif

#define	DISPATCH_CRASH(x)		_dispatch_hardware_crash()
#define	DISPATCH_CLIENT_CRASH(x)	_dispatch_hardware_crash()

#endif /* USE_APPLE_CRASHREPORTER_INFO */

#endif /* __DISPATCH_INTERNAL__ */
