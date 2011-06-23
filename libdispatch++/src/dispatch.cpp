#include "dispatch.hpp"

#include <cstdarg>
#include <memory>

#include "scopeguard.hpp"

namespace gcd
{
	namespace
	{
		void DISPATCH_INLINE debug_break()
		{
#if defined(_MSC_VER)
			__debugbreak();
#elif defined(__GNUC__)
			asm("int3");
#else
			*static_cast<size_t*>(nullptr) = 0;
#endif
		}

		template<typename T>
		struct counted_pointer
		{
			counted_pointer(T* obj_, ptrdiff_t count_) : obj(obj_), count(count_)
			{
			}

			void release()
			{
				if(0 == dispatch_atomic_dec(&count))
				{
					delete this;
				}
			}

			T& operator* () const
			{
				return *obj;
			}

			T* operator-> () const
			{
				return obj;
			}

			T* obj;
			ptrdiff_t count;

		protected:
			~counted_pointer()
			{
				delete obj;
			}
		};

		void dispatch_function_object(void* context)
		{
			try
			{
				std::unique_ptr<function_t> fun(static_cast<function_t*>(context));
				(*fun)();
			}
			catch(std::exception&)
			{
				debug_break();
			}
		}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4189)
#endif
		void dispatch_counted_function_object(void* context, size_t count)
		{
			try
			{
				counted_pointer<counted_function_t>* fun(static_cast<counted_pointer<counted_function_t>*>(context));
				ON_BLOCK_EXIT([&] { fun->release(); });
				(**fun)(count);
			}
			catch(std::exception&)
			{
				debug_break();
			}
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		struct dispatch_context
		{
			function_t* finalizer;
			::dispatch_function_t raw_finalizer;
			function_t* event_handler;
			::dispatch_function_t raw_event_handler;
			function_t* cancel_handler;
			::dispatch_function_t raw_cancel_handler;
			void* context;
		};

		void dispatch_finalizer(void* raw_context)
		{
			try
			{
				std::unique_ptr<dispatch_context> cooked_context(static_cast<dispatch_context*>(raw_context));
				std::unique_ptr<function_t> finalizer(cooked_context->finalizer);
				std::unique_ptr<function_t> event_handler(cooked_context->event_handler);
				std::unique_ptr<function_t> cancel_handler(cooked_context->cancel_handler);
				if(cooked_context->context)
				{
					if(cooked_context->finalizer)
					{
						(*finalizer)();
					}
					else if(cooked_context->raw_finalizer)
					{
						cooked_context->raw_finalizer(cooked_context->context);
					}
				}
			}
			catch(std::exception&)
			{
				debug_break();
			}
		}

		void dispatch_event_handler(void* raw_context)
		{
			try
			{
				dispatch_context* cooked_context(static_cast<dispatch_context*>(raw_context));
				if(cooked_context->event_handler)
				{
					(*cooked_context->event_handler)();
				}
				else if(cooked_context->raw_event_handler)
				{
					cooked_context->raw_event_handler(cooked_context->context);
				}
			}
			catch(std::exception&)
			{
				debug_break();
			}
		}

		void dispatch_cancel_handler(void* raw_context)
		{
			try
			{
				dispatch_context* cooked_context(static_cast<dispatch_context*>(raw_context));
				if(cooked_context->cancel_handler)
				{
					(*cooked_context->cancel_handler)();
				}
				else if(cooked_context->raw_cancel_handler)
				{
					cooked_context->raw_cancel_handler(cooked_context->context);
				}
			}
			catch(std::exception&)
			{
				debug_break();
			}
		}
	}

	object::object(::dispatch_object_t o_) : o(o_)
	{
	}

	object::object(const object& rhs)
	{
		if(this == &rhs) { return; }
		::dispatch_retain(rhs.o);
		::dispatch_release(o);
		o = rhs.o;
	}

	void object::debug(const char* message, ...) const
	{
		va_list args;
		va_start(args, message);
		dispatch_debugv(o, message, args);
		va_end(args);
	}

	void object::retain() const
	{
		::dispatch_retain(o);
	}

	void object::release() const
	{
		::dispatch_release(o);
	}

