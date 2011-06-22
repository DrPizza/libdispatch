#ifndef DISPATCH__HPP
#define DISPATCH__HPP

#include "dispatch/dispatch.h"

#include <functional>
#include <cstdint>

#if __GNUC__
#define DISPATCHPP_EXPORT extern __attribute__((visibility("default")))
#else
#if defined(_WINDLL) // building a DLL
#define DISPATCHPP_EXPORT __declspec(dllexport)
#elif defined(_LIB) // building a static lib
#define DISPATCHPP_EXPORT
#elif defined(USE_LIB) // linking a static lib
#pragma message("Applications must link using /OPT:ICF, COMDAT folding, due to quirks of the Microsoft linker")
#define DISPATCHPP_EXPORT
#else
#define DISPATCHPP_EXPORT __declspec(dllimport)
#endif
#endif

#ifdef dispatch_once
#undef dispatch_once
#endif

#ifdef dispatch_once_f
#undef dispatch_once_f
#endif

#ifdef dispatch_get_main_queue
#undef dispatch_get_main_queue
#endif

namespace gcd
{
	typedef std::function<void (void)> function_t;
	typedef std::function<void (size_t)> counted_function_t;

	struct queue;
	struct group;

	struct DISPATCHPP_EXPORT object
	{
		object(const object& rhs);
		void debug(const char* message, ...) const;
		void retain() const;
		void release() const;
		void suspend() const;
		void resume() const;
		void set_target_queue(queue& target) const;
		void set_context(void* context) const;
		void* get_context() const;
		void set_finalizer(function_t finalizer) const;
		void set_finalizer(::dispatch_function_t finalizer) const;
		virtual ~object();
		friend struct queue;
		friend struct group;
		friend struct source;

	protected:
		object(::dispatch_object_t o_);
		void* ensure_context() const;
		::dispatch_object_t o;
	};

	struct DISPATCHPP_EXPORT queue : object
	{
		queue(const char* label, ::dispatch_queue_attr_t attributes);
		static queue get_main_queue();
		static queue get_current_thread_queue();
		static queue get_current_queue();
		static queue get_global_queue(long priority, unsigned long flags);
		const char* get_label() const;
		void after(::dispatch_time_t when, void* context, ::dispatch_function_t work) const;
		void after(::dispatch_time_t when, function_t work) const;
		void apply(size_t iterations, void* context, ::dispatch_function_apply_t work) const;
		void apply(size_t iterations, counted_function_t work) const;
		void async(void* context, ::dispatch_function_t work) const;
		void async(function_t work) const;
		void sync(void* context, ::dispatch_function_t work) const;
		void sync(function_t work) const;
	protected:
		queue(::dispatch_queue_t q);
	};

	struct DISPATCHPP_EXPORT group : object
	{
		group();
		void async(queue& queue, void* context, ::dispatch_function_t work) const;
		void async(queue& queue, function_t work) const;
		void enter() const;
		void leave() const;
		long wait(::dispatch_time_t when) const;
		void notify(queue& queue, void* context, ::dispatch_function_t work) const;
		void notify(queue& queue, function_t work) const;
	};

	struct DISPATCHPP_EXPORT source : object
	{
		source(::dispatch_source_type_t type, uintptr_t handle, uintptr_t mask, queue& queue);
		long test_cancel() const;
		void cancel() const;
		uintptr_t get_data() const;
		uintptr_t get_handle() const;
		uintptr_t get_mask() const;
		void merge_data(uintptr_t value) const;
		void set_timer(::dispatch_time_t start, uint64_t interval, uint64_t leeway) const;
		void set_event_handler(function_t handler) const;
		void set_event_handler(::dispatch_function_t handler) const;
		void set_cancel_handler(function_t handler) const;
		void set_cancel_handler(::dispatch_function_t handler) const;
	};

	struct DISPATCHPP_EXPORT semaphore : object
	{
		semaphore(long value);
		void signal();
		long wait(dispatch_time_t timeout);
	};

	DISPATCHPP_EXPORT void dispatch_once(::dispatch_once_t& predicate, function_t work);
}

#endif
