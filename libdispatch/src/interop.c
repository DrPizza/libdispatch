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
static UINT _dispatch_main_window_message;
static UINT _dispatch_thread_window_message;
#endif

static bool _dispatch_program_is_probably_callback_driven;

// 6618342 Contact the team that owns the Instrument DTrace probe before renaming this symbol
struct dispatch_queue_s _dispatch_main_q = {
	/*.do_vtable               = */	&_dispatch_queue_vtable,
	/*.do_next                 = */	0,
	/*.do_ref_cnt              = */	DISPATCH_OBJECT_GLOBAL_REFCNT,
	/*.do_xref_cnt             = */	DISPATCH_OBJECT_GLOBAL_REFCNT,
	/*.do_suspend_cnt          = */	DISPATCH_OBJECT_SUSPEND_LOCK,
	/*.do_targetq              = */	&_dispatch_root_queues[DISPATCH_ROOT_QUEUE_COUNT / 2],
	/*.do_ctxt                 = */	0,
	/*.do_finalizer            = */	0,
	/*.dq_running              = */	1,
	/*.dq_width                = */	1,
	/*.dq_items_tail           = */	0,
	/*.dq_items_head           = */	0,
	/*.dq_serialnum            = */	1,
	/*.dq_finalizer_ctxt       = */	0,
	/*.dq_manually_drained     = */	0,
	/*.dq_is_manually_draining = */	false,
	/*.dq_label                = */	"com.apple.main-thread",
};

#undef dispatch_get_main_queue
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_NA)
dispatch_queue_t dispatch_get_main_queue(void);

dispatch_queue_t
dispatch_get_main_queue(void)
{
	return &_dispatch_main_q;
}

#if !TARGET_OS_WIN32
#define dispatch_get_main_queue() (&_dispatch_main_q)
#endif

/*
 * XXXRW: Work-around for possible clang bug in which __builtin_trap() is not
 * marked noreturn, leading to a build error as dispatch_main() *is* marked
 * noreturn.  Mask by marking __builtin_trap() as noreturn locally.
 */
#ifndef HAVE_NORETURN_BUILTIN_TRAP
void __builtin_trap(void) __attribute__((__noreturn__));
#endif

void
dispatch_main(void)
{

#if HAVE_PTHREAD_MAIN_NP
	if (pthread_main_np()) {
#endif
		_dispatch_program_is_probably_callback_driven = true;
		pthread_exit(NULL);
		DISPATCH_CRASH("pthread_exit() returned");
#if HAVE_PTHREAD_MAIN_NP
	}
	DISPATCH_CLIENT_CRASH("dispatch_main() must be called on the main thread");
#endif
}

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
	_dispatch_thread_window_message = RegisterWindowMessageW(L"libdispatch-threadq");
	_dispatch_main_window_message   = RegisterWindowMessageW(L"libdispatch-mainq");
}

UINT dispatch_get_thread_window_message(void)
{
	dispatch_once_f(&_dispatch_window_message_pred, NULL, _dispatch_window_message_init);
	return _dispatch_thread_window_message;
}

UINT dispatch_get_main_window_message(void)
{
	dispatch_once_f(&_dispatch_window_message_pred, NULL, _dispatch_window_message_init);
	return _dispatch_main_window_message;
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
#else if TARGET_OS_WIN32
	PostThreadMessage(GetThreadId(_pthread_get_native_handle(_dispatch_main_q.dq_manually_drained)), dispatch_get_main_window_message(), 0, 0);
#endif
}

DISPATCH_NOINLINE
void
_dispatch_queue_wakeup_thread(dispatch_queue_t q)
{
#if TARGET_OS_WIN32
	PostThreadMessage(GetThreadId(_pthread_get_native_handle(q->dq_manually_drained)), dispatch_get_thread_window_message(), 0, 0);
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
		_dispatch_queue_wakeup_thread(q);
	}
}

static void
_dispatch_sigsuspend(void *ctxt DISPATCH_UNUSED)
{
	// not sure if this is really the right thing to do.
	// after all, this workitem is placed in an aync queue,
	// so all we're achieving is tying up a thread.
#if TARGET_OS_WIN32
	for (;;) {
		Sleep(INFINITE);
	}
#else
	static const sigset_t mask;

	for (;;) {
		sigsuspend(&mask);
	}
#endif
}

DISPATCH_NOINLINE
void
_dispatch_queue_cleanup_main(void)
{
	// overload the "probably" variable to mean that dispatch_main() or
	// similar non-POSIX API was called
	// this has to run before the DISPATCH_COCOA_COMPAT below
	if (_dispatch_program_is_probably_callback_driven) {
		dispatch_async_f(dispatch_get_global_queue(0, 0), NULL, _dispatch_sigsuspend);
		sleep(1);	// workaround 6778970
	}
	
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
	dispatch_queue_t q = dispatch_get_current_thread_queue();
	_dispatch_manual_queue_drain(q);
	dispatch_release(as_do(q));
}

void
dispatch_main_queue_callback(void)
{
	_dispatch_manual_queue_drain(dispatch_get_main_queue());
}
