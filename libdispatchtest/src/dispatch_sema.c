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

#include <dispatch/dispatch.h>

#include <assert.h>

#define LAPS 10000

static size_t total;
dispatch_semaphore_t dsema;

void semaphore_callback(void* context, size_t unused)
{
	dispatch_semaphore_wait(dsema, DISPATCH_TIME_FOREVER);
	dispatch_atomic_inc(&total);
	dispatch_semaphore_signal(dsema);
}

int
main(void)
{
	test_start("Dispatch Semaphore");

	dsema = dispatch_semaphore_create(1);
	assert(dsema);

	dispatch_apply_f(LAPS, dispatch_get_global_queue(0, 0), NULL, &semaphore_callback);

	dispatch_release(as_do(dsema));

	test_long("count", total, LAPS);
	test_stop();

	return 0;
}
