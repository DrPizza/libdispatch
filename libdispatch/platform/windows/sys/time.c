#include "time.h"

#define WIN32_LEAN_AND_MEAN // prevent winsock.h from being included--it has duplicate definitions of struct timeval
#include <SDKDDKVer.h>
#include <Windows.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

static const unsigned __int64 intervals_per_second      = 10000000ULL;
static const unsigned __int64 microseconds_per_second   = 1000000ULL;
static const unsigned __int64 intervals_per_microsecond = 10ULL;
static const unsigned __int64 intervals_since_epoch     = 116444736000000000ULL;
static const unsigned __int64 microseconds_since_epoch  = 11644473600000000ULL;

int gettimeofday(struct timeval* tp, void* tzp)
{
	FILETIME file_time;
	ULARGE_INTEGER ularge;

	UNREFERENCED_PARAMETER(tzp);

	GetSystemTimeAsFileTime(&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
	ularge.QuadPart -= intervals_since_epoch;
	ularge.QuadPart /= intervals_per_microsecond;
	tp->tv_sec = (long) (ularge.QuadPart / microseconds_per_second);
	tp->tv_usec = (long) (ularge.QuadPart % microseconds_per_second);

	return 0;
}
