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

#include "config/config.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_WIN32
// Include Win32 headers early in order to minimize the
// likelihood of name pollution from dispatch headers.

#ifndef WINVER
#define WINVER 0x502
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x502
#endif

#ifndef _MSC_VER
#define _MSC_VER 1400
#pragma warning(disable:4159)
#endif

#define WIN32_LEAN_AND_MEAN 1
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#define BOOL WINBOOL
#include <Windows.h>
#undef BOOL

#endif /* TARGET_OS_WIN32 */

#define __DISPATCH_BUILDING_DISPATCH__
#define __DISPATCH_INDIRECT__

#include "dispatch/dispatch.h"
#include "dispatch/base.h"
#include "dispatch/time.h"
#include "dispatch/queue.h"
#include "dispatch/object.h"
#include "dispatch/source.h"
#include "dispatch/group.h"
#include "dispatch/semaphore.h"
#include "dispatch/once.h"
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


#if HAVE_LIBKERN_OSCROSSENDIAN_H
#include <libkern/OSCrossEndian.h>
#endif
#if HAVE_LIBKERN_OSATOMIC_H
#include <libkern/OSAtomic.h>
#endif
#if HAVE_MACH
#include <mach/boolean.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/exception.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_interface.h>
#include <mach/mach_time.h>
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/host_info.h>
#include <mach/notify.h>
#endif /* HAVE_MACH */
#if HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/stat.h>
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#ifdef __BLOCKS__
#if TARGET_OS_WIN32
#define BLOCK_EXPORT extern "C" __declspec(dllexport)
#endif /* TARGET_OS_WIN32 */
#include <Block_private.h>
#include <Block.h>
#endif /* __BLOCKS__ */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <search.h>
#if USE_POSIX_SEM
#include <semaphore.h>
#endif
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define DISPATCH_NOINLINE	__attribute__((noinline))

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

void _dispatch_bug(size_t line, long val) __attribute__((__noinline__));
void _dispatch_abort(size_t line, long val) __attribute__((__noinline__,__noreturn__));
void _dispatch_log(const char *msg, ...) __attribute__((__noinline__,__format__(printf,1,2)));
void _dispatch_logv(const char *msg, va_list) __attribute__((__noinline__,__format__(printf,1,0)));

/*
 * For reporting bugs within libdispatch when using the "_debug" version of the library.
 */
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

/*
 * For reporting bugs or impedance mismatches between libdispatch and external subsystems.
 * These do NOT abort(), and are always compiled into the product.
 *
 * In particular, we wrap all system-calls with assume() macros.
 */
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

/*
 * For reporting bugs in clients when using the "_debug" version of the library.
 */
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
#define _dispatch_debug(x, args...)	\
({	\
	if (DISPATCH_DEBUG) {	\
		_dispatch_log("libdispatch: %u\t%p\t" x, __LINE__,	\
		    (void *)_dispatch_thread_self(), ##args);	\
	}	\
})


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
