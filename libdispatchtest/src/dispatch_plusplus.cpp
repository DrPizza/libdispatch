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

#include <dispatch.hpp>
#include <iostream>

void async_f(void*)
{
	test_stop();
	exit(0);
}

int main(void) {
	test_start("Dispatch C++");

	gcd::queue q = gcd::queue::get_main_queue();

	q.async([] {
		test_stop();
		exit(0);
	});

	dispatch_main();
	return 0;
}
