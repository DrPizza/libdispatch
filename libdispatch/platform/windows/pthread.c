#include "pthread.h"

#include <SDKDDKVer.h>
#include <Windows.h>

#include <errno.h>

#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union

DWORD pthread_slot;
long slots_allocated;
typedef void (*slot_destructor)(void*);
slot_destructor* destructors; // slot_destructor[slots_allocated]
CRITICAL_SECTION destructor_lock;

struct _pthread_items
{
	long count;
	pthread_t current_thread;
	void* items[];
};

typedef struct _pthread_items pthread_items;
typedef pthread_items* pthread_items_t;

struct _pthread
{
	HANDLE thread;
	void* result;
};

typedef struct _pthread pthread;

typedef pthread* pthread_t;

void NTAPI pthread_tls_init(void* dll, DWORD reason, void* reserved)
{
	UNREFERENCED_PARAMETER(dll);
	UNREFERENCED_PARAMETER(reserved);
	switch(reason)
	{
	case DLL_PROCESS_ATTACH:
		slots_allocated = 0;
		destructors = NULL;
		pthread_slot = TlsAlloc();
		InitializeCriticalSection(&destructor_lock);
	case DLL_THREAD_ATTACH:
		{
			pthread_items_t items = calloc(1, sizeof(pthread_items) + slots_allocated * sizeof(void*));
			items->count = slots_allocated;
			items->current_thread = calloc(1, sizeof(pthread));
			items->current_thread->thread = INVALID_HANDLE_VALUE;
			TlsSetValue(pthread_slot, items);
		}
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_DETACH:
		{
			pthread_items_t items = TlsGetValue(pthread_slot);
			slot_destructor* local_destructors = calloc(slots_allocated, sizeof(slot_destructor));
			long i;
			int more_calls_needed;

			EnterCriticalSection(&destructor_lock);
			memcpy(local_destructors, destructors, slots_allocated * sizeof(slot_destructor));
			LeaveCriticalSection(&destructor_lock);
			do
			{
				for(i = 0; i < slots_allocated; ++i)
				{
					if(local_destructors[i] && items->items[i])
					{
						void* value = items->items[i];
						items->items[i] = NULL;
						local_destructors[i](value);
					}
				}
				more_calls_needed = 0;
				for(i = 0; i < slots_allocated; ++i)
				{
					if(items->items[i])
					{
						more_calls_needed = 1;
					}
				}
			}
			while(more_calls_needed);
			free(local_destructors);

			if(items->current_thread->thread == INVALID_HANDLE_VALUE)
			{
				free(items->current_thread);
			}
			free(items);
			TlsSetValue(pthread_slot, NULL);

			if(reason == DLL_PROCESS_DETACH)
			{
				free(destructors);
				DeleteCriticalSection(&destructor_lock);
			}
		}
		break;
	}
}

#ifdef _M_IX86
#pragma comment (linker, "/INCLUDE:__tls_used")
#pragma comment (linker, "/INCLUDE:__xl_b")
#else
#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:_xl_b")
#endif

#ifdef _M_X64
#pragma const_seg(".CRT$XLB")
EXTERN_C const
#else
#pragma data_seg(".CRT$XLB")
EXTERN_C
#endif

PIMAGE_TLS_CALLBACK _xl_b = pthread_tls_init;

#ifdef _M_X64
#pragma const_seg()
#else
#pragma data_seg()
#endif

int pthread_key_create(pthread_key_t* key, void (*destr_function)(void*))
{
	if(!key)
	{
		return EFAULT;
	}

	EnterCriticalSection(&destructor_lock);
	{
		long new_key = slots_allocated++;
		destructors = realloc(destructors, sizeof(slot_destructor) * slots_allocated);
		destructors[new_key] = destr_function;
		*key = new_key;
	}
	LeaveCriticalSection(&destructor_lock);

	return 0;
}

int pthread_key_delete(pthread_key_t key)
{
	if(key >= slots_allocated)
	{
		return EINVAL;
	}
	return 0;
}

static pthread_items_t _get_items()
{
	pthread_items_t items = TlsGetValue(pthread_slot);
	if(items->count < slots_allocated)
	{
		long i = 0;
		items = realloc(items, sizeof(pthread_items) + slots_allocated * sizeof(void*));
		for(i = items->count; i < slots_allocated; ++i)
		{
			items->items[i] = NULL;
		}
		items->count = slots_allocated;
		TlsSetValue(pthread_slot, items);
	}
	return items;
}

int pthread_setspecific(pthread_key_t key, const void* pointer)
{
	pthread_items_t items = _get_items();
	items->items[key] = (void*)pointer;
	return 0;
}

void* pthread_getspecific(pthread_key_t key)
{
	pthread_items_t items = _get_items();
	return items->items[key];
}

struct _fn
{
	void* (*fun)(void*);
	void* context;
	pthread* thread;
};

typedef struct _fn fn;

static DWORD WINAPI thread_proc(void* parameter)
{
	fn* f = parameter;

	pthread_self()->thread = f->thread->thread;
	pthread_self()->result = f->fun(f->context);

	free(f);
	return 0;
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg)
{
	fn* f;

	if(!thread || !start_routine)
	{
		return EFAULT;
	}
	if(attr)
	{
		return EINVAL;
	}

	*thread = calloc(1, sizeof(pthread));
	if(!*thread)
	{
		return ENOMEM;
	}
	f = calloc(1, sizeof(fn));
	if(!f)
	{
		return ENOMEM;
	}

	f->fun = start_routine;
	f->context = arg;
	f->thread = *thread;
	f->thread->thread = CreateThread(NULL, 0, thread_proc, f, 0, NULL);
	return 0;
}

int pthread_detach(pthread_t thread)
{
	CloseHandle(thread->thread);
	thread->thread = INVALID_HANDLE_VALUE;
	return 0;
}

int pthread_join(pthread_t thread, void** thread_return)
{
	WaitForSingleObject(thread->thread, INFINITE);
	if(thread_return)
	{
		thread_return = thread->result;
	}
	CloseHandle(thread->thread);
	free(thread);
	return 0;
}

void pthread_exit(void* status)
{
	pthread_self()->result = status;
	ExitThread(0);
}

pthread_t pthread_self()
{
	pthread_items_t items = _get_items();
	return items->current_thread;
}
