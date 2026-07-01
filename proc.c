/* $SchulteIT: proc.c 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: proc.c 9608 2026-07-01 06:17:27Z schulte $ */

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

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

inline const char *envs(const char *restrict const nm,
                        const char *restrict const dflt) {
  const char *restrict const env = getenv(nm);
  return env != NULL ? env : dflt;
}

inline unsigned long envul(const char *restrict const nm,
                           const unsigned long dflt) {
  char *ep;
  const char *restrict env = getenv(nm);

  if (env == NULL)
    return dflt;

  errno = 0;
  const long v = strtol(env, &ep, 0);

  if (env[0] == '\0' || *ep != '\0')
    fatal("%s: %s: %s", nm, env, ep);

  if (errno == EINVAL || (errno == ERANGE && (v == LONG_MAX || v == LONG_MIN)))
    fatal("%s: %s: %s", nm, env, strerror(errno));

  if (v < 0)
    fatal("%s: %s < 0", nm, env);

  return v;
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
