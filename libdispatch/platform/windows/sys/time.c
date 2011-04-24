#include "time.h"

#define WIN32_LEAN_AND_MEAN // prevent winsock.h from being included--it has duplicate definitions of struct timeval
#include <SDKDDKVer.h>
#include <Windows.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

static const unsigned __int64 epoch = 116444736000000000ULL;

int gettimeofday(struct timeval* tp, void* tzp)
{
	FILETIME file_time;
	SYSTEMTIME system_time;
	ULARGE_INTEGER ularge;

	UNREFERENCED_PARAMETER(tzp);

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

	return 0;
}
