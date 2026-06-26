/* $SchulteIT: proc.h 15260 2025-11-04 03:03:57Z schulte $ */
/* $JDTAUS: proc.h 9534 2026-06-15 09:02:46Z schulte $ */

/*
 * Copyright (c) 2018 - 2026 Christian Schulte <cs@schulte.it>
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

#ifndef PROC_H
#define PROC_H

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#include <stdlib.h>

#define panic()                                                                \
  (werr("%s: Abort: %s: %d\n", __func__, __FILE__, __LINE__), abort())

#define fatal(_fmt, ...)                                                       \
  (werr("%s: Failure: ", __func__), werr((_fmt), __VA_ARGS__), werr("\n"),     \
   exit(EXIT_FAILURE))

void wout(const char *, ...) __attribute__((__format__(printf, 1, 2)));
void werr(const char *, ...) __attribute__((__format__(printf, 1, 2)));

#ifdef MULTI_THREADED
void proc_init(void);
void proc_destroy(void);
#endif
#endif
