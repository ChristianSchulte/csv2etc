/* $SchulteIT: proc.c 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: proc.c 9511 2026-06-13 19:28:18Z schulte $ */

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

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#ifdef MULTI_THREADED
#include "thread.h"
mtx_t stdout_mtx;
mtx_t stderr_mtx;
#endif

#include "proc.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool proc_prefix_systemd = false;

inline void wout(const char *restrict fmt, ...) {
  va_list ap;
#ifdef MULTI_THREADED
  mutex_lock(&stdout_mtx);
#endif
  if (proc_prefix_systemd)
    (void)fprintf(stdout, "<6>");
  va_start(ap, fmt);
  (void)vfprintf(stdout, fmt, ap);
  va_end(ap);
  (void)fflush(stdout);
#ifdef MULTI_THREADED
  mutex_unlock(&stdout_mtx);
#endif
}

inline void werr(const char *restrict fmt, ...) {
  va_list ap;
#ifdef MULTI_THREADED
  mutex_lock(&stderr_mtx);
#endif
  if (proc_prefix_systemd)
    (void)fprintf(stderr, "<3>");
  va_start(ap, fmt);
  (void)vfprintf(stderr, fmt, ap);
  va_end(ap);
  (void)fflush(stderr);
#ifdef MULTI_THREADED
  mutex_unlock(&stderr_mtx);
#endif
}

#ifdef MULTI_THREADED
void proc_init(void) {
  mutex_init(&stdout_mtx);
  mutex_init(&stderr_mtx);
}

void proc_destroy(void) {
  mutex_destroy(&stdout_mtx);
  mutex_destroy(&stderr_mtx);
}
#endif
