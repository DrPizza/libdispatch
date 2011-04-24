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

/*
 * IMPORTANT: This header file describes INTERNAL interfaces to libdispatch
 * which are subject to change in future releases of Mac OS X. Any applications
 * relying on these interfaces WILL break.
 */

#ifndef __DISPATCH_SOURCE_PRIVATE__
#define __DISPATCH_SOURCE_PRIVATE__

#ifndef __DISPATCH_INDIRECT__
#error "Please #include <dispatch/dispatch.h> instead of this file directly."
#include <dispatch/base.h> // for HeaderDoc
#endif

/*!
 * @const DISPATCH_SOURCE_TYPE_VFS
 * @discussion Apple-internal dispatch source that monitors for vfs events
 * defined by dispatch_vfs_flags_t.
 * The handle is a process identifier (pid_t).
 */
#define DISPATCH_SOURCE_TYPE_VFS (&_dispatch_source_type_vfs)
__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
extern const struct dispatch_source_type_s _dispatch_source_type_vfs;

/*!
 * @enum dispatch_source_vfs_flags_t
 *
 * @constant DISPATCH_VFS_NOTRESP
 * Server down.
 *
 * @constant DISPATCH_VFS_NEEDAUTH
 * Server bad auth.
 *
 * @constant DISPATCH_VFS_LOWDISK
 * We're low on space.
 *
 * @constant DISPATCH_VFS_MOUNT
 * New filesystem arrived.
 *
 * @constant DISPATCH_VFS_UNMOUNT
 * Filesystem has left.
 *
 * @constant DISPATCH_VFS_DEAD
 * Filesystem is dead, needs force unmount.
 *
 * @constant DISPATCH_VFS_ASSIST
 * Filesystem needs assistance from external program.
 *
 * @constant DISPATCH_VFS_NOTRESPLOCK
 * Server lockd down.
 *
 * @constant DISPATCH_VFS_UPDATE
 * Filesystem information has changed.
 *
 * @constant DISPATCH_VFS_VERYLOWDISK
 * File system has *very* little disk space left.
 */
enum {
	DISPATCH_VFS_NOTRESP = 0x0001,
	DISPATCH_VFS_NEEDAUTH = 0x0002,
	DISPATCH_VFS_LOWDISK = 0x0004,
	DISPATCH_VFS_MOUNT = 0x0008,
	DISPATCH_VFS_UNMOUNT = 0x0010,
	DISPATCH_VFS_DEAD = 0x0020,
	DISPATCH_VFS_ASSIST = 0x0040,
	DISPATCH_VFS_NOTRESPLOCK = 0x0080,
	DISPATCH_VFS_UPDATE = 0x0100,
	DISPATCH_VFS_VERYLOWDISK = 0x0200,
};

/*!
 * @enum dispatch_source_mach_send_flags_t
 *
 * @constant DISPATCH_MACH_SEND_DELETED
 * The receive right corresponding to the given send right was destroyed.
 */
enum {
	DISPATCH_MACH_SEND_DELETED = 0x2,
};

/*!
 * @enum dispatch_source_proc_flags_t
 *
 * @constant DISPATCH_PROC_REAP
 * The process has been reaped by the parent process via
 * wait*().
 */
enum {
	DISPATCH_PROC_REAP = 0x10000000,
};

__DISPATCH_BEGIN_DECLS

#if HAVE_MACH
/*!
 * @typedef dispatch_mig_callback_t
 *
 * @abstract
 * The signature of a function that handles Mach message delivery and response.
 */
typedef boolean_t (*dispatch_mig_callback_t)(mach_msg_header_t *message, mach_msg_header_t *reply);

__OSX_AVAILABLE_STARTING(__MAC_10_6,__IPHONE_4_0)
DISPATCH_NONNULL_ALL DISPATCH_NOTHROW
mach_msg_return_t
dispatch_mig_server(dispatch_source_t ds, size_t maxmsgsz, dispatch_mig_callback_t callback);
#endif

__DISPATCH_END_DECLS

#endif
