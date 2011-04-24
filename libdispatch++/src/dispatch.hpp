#ifndef DISPATCH__HPP
#define DISPATCH__HPP

#include "dispatch/dispatch.h"

#include <functional>

namespace gcd
{
	typedef std::function<void (void)> function_t;
	typedef std::function<void (size_t)> counted_function_t;

	struct dispatch_queue;
	struct dispatch_group;

	struct dispatch_object
	{
		dispatch_object(const dispatch_object& rhs);
		void retain();
		void release();
		void suspend();
		void resume();
		void set_target_queue(dispatch_queue& target);
		virtual ~dispatch_object();
		friend struct dispatch_queue;
		friend struct dispatch_group;

	protected:
		dispatch_object(::dispatch_object_t o_);
		::dispatch_object_t o;
	};

	struct dispatch_queue : dispatch_object
	{
		dispatch_queue(const char* label, ::dispatch_queue_attr_t attributes);
		static dispatch_queue get_current_queue();
		static dispatch_queue get_global_queue(long priority, unsigned long flags);
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
		dispatch_queue(::dispatch_queue_t q);
	};

	struct dispatch_group : dispatch_object
	{
		dispatch_group();
		void async(dispatch_queue& queue, void* context, ::dispatch_function_t work);
		void async(dispatch_queue& queue, function_t work);
		void enter();
		void leave();
		long wait(::dispatch_time_t when);
		void notify(dispatch_queue& queue, void* context, ::dispatch_function_t work);
		void notify(dispatch_queue& queue, function_t work);
	};

	void dispatch_once(::dispatch_once_t& predicate, function_t work);
}

#endif
