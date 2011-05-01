#include "internal.h"

#if DISPATCH_COCOA_COMPAT
void (*dispatch_begin_thread_4GC)(void) = dummy_function;
void (*dispatch_end_thread_4GC)(void) = dummy_function;
void *(*_dispatch_begin_NSAutoReleasePool)(void) = (void *)dummy_function;
void (*_dispatch_end_NSAutoReleasePool)(void *) = (void *)dummy_function;

static dispatch_once_t _dispatch_main_q_port_pred;
static mach_port_t main_q_port;
#endif

#if TARGET_OS_WIN32
static dispatch_once_t _dispatch_window_message_pred;
static UINT _dispatch_thread_window_message;
#endif

// 6618342 Contact the team that owns the Instrument DTrace probe before renaming this symbol
DISPATCH_NOINLINE
static void
_dispatch_queue_set_manual_drain_state(dispatch_queue_t q, bool arg)
{
	q->dq_is_manually_draining = arg;
}

DISPATCH_NOINLINE
static void
_dispatch_manual_queue_drain(dispatch_queue_t q)
{
	if (q->dq_is_manually_draining) {
		return;
	}
	_dispatch_queue_set_manual_drain_state(q, true);
	_dispatch_queue_serial_drain_till_empty(q);
	_dispatch_queue_set_manual_drain_state(q, false);
}

#if DISPATCH_COCOA_COMPAT
static void
_dispatch_main_q_port_init(void *ctxt DISPATCH_UNUSED)
{
	kern_return_t kr;

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &main_q_port);
	DISPATCH_VERIFY_MIG(kr);
	(void)dispatch_assume_zero(kr);
	kr = mach_port_insert_right(mach_task_self(), main_q_port, main_q_port, MACH_MSG_TYPE_MAKE_SEND);
	DISPATCH_VERIFY_MIG(kr);
	(void)dispatch_assume_zero(kr);

	_dispatch_program_is_probably_callback_driven = true;
	_dispatch_safe_fork = false;
}

void
_dispatch_main_q_port_clean(void)
{
	dispatch_once_f(&_dispatch_main_q_port_pred, NULL, _dispatch_main_q_port_init);

	mach_port_t mp = main_q_port;
	kern_return_t kr;

	main_q_port = 0;

	if (mp) {
		kr = mach_port_deallocate(mach_task_self(), mp);
		DISPATCH_VERIFY_MIG(kr);
		(void)dispatch_assume_zero(kr);
		kr = mach_port_mod_refs(mach_task_self(), mp, MACH_PORT_RIGHT_RECEIVE, -1);
		DISPATCH_VERIFY_MIG(kr);
		(void)dispatch_assume_zero(kr);
	}
}

void
_dispatch_main_queue_callback_4CF(mach_msg_header_t *msg DISPATCH_UNUSED)
{
	_dispatch_manual_queue_drain(&_dispatch_main_q);
}

mach_port_t
_dispatch_get_main_queue_port_4CF(void)
{
	dispatch_once_f(&_dispatch_main_q_port_pred, NULL, _dispatch_main_q_port_init);
	return main_q_port;
}

#endif

#if TARGET_OS_WIN32
static void
_dispatch_window_message_init(void *ctxt DISPATCH_UNUSED)
{
	_dispatch_thread_window_message = RegisterWindowMessageW(L"libdispatch");
}

UINT dispatch_get_thread_window_message(void)
{
	dispatch_once_f(&_dispatch_window_message_pred, NULL, _dispatch_window_message_init);
	return _dispatch_thread_window_message;
}

#endif

DISPATCH_NOINLINE
void
_dispatch_queue_wakeup_main(void)
{
#if DISPATCH_COCOA_COMPAT
	kern_return_t kr;

	dispatch_once_f(&_dispatch_main_q_port_pred, NULL, _dispatch_main_q_port_init);

	kr = _dispatch_send_wakeup_main_thread(main_q_port, 0);

	switch (kr) {
	case MACH_SEND_TIMEOUT:
	case MACH_SEND_TIMED_OUT:
	case MACH_SEND_INVALID_DEST:
		break;
	default:
		(void)dispatch_assume_zero(kr);
		break;
	}

	_dispatch_safe_fork = false;
#endif
	// TODO decide on Win32 main thread semantics
}

DISPATCH_NOINLINE
void
_dispatch_queue_wakeup_thread(void)
{
#if TARGET_OS_WIN32
	PostThreadMessage(GetThreadId(_pthread_get_native_handle(pthread_self())), dispatch_get_thread_window_message(), 0, 0);
#endif
	// TODO decide on Mac OS X per-thread queue semantics. A mach port per thread would work nicely enough, I think.
}

DISPATCH_NOINLINE
void
_dispatch_queue_wakeup_manual(dispatch_queue_t q)
{
	if (q == &_dispatch_main_q) {
		_dispatch_queue_wakeup_main();
	} else {
		_dispatch_queue_wakeup_thread();
	}
}

DISPATCH_NOINLINE
void
_dispatch_queue_cleanup_main(void)
{
#if DISPATCH_COCOA_COMPAT
	_dispatch_main_q_port_clean();
#endif
	// TODO
}

DISPATCH_NOINLINE
void
_dispatch_queue_cleanup_thread(void)
{
}

DISPATCH_NOINLINE
void
_dispatch_queue_cleanup_manual(dispatch_queue_t q)
{
	if (q == &_dispatch_main_q) {
		_dispatch_queue_cleanup_main();
	} else {
		_dispatch_queue_cleanup_thread();
	}
}

void
dispatch_thread_queue_callback(void)
{
	_dispatch_manual_queue_drain(dispatch_get_current_thread_queue());
}
