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

namespace gcd
{
	typedef std::function<void (void)> function_t;
	typedef std::function<void (size_t)> counted_function_t;

	struct queue;
	struct group;

	struct DISPATCHPP_EXPORT object
	{
		object(const object& rhs);
		void debug(const char* message, ...);
		void retain();
		void release();
		void suspend();
		void resume();
		void set_target_queue(queue& target);
		void set_context(void* context);
		void* get_context();
		void set_finalizer(function_t finalizer);
		void set_finalizer(::dispatch_function_t finalizer);
		virtual ~object();
		friend struct queue;
		friend struct group;
		friend struct source;

	protected:
		object(::dispatch_object_t o_);
		void* ensure_context();
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
		void after(::dispatch_time_t when, void* context, ::dispatch_function_t work);
		void after(::dispatch_time_t when, function_t work);
		void apply(size_t iterations, void* context, ::dispatch_function_apply_t work);
		void apply(size_t iterations, counted_function_t work);
		void async(void* context, ::dispatch_function_t work);
		void async(function_t work);
		void sync(void* context, ::dispatch_function_t work);
		void sync(function_t work);
	protected:
		queue(::dispatch_queue_t q);
	};

	struct DISPATCHPP_EXPORT group : object
	{
		group();
		void async(queue& queue, void* context, ::dispatch_function_t work);
		void async(queue& queue, function_t work);
		void enter();
		void leave();
		long wait(::dispatch_time_t when);
		void notify(queue& queue, void* context, ::dispatch_function_t work);
		void notify(queue& queue, function_t work);
	};

	struct DISPATCHPP_EXPORT source : object
	{
		source(::dispatch_source_type_t type, uintptr_t handle, unsigned long mask, queue& queue);
		long test_cancel();
		void cancel();
		unsigned long get_data();
		uintptr_t get_handle();
		uintptr_t get_mask();
		void merge_data(unsigned long value);
		void set_timer(::dispatch_time_t start, uint64_t interval, uint64_t leeway);
		void set_event_handler(function_t handler);
		void set_event_handler(::dispatch_function_t handler);
		void set_cancel_handler(function_t handler);
		void set_cancel_handler(::dispatch_function_t handler);
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
