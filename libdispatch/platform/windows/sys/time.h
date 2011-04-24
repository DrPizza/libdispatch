#ifndef SYS_TIME__H
#define SYS_TIME__H

struct timeval;

int gettimeofday(struct timeval* tp, void* tzp);

#endif
