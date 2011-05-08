#ifndef SCOPEGUARD_HPP
#define SCOPEGUARD_HPP

#include <utility>

namespace util
{
	struct scope_guard_base
	{
		void dismiss() const
		{
			active = false;
		}
	protected:
		explicit scope_guard_base(bool active_ = true) : active(active_)
		{
		}

		~scope_guard_base()
		{
		}

		mutable bool active;
	private:
		scope_guard_base(const scope_guard_base&);
		scope_guard_base& operator=(const scope_guard_base&);
	};

	template<typename F>
	struct scope_guard_with_functor : scope_guard_base
	{
		explicit scope_guard_with_functor(F f) : func(std::move(f))
		{
		}

		// in an ideal world, this would be a move constructor, dismiss would be non-const, and active would be non-mutable
		// sadly, http://connect.microsoft.com/VisualStudio/feedback/details/571550/the-copy-constructor-incorrectly-chosen-for-an-rvalue and
		// https://connect.microsoft.com/VisualStudio/feedback/details/553184/non-copyable-rvalues-cannot-be-bound-to-rvalue-references seem
		// to cause problems. Even though one is "fixed" and one is "won't fix"
		scope_guard_with_functor(const scope_guard_with_functor& rhs) : scope_guard_base(rhs.active), func(std::move(rhs.func))
		{
			rhs.dismiss();
		}

		~scope_guard_with_functor()
		{
			if(active)
			{
				func();
			}
		}

	private:
		F func;
		scope_guard_with_functor& operator=(const scope_guard_with_functor<F>&);
	};

	template<typename F>
	scope_guard_with_functor<F> make_guard(F f)
	{
		return scope_guard_with_functor<F>(std::move(f));
	}

	typedef scope_guard_base&& scope_guard;
}

#define CONCATENATE_DIRECT(s1, s2)  s1##s2
#define CONCATENATE(s1, s2)         CONCATENATE_DIRECT(s1, s2)
#define ANONYMOUS_VARIABLE(str)     CONCATENATE(str, __LINE__)

#define ON_BLOCK_EXIT(lambda) util::scope_guard ANONYMOUS_VARIABLE(scope_guard) = util::make_guard(lambda)

#endif
