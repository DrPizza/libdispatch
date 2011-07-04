/*
 * libdispatch asprintf portability shim, based on 
 * http://www.opensource.apple.com/source/OpenSSH/OpenSSH-142/openssh/openbsd-compat/bsd-asprintf.c
 *
 *
 * Copyright (c) 2004 Darren Tucker.
 *
 * Based originally on asprintf.c from OpenBSD:
 * Copyright (c) 1997 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config/config.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef HAVE_ASPRINTF
int asprintf(char **str, const char *fmt, ...)
{
	va_list ap;
	size_t n, size = 32;
	char* p;
	char* np;
	
	if((p = malloc(size)) == NULL)
	{
		return -1;
	}

	while(1)
	{
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);
		if(n > -1 && n < size)
		{
			*str = p;
			return n;
		}
		else
		{
			size *= 2;
		}
		if((np = realloc(p, size)) == NULL)
		{
			free(p);
			return -1;
		}
		else
		{
			p = np;
		}
	}
}
#endif
