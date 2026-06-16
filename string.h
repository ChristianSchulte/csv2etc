/* $SchulteIT: string.h 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: string.h 9505 2026-06-11 14:07:36Z schulte $ */

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

#ifndef STRING_H
#define STRING_H

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#include <stdbool.h>
#include <stddef.h>

extern const void *const StringMapOps;

struct String;

void string_init(void);
void string_destroy(void);

struct String *String_new(const struct String *restrict const, const size_t,
                          const size_t);
struct String *String_cnew(const char *restrict);
struct String *String_cnnew(const char *restrict, size_t);

void *String_copy(void *restrict const);
void String_delete(void *restrict const);
size_t String_hash(const void *restrict const);
bool String_equals(const void *restrict const, const void *restrict const);

struct String *String_tolower(const struct String *restrict const, const size_t,
                              const size_t);
struct String *String_toupper(const struct String *restrict const, const size_t,
                              const size_t);

const char *const String_chars(const struct String *restrict const);
const size_t String_length(const struct String *restrict const);
#endif
