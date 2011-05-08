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

uint32_t count = 0;
const uint32_t final = 1000000; // 1M

typedef struct {
	dispatch_group_t group;
	dispatch_queue_t ping;
	dispatch_queue_t pong;
	uint32_t counter;
} args_s, * args_t;

static args_t
make_args(dispatch_group_t group, dispatch_queue_t ping, dispatch_queue_t pong, uint32_t counter) {
	args_t args = calloc(1, sizeof(args_s));
	args->group = group;
	args->ping = ping;
	args->pong = pong;
	args->counter = counter;
	return args;
}

static void
pingpongloop(void* context) {
	args_t args = context;
	//printf("[%p] %s: %lu\n", (void*)GetCurrentThreadId(), dispatch_queue_get_label(dispatch_get_current_queue()), args->counter);
	if (args->counter < final) {
		dispatch_group_async_f(args->group, args->pong, make_args(args->group, args->pong, args->ping, args->counter + 1), &pingpongloop);
		free(args);
	} else {
		count = args->counter;
	}
}

int main(void) {
	
	dispatch_queue_t ping;
	dispatch_queue_t pong;
	dispatch_group_t group;

	test_start("Dispatch Ping Pong");

	ping = dispatch_queue_create("ping", NULL);
	test_ptr_notnull("dispatch_queue_create(ping)", ping);
	pong = dispatch_queue_create("pong", NULL);
	test_ptr_notnull("dispatch_queue_create(pong)", pong);
	
	group = dispatch_group_create();
	test_ptr_notnull("dispatch_group_create", group);
	
	pingpongloop(make_args(group, ping, pong, 0));
	dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

	test_long("count", count, final);
	test_stop();

	return 0;
}
