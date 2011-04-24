#include "dispatch.hpp"

#include <SDKDDKVer.h>
#include <Windows.h>

#include <memory>
#include <loki/ScopeGuard.h>

namespace gcd
{
	namespace
	{
		template<typename T>
		struct counted_pointer
		{
			counted_pointer(T* obj_, LONG count_) : obj(obj_), count(count_)
			{
			}

			void release()
			{
				if(0 == ::InterlockedDecrement(&count))
				{
					delete this;
				}
			}

		protected:
			~counted_pointer()
			{
				delete obj;
			}

		public:
			T& operator* () const
			{
				return *obj;
			}

			T* operator-> () const
			{
				return obj;
			}

			T* obj;
			LONG count;
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
				::DebugBreak();
			}
		}

		void dispatch_counted_function_object(void* context, size_t count)
		{
			try
			{
				using namespace Loki;
				counted_pointer<counted_function_t>* fun(static_cast<counted_pointer<counted_function_t>*>(context));
				LOKI_ON_BLOCK_EXIT_OBJ(*fun, &counted_pointer<counted_function_t>::release);
				(**fun)(count);
			}
			catch(std::exception&)
			{
				::DebugBreak();
			}
		}
	}

	dispatch_object::dispatch_object(::dispatch_object_t o_) : o(o_)
	{
	}

	dispatch_object::dispatch_object(const dispatch_object& rhs)
	{
		if(this == &rhs) { return; }

		o = rhs.o;
		::dispatch_retain(o);
	}

	void dispatch_object::retain()
	{
		::dispatch_retain(o);
	}

	void dispatch_object::release()
	{
		::dispatch_release(o);
	}

	void dispatch_object::suspend()
	{
		::dispatch_suspend(o);
	}

	void dispatch_object::resume()
	{
		::dispatch_resume(o);
	}

	void dispatch_object::set_target_queue(dispatch_queue& target)
	{
		if(dynamic_cast<dispatch_queue*>(this) == nullptr)
		{
			return;
		}
		::dispatch_set_target_queue(o, static_cast<dispatch_queue_t>(target.o));
	}

	dispatch_object::~dispatch_object()
	{
		::dispatch_release(o);
	}

	dispatch_queue::dispatch_queue(const char* label, ::dispatch_queue_attr_t attributes) : dispatch_object(::dispatch_queue_create(label, attributes))
	{
	}

	dispatch_queue::dispatch_queue(::dispatch_queue_t q) : dispatch_object(q)
	{
	}

	dispatch_queue dispatch_queue::get_current_queue()
	{
		return dispatch_queue(::dispatch_get_current_queue());
	}

	dispatch_queue dispatch_queue::get_global_queue(long priority, unsigned long flags)
	{
		return dispatch_queue(::dispatch_get_global_queue(priority, flags));
	}

	const char* dispatch_queue::get_label() const
	{
		return ::dispatch_queue_get_label(static_cast<dispatch_queue_t>(o));
	}

	void dispatch_queue::after(::dispatch_time_t when, void* context, ::dispatch_function_t work)
	{
		return ::dispatch_after_f(when, static_cast<dispatch_queue_t>(o), context, work);
	}

	void dispatch_queue::after(::dispatch_time_t when, function_t work)
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		after(when, clone.release(), &dispatch_function_object);
	}

	void dispatch_queue::apply(size_t iterations, void* context, ::dispatch_function_apply_t work)
	{
		return ::dispatch_apply_f(iterations, static_cast<dispatch_queue_t>(o), context, work);
	}

	void dispatch_queue::apply(size_t iterations, counted_function_t work)
	{
		apply(iterations, new counted_pointer<counted_function_t>(new counted_function_t(work), static_cast<LONG>(iterations)), &dispatch_counted_function_object);
	}

	void dispatch_queue::async(void* context, ::dispatch_function_t work)
	{
		return ::dispatch_async_f(static_cast<dispatch_queue_t>(o), context, work);
	}

	void dispatch_queue::async(function_t work)
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		async(clone.release(), &dispatch_function_object);
	}

	void dispatch_queue::sync(void* context, ::dispatch_function_t work)
	{
		return ::dispatch_sync_f(static_cast<dispatch_queue_t>(o), context, work);
	}

	void dispatch_queue::sync(function_t work)
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		sync(clone.release(), &dispatch_function_object);
	}

	dispatch_group::dispatch_group() : dispatch_object(::dispatch_group_create())
	{
	}

	void dispatch_group::async(dispatch_queue& queue, void* context, ::dispatch_function_t work)
	{
		return ::dispatch_group_async_f(static_cast<dispatch_group_t>(o), static_cast<dispatch_queue_t>(queue.o), context, work);
	}

	void dispatch_group::async(dispatch_queue& queue, function_t work)
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		return async(queue, clone.release(), &dispatch_function_object);
	}

	void dispatch_group::enter()
	{
		return ::dispatch_group_enter(static_cast<dispatch_group_t>(o));
	}

	void dispatch_group::leave()
	{
		return ::dispatch_group_leave(static_cast<dispatch_group_t>(o));
	}

	long dispatch_group::wait(::dispatch_time_t when)
	{
		return ::dispatch_group_wait(static_cast<dispatch_group_t>(o), when);
	}

	void dispatch_group::notify(dispatch_queue& queue, void* context, ::dispatch_function_t work)
	{
		return ::dispatch_group_notify_f(static_cast<dispatch_group_t>(o), static_cast<dispatch_queue_t>(queue.o), context, work);
	}

	void dispatch_group::notify(dispatch_queue& queue, function_t work)
	{
		std::unique_ptr<function_t> clone(new function_t(work));
		return notify(queue, clone.release(), &dispatch_function_object);
	}

	void dispatch_once(::dispatch_once_t& predicate, function_t work)
	{
		std::unique_ptr<function_t> fun(new function_t(work));
		::dispatch_once_f(&predicate, fun.release(), &dispatch_function_object);
	}
}
