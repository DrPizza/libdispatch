#ifndef PTHREAD__H
#define PTHREAD__H

#ifdef __cplusplus
extern "C"
{
#endif

struct _pthread;
typedef struct _pthread pthread;
typedef pthread* pthread_t;
struct _pthread_attr;
typedef struct _pthread_attr pthread_attr;
typedef pthread_attr* pthread_attr_t;

void pthread_exit(void *value_ptr);

typedef long pthread_key_t;

int pthread_key_create(pthread_key_t* key, void (*destr_function)(void*));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void* pointer);
void* pthread_getspecific(pthread_key_t key);

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int pthread_detach(pthread_t thread);
int pthread_join(pthread_t thread, void** thread_return);
void pthread_exit(void *status);
pthread_t pthread_self();

#ifdef __cplusplus
}
#endif

#endif
