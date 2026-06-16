/* $SchulteIT: heap.h 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: heap.h 9502 2026-06-11 04:46:18Z schulte $ */

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

#ifndef HEAP_H
#define HEAP_H

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#include <stddef.h>

void *heap_malloc(const size_t);
void *heap_calloc(const size_t nmemb, const size_t size);
void *heap_realloc(void *restrict const, const size_t);
void heap_free(void *restrict const);
void heap_trim(const size_t);
#endif
