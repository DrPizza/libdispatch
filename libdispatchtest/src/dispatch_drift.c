/*
 * Copyright (c) 2008-2009 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

#include "dispatch_test.h"

#include <stdlib.h>
#include <time.h>

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

#if !defined(HAS_TIMESPEC) && defined(_CRT_NO_TIME_T)
struct timespec
{
	time_t tv_sec;
	__int32 tv_nsec;
};
#endif

//
// Verify that dispatch timers do not drift, that is to say, increasingly
// stray from their intended interval.
//
// The jitter of the event handler is defined to be the amount of time
// difference between the actual and expected timing of the event handler
// invocation. The drift is defined to be the amount that the jitter changes
// over time. 
//
// Important: be sure to use the same clock when comparing actual and expected
// values. Skew between different clocks is to be expected.
//

uintptr_t count = 0;
double last_jitter = 0;
double first = 0;
double jittersum = 0;
double driftsum = 0;
double interval_d;
unsigned int target;
dispatch_source_t timer;

void timer_handler(void* context)
{
	struct timeval now_tv;
	double now;
	double goal;
	double jitter;
	double drift;

	UNREFERENCED_PARAMETER(context);

	gettimeofday(&now_tv, NULL);
	now = now_tv.tv_sec + ((double)now_tv.tv_usec / (double)USEC_PER_SEC);

	if (first == 0) {
		first = now;
	}

	goal = first + interval_d * count;
	jitter = goal - now;
	drift = jitter - last_jitter;

	count += dispatch_source_get_data(timer);
	jittersum += jitter;
	driftsum += drift;

	printf("%4d: jitter %f, drift %f\n", count, jitter, drift);
		
	if (count >= target) {
		test_double_less_than("average jitter", fabs(jittersum) / (double)count, 0.0001);
		test_double_less_than("average drift", fabs(driftsum) / (double)count, 0.0001);
		test_stop();
	}
	last_jitter = jitter;
}

int
main()
{
	// interval is 1/10th of a second
	uint64_t interval = NSEC_PER_SEC / 10;
	// for 25 seconds
	struct timeval now_tv;
	struct timespec now_ts;

	interval_d = (double)interval / (double)NSEC_PER_SEC;
	target = (unsigned int)(25 / interval_d);

	test_start("Timer drift test");

	timeBeginPeriod(1);

	timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
	test_ptr_notnull("DISPATCH_SOURCE_TYPE_TIMER", timer);
	
	dispatch_source_set_event_handler_f(timer, timer_handler);
	
	gettimeofday(&now_tv, NULL);
	now_ts.tv_sec = now_tv.tv_sec;
	now_ts.tv_nsec = now_tv.tv_usec * NSEC_PER_USEC;

	dispatch_source_set_timer(timer, dispatch_walltime(&now_ts, interval), interval, 0);

	dispatch_resume(as_do(timer));

	dispatch_main();
}
