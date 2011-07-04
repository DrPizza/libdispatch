#ifndef PLATFORM_WINDOWS_SYS_TIME__H
#define PLATFORM_WINDOWS_SYS_TIME__H

#ifdef __cplusplus
extern "C" {
#endif

struct timeval;

int gettimeofday(struct timeval* tp, void* tzp);

#ifdef __cplusplus
}
#endif

#endif
