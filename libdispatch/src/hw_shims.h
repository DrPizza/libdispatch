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

#ifndef __DISPATCH_HW_SHIMS__
#define __DISPATCH_HW_SHIMS__

/* x86 has a 64 byte cacheline */
#define DISPATCH_CACHELINE_SIZE	64
#define ROUND_UP_TO_CACHELINE_SIZE(x)	(((x) + (DISPATCH_CACHELINE_SIZE - 1)) & ~(DISPATCH_CACHELINE_SIZE - 1))
#define ROUND_UP_TO_VECTOR_SIZE(x)	(((x) + 15) & ~15)

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)
// GCC generates suboptimal register pressure
// LLVM does better, but doesn't support tail calls
// 6248590 __sync_*() intrinsics force a gratuitous "lea" instruction, with resulting register pressure
#if 0 && defined(__i386__) || defined(__x86_64__)
#define dispatch_atomic_xchg(p, n)	({ typeof(*(p)) _r; asm("xchg %0, %1" : "=r" (_r) : "m" (*(p)), "0" (n)); _r; })
#else
#define dispatch_atomic_xchg(p, n)	((typeof(*(p)))__sync_lock_test_and_set((p), (n)))
#endif
#define dispatch_atomic_xchg_pointer	dispatch_atomic_xchg
#define dispatch_atomic_cmpxchg(p, o, n)	__sync_bool_compare_and_swap((p), (o), (n))
#define dispatch_atomic_cmpxchg_pointer	dispatch_atomic_cmpxchg
#define dispatch_atomic_inc(p)	__sync_add_and_fetch((p), 1)
#define dispatch_atomic_dec(p)	__sync_sub_and_fetch((p), 1)
#define dispatch_atomic_add(p, v)	__sync_add_and_fetch((p), (v))
#define dispatch_atomic_sub(p, v)	__sync_sub_and_fetch((p), (v))
#define dispatch_atomic_or(p, v)	__sync_fetch_and_or((p), (v))
#define dispatch_atomic_and(p, v)	__sync_fetch_and_and((p), (v))
#if defined(__i386__) || defined(__x86_64__)
/* GCC emits nothing for __sync_synchronize() on i386/x86_64. */
#define dispatch_atomic_barrier()	__asm__ __volatile__("mfence")
#else
#define dispatch_atomic_barrier()	__sync_synchronize()
#endif
#elif _MSC_VER
#include <intrin.h>
#if defined(__i386__)
#pragma intrinsic(_InterlockedExchange)
#define dispatch_atomic_xchg(p, n)	_InterlockedExchange((p), (n))
#define dispatch_atomic_xchg_pointer(p, n)	InterlockedExchangePointer((p), (n))
#pragma intrinsic(_InterlockedCompareExchange)
#define _dispatch_atomic_cmpxchg(p, o, n)	_InterlockedCompareExchange((p), (n), (o))
#define _dispatch_atomic_cmpxchg_pointer(p, o, n)	InterlockedCompareExchangePointer((p), (n), (o))
#pragma intrinsic(_InterlockedIncrement)
#define dispatch_atomic_inc(p)	_InterlockedIncrement((p))
#pragma intrinsic(_InterlockedDecrement)
#define dispatch_atomic_dec(p)	_InterlockedDecrement((p))
#pragma intrinsic(_InterlockedExchangeAdd)
#define dispatch_atomic_add(p, v)	(_InterlockedExchangeAdd((p), +(signed long)(v)), *(p))
#define dispatch_atomic_sub(p, v)	(_InterlockedExchangeAdd((p), -(signed long)(v)), *(p))
#pragma intrinsic(_InterlockedOr)
#define dispatch_atomic_or(p, v)	_InterlockedOr((p), (v))
#pragma intrinsic(_InterlockedAnd)
#define dispatch_atomic_and(p, v)	_InterlockedAnd((p), (v))
#elif defined(__x86_64__)
#pragma intrinsic(_InterlockedExchange64)
#define dispatch_atomic_xchg(p, n)	_InterlockedExchange64((p), (n))
#pragma intrinsic(_InterlockedExchangePointer)
#define dispatch_atomic_xchg_pointer(p, n)	_InterlockedExchangePointer((p), (n))
#pragma intrinsic(_InterlockedCompareExchange64)
#define _dispatch_atomic_cmpxchg(p, o, n)	_InterlockedCompareExchange64((p), (n), (o))
#pragma intrinsic(_InterlockedCompareExchangePointer)
#define _dispatch_atomic_cmpxchg_pointer(p, o, n)	_InterlockedCompareExchangePointer((p), (n), (o))
#pragma intrinsic(_InterlockedIncrement64)
#define dispatch_atomic_inc(p)	_InterlockedIncrement64((p))
#pragma intrinsic(_InterlockedDecrement64)
#define dispatch_atomic_dec(p)	_InterlockedDecrement64((p))
#pragma intrinsic(_InterlockedExchangeAdd64)
#define dispatch_atomic_add(p, v)	(_InterlockedExchangeAdd64((p), +(signed __int64)(v)), *(p))
#define dispatch_atomic_sub(p, v)	(_InterlockedExchangeAdd64((p), -(signed __int64)(v)), *(p))
#pragma intrinsic(_InterlockedOr)
#define dispatch_atomic_or(p, v)	_InterlockedOr64((p), (v))
#pragma intrinsic(_InterlockedAnd)
#define dispatch_atomic_and(p, v)	_InterlockedAnd64((p), (v))
#endif

__forceinline bool dispatch_atomic_cmpxchg(intptr_t* p, intptr_t o, intptr_t n)
{
	return o == _dispatch_atomic_cmpxchg(p, o, n);
}

__forceinline bool dispatch_atomic_cmpxchg_pointer(void* volatile* p, void* o, void* n)
{
	return o == _dispatch_atomic_cmpxchg_pointer(p, o, n);
}

#pragma intrinsic(_mm_mfence)
#define dispatch_atomic_barrier()	_mm_mfence()
#else
#error "Please upgrade to GCC 4.2 or newer."
#endif

#if __GNUC__
#if defined(__i386__) || defined(__x86_64__)
#define _dispatch_hardware_pause() asm("pause")
#define _dispatch_debugger() asm("int3")
#else
#define _dispatch_hardware_pause() asm("")
#define _dispatch_debugger() asm("trap")
#endif
// really just a low level abort()
#define _dispatch_hardware_crash() __builtin_trap()
#elif _MSC_VER
#define _dispatch_hardware_pause() YieldProcessor()
#define _dispatch_debugger() DebugBreak()
#define _dispatch_hardware_crash() (DebugBreak(), FatalExit(-1))
#endif

#endif
