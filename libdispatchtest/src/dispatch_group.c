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

#include <assert.h>

#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000
#endif

static void
group_callback(void* context)
{
	int delay = (int)context;
	if (delay) {
		fprintf(stderr, "sleeping...\n");
		Sleep(delay * 1000);
		fprintf(stderr, "done.\n");
	}
}

static dispatch_group_t
create_group(size_t count, int delay)
{
	size_t i;

	dispatch_group_t group = dispatch_group_create();

	for (i = 0; i < count; ++i) {
		dispatch_queue_t queue = dispatch_queue_create(NULL, NULL);
		assert(queue);

		dispatch_group_async_f(group, queue, (void*)delay, &group_callback);

		dispatch_release(as_do(queue));
	}
	return group;
}

static void
noop(void* unused)
{
}

static void
group_notification(void* unused)
{
	dispatch_queue_t m = dispatch_get_main_queue();
	dispatch_queue_t c = dispatch_get_current_queue();
	test_ptr("Notification Received", m, c);
	test_stop();
}

int
main(void)
{
	long res;
	dispatch_group_t group;

	test_start("Dispatch Group");

	group = create_group(100, 0);
	test_ptr_notnull("dispatch_group_async", group);

	dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
	
	// should be OK to re-use a group
	dispatch_group_async_f(group, dispatch_get_global_queue(0, 0), NULL, &noop);
	dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

	dispatch_release(as_do(group));
	group = NULL;
	
	group = create_group(3, 7);
	test_ptr_notnull("dispatch_group_async", group);

	res = dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, 5ull * NSEC_PER_SEC));
	test_long("dispatch_group_wait", !res, 0);

	// retry after timeout (this time succeed)
	res = dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, 5ull * NSEC_PER_SEC));
	test_long("dispatch_group_wait", res, 0);

	dispatch_release(as_do(group));
	group = NULL;

	group = create_group(100, 0);
	test_ptr_notnull("dispatch_group_async", group);

	dispatch_group_notify_f(group, dispatch_get_main_queue(), NULL, &group_notification);
	
	dispatch_release(as_do(group));
	group = NULL;

	dispatch_main();

	return 0;
}
