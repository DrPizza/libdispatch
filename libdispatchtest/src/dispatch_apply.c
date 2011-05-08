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

volatile int32_t count = 0;

void incrementer(void* context, size_t i) {
	dispatch_atomic_inc(&count);
}

int
main(void)
{
	const int32_t final = 32;
	dispatch_queue_t queue;

	test_start("Dispatch Apply");

	queue = dispatch_get_global_queue(0, 0);
	test_ptr_notnull("dispatch_get_concurrent_queue", queue);

	dispatch_apply_f(final, queue, NULL, &incrementer);

	test_long("count", count, final);
	test_stop();

	return 0;
}