	void object::suspend() const
	{
		::dispatch_suspend(o);
	}

	void object::resume() const
	{
		::dispatch_resume(o);
	}

	void object::set_target_queue(queue& target) const
	{
		::dispatch_set_target_queue(o, static_cast<dispatch_queue_t>(target.o));
	}

	void* object::ensure_context() const
	{
		void* raw_context(::dispatch_get_context(o));
		std::unique_ptr<dispatch_context> cooked_context(raw_context == nullptr ? new dispatch_context() : static_cast<dispatch_context*>(raw_context));
		::dispatch_set_context(o, cooked_context.release());
		::dispatch_set_finalizer_f(o, &dispatch_finalizer);
		return ::dispatch_get_context(o);
	}

	void object::set_context(void* context) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		cooked_context->context = context;
	}

	void* object::get_context() const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		return cooked_context->context;
	}

	void object::set_finalizer(function_t finalizer) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_finalizer(cooked_context->finalizer);
		std::unique_ptr<function_t> clone(new function_t(finalizer));
		cooked_context->finalizer = clone.release();
		cooked_context->raw_finalizer = nullptr;
	}

	void object::set_finalizer(::dispatch_function_t finalizer) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_finalizer(cooked_context->finalizer);
		cooked_context->finalizer = nullptr;
		cooked_context->raw_finalizer = finalizer;
	}

	object::~object()
	{
		::dispatch_release(o);
	}

	queue::queue(const char* label, ::dispatch_queue_attr_t attributes) : object(::dispatch_queue_create(label, attributes))
	{
	}

	queue::queue(::dispatch_queue_t q) : object(q)
	{
	}

	queue queue::get_main_queue()
	{
		return queue(::dispatch_get_main_queue());
	}

	queue queue::get_current_thread_queue()
	{
		return queue(dispatch_get_current_thread_queue());
	}

	queue queue::get_current_queue()
	{
		return queue(::dispatch_get_current_queue());
	}

	queue queue::get_global_queue(long priority, unsigned long flags)
	{
		return queue(::dispatch_get_global_queue(priority, flags));
	}

	const char* queue::get_label() const
	{
		return ::dispatch_queue_get_label(static_cast<dispatch_queue_t>(o));
	}

	void queue::after(::dispatch_time_t when, void* context, ::dispatch_function_t work) const
	{
		return ::dispatch_after_f(when, static_cast<dispatch_queue_t>(o), context, work);
	}

	void queue::after(::dispatch_time_t when, function_t work) const
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		after(when, clone.release(), &dispatch_function_object);
	}

	void queue::apply(size_t iterations, void* context, ::dispatch_function_apply_t work) const
	{
		return ::dispatch_apply_f(iterations, static_cast<dispatch_queue_t>(o), context, work);
	}

	void queue::apply(size_t iterations, counted_function_t work) const
	{
		apply(iterations, new counted_pointer<counted_function_t>(new counted_function_t(work), static_cast<ptrdiff_t>(iterations)), &dispatch_counted_function_object);
	}

	void queue::async(void* context, ::dispatch_function_t work) const
	{
		return ::dispatch_async_f(static_cast<dispatch_queue_t>(o), context, work);
	}

	void queue::async(function_t work) const
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		async(clone.release(), &dispatch_function_object);
	}

	void queue::sync(void* context, ::dispatch_function_t work) const
	{
		return ::dispatch_sync_f(static_cast<dispatch_queue_t>(o), context, work);
	}

	void queue::sync(function_t work) const
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		sync(clone.release(), &dispatch_function_object);
	}

	group::group() : object(::dispatch_group_create())
	{
	}

	void group::async(queue& queue, void* context, ::dispatch_function_t work) const
	{
		return ::dispatch_group_async_f(static_cast<dispatch_group_t>(o), static_cast<dispatch_queue_t>(queue.o), context, work);
	}

	void group::async(queue& queue, function_t work) const
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		return async(queue, clone.release(), &dispatch_function_object);
	}

	void group::enter() const
	{
		return ::dispatch_group_enter(static_cast<dispatch_group_t>(o));
	}

	void group::leave() const
	{
		return ::dispatch_group_leave(static_cast<dispatch_group_t>(o));
	}

	long group::wait(::dispatch_time_t when) const
	{
		return ::dispatch_group_wait(static_cast<dispatch_group_t>(o), when);
	}

	void group::notify(queue& queue, void* context, ::dispatch_function_t work) const
	{
		return ::dispatch_group_notify_f(static_cast<dispatch_group_t>(o), static_cast<dispatch_queue_t>(queue.o), context, work);
	}

	void group::notify(queue& queue, function_t work) const
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		return notify(queue, clone.release(), &dispatch_function_object);
	}

	source::source(::dispatch_source_type_t type, uintptr_t handle, uintptr_t mask, queue& queue) : object(dispatch_source_create(type, handle, mask, static_cast<dispatch_queue_t>(queue.o)))
	{
	}

	long source::test_cancel() const
	{
		return dispatch_source_testcancel(static_cast<dispatch_source_t>(o));
	}

	void source::cancel() const
	{
		dispatch_source_cancel(static_cast<dispatch_source_t>(o));
	}

	uintptr_t source::get_data() const
	{
		return dispatch_source_get_data(static_cast<dispatch_source_t>(o));
	}

	uintptr_t source::get_handle() const
	{
		return dispatch_source_get_handle(static_cast<dispatch_source_t>(o));
	}

	uintptr_t source::get_mask() const
	{
		return dispatch_source_get_mask(static_cast<dispatch_source_t>(o));
	}

	void source::merge_data(uintptr_t value) const
	{
		dispatch_source_merge_data(static_cast<dispatch_source_t>(o), value);
	}

	void source::set_timer(::dispatch_time_t start, uint64_t interval, uint64_t leeway) const
	{
		dispatch_source_set_timer(static_cast<dispatch_source_t>(o), start, interval, leeway);
	}

	void source::set_event_handler(function_t handler) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_handler(cooked_context->event_handler);
		std::unique_ptr<function_t> clone(new function_t(handler));
		cooked_context->event_handler = clone.release();
		cooked_context->raw_event_handler = nullptr;
		::dispatch_source_set_event_handler_f(static_cast<dispatch_source_t>(o), &dispatch_event_handler);
	}

	void source::set_event_handler(::dispatch_function_t handler) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_handler(cooked_context->event_handler);
		cooked_context->event_handler = nullptr;
		cooked_context->raw_event_handler = handler;
		::dispatch_source_set_event_handler_f(static_cast<dispatch_source_t>(o), &dispatch_event_handler);
	}

	void source::set_cancel_handler(function_t handler) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_handler(cooked_context->cancel_handler);
		std::unique_ptr<function_t> clone(new function_t(handler));
		cooked_context->cancel_handler = clone.release();
		cooked_context->raw_cancel_handler = nullptr;
		::dispatch_source_set_cancel_handler_f(static_cast<dispatch_source_t>(o), &dispatch_cancel_handler);
	}

	void source::set_cancel_handler(::dispatch_function_t handler) const
	{
		dispatch_context* cooked_context(static_cast<dispatch_context*>(ensure_context()));
		std::unique_ptr<function_t> old_handler(cooked_context->cancel_handler);
		cooked_context->cancel_handler = nullptr;
		cooked_context->raw_cancel_handler = handler;
		::dispatch_source_set_cancel_handler_f(static_cast<dispatch_source_t>(o), &dispatch_cancel_handler);
	}

	semaphore::semaphore(long value) : object(::dispatch_semaphore_create(value))
	{
	}

	void semaphore::signal()
	{
		::dispatch_semaphore_signal(static_cast<dispatch_semaphore_t>(o));
	}

	long semaphore::wait(dispatch_time_t timeout)
	{
		return ::dispatch_semaphore_wait(static_cast<dispatch_semaphore_t>(o), timeout);
	}

	void dispatch_once(::dispatch_once_t& predicate, function_t work)
	{
		std::unique_ptr<function_t> fun(new function_t(work));
		::dispatch_once_f(&predicate, fun.release(), &dispatch_function_object);
	}
}
