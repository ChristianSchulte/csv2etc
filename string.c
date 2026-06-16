/* $SchulteIT: string.c 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: string.c 9516 2026-06-14 22:14:26Z schulte $ */

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
#endif

#include "heap.h"
#include "map.h"
#include "proc.h"
#include "string.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

struct String {
  char *restrict s;
  size_t len;
  size_t hc;
  size_t r_cnt;
#ifdef MULTI_THREADED
  mtx_t mtx;
#endif
};

const void *const StringMapOps = &(const struct MapOps){
    .k_copy = String_copy,
    .k_delete = String_delete,
    .k_hash = String_hash,
    .k_equals = String_equals,
};

struct Map *restrict strings;

void string_init(void) { strings = Map_new(StringMapOps, 524288); }
void string_destroy(void) { Map_delete(strings, String_delete); }

inline struct String *String_cnew(const char *restrict s) {
  size_t hc = 5381;
  const char *s_p;

  for (s_p = s; *s_p; s_p++)
    hc = ((hc << 5) + hc) + (unsigned char)*s_p;

  // XXX: (char *)
  struct String k = {
      .s = (char *)s,
      .len = s_p - s,
      .hc = hc,
      .r_cnt = 0,
  };
#ifdef MULTI_THREADED
  Map_lock(strings);
#endif
  struct String *restrict str = Map_get(strings, &k);

  if (str == NULL) {
    str = heap_malloc(sizeof(struct String));
    str->len = k.len;
    str->hc = k.hc;
    str->r_cnt = 1;
    str->s = heap_calloc(str->len + 1, sizeof(char));
    memcpy(str->s, s, str->len);
#ifdef MULTI_THREADED
    mutex_init(&str->mtx);
#endif
    Map_put(strings, str, str);
  }
#ifdef MULTI_THREADED
  Map_unlock(strings);
#endif
  return String_copy(str);
}

inline struct String *String_cnnew(const char *restrict s, size_t maxlen) {
  size_t hc = 5381;
  const char *s_p;

  for (s_p = s; *s_p && maxlen != 0; s_p++, maxlen--)
    hc = ((hc << 5) + hc) + (unsigned char)*s_p;

  // XXX: (char *)
  struct String k = {
      .s = (char *)s,
      .len = s_p - s,
      .hc = hc,
      .r_cnt = 0,
  };
#ifdef MULTI_THREADED
  Map_lock(strings);
#endif
  struct String *restrict str = Map_get(strings, &k);

  if (str == NULL) {
    str = heap_malloc(sizeof(struct String));
    str->len = k.len;
    str->hc = k.hc;
    str->r_cnt = 1;
    str->s = heap_calloc(str->len + 1, sizeof(char));
    memcpy(str->s, s, str->len);
#ifdef MULTI_THREADED
    mutex_init(&str->mtx);
#endif
    Map_put(strings, str, str);
  }
#ifdef MULTI_THREADED
  Map_unlock(strings);
#endif
  return String_copy(str);
}

inline struct String *String_new(const struct String *restrict s,
                                 const size_t i, const size_t c) {
  // i + c + 1 <= SIZE_MAX
  // => i <= SIZE_MAX - c - 1
  // => c <= SIZE_MAX - i - 1
  if (i > SIZE_MAX - c - 1 || c > SIZE_MAX - i - 1 || i + c > s->len)
    panic();

  struct String *restrict const str = heap_malloc(sizeof(struct String));
  str->len = c;
  str->hc = 5381;
  str->r_cnt = 1;
  str->s = heap_calloc(str->len + 1, sizeof(char));

  char *s_p = s->s + i;
  char *d_p = str->s;
  size_t l = str->len;

  while (l-- != 0) {
    str->hc = ((str->hc << 5) + str->hc) + (unsigned char)*s_p;
    *d_p++ = *s_p++;
  }
#ifdef MULTI_THREADED
  mutex_init(&str->mtx);
#endif
  return str;
}

inline const char *const String_chars(const struct String *restrict const s) {
  return s->s;
}

inline const size_t String_length(const struct String *restrict const s) {
  return s->len;
}

inline size_t String_hash(const void *restrict const s) {
  return ((const struct String *)s)->hc;
}

inline void String_delete(void *restrict const s) {
  if (s == NULL)
    return;

  struct String *restrict const str = s;

#ifdef MULTI_THREADED
  mutex_lock(&str->mtx);
#endif
  if (str->r_cnt-- == 0)
    panic();

  if (str->r_cnt > 0) {
#ifdef MULTI_THREADED
    mutex_unlock(&str->mtx);
#endif
    return;
  }
#ifdef MULTI_THREADED
  mutex_unlock(&str->mtx);
  mutex_destroy(&str->mtx);
#endif
  heap_free(str->s);
  heap_free(s);
}

inline void *String_copy(void *restrict const o) {
  if (o == NULL)
    return NULL;

  struct String *restrict const str = o;
#ifdef MULTI_THREADED
  mutex_lock(&str->mtx);
#endif
  // str->r_cnt + 1 <= SIZE_MAX
  // => str->r_cnt <= SIZE_MAX - 1
  if (str->r_cnt > SIZE_MAX - 1)
    panic();

  str->r_cnt++;
#ifdef MULTI_THREADED
  mutex_unlock(&str->mtx);
#endif
  return str;
}

inline bool String_equals(const void *restrict const o1,
                          const void *restrict const o2) {
  return ((const struct String *)o1)->hc == ((const struct String *)o2)->hc &&
         ((const struct String *)o1)->len == ((const struct String *)o2)->len &&
         memcmp(((const struct String *)o1)->s, ((const struct String *)o2)->s,
                ((const struct String *)o1)->len) == 0;
}

inline struct String *String_tolower(const struct String *restrict s,
                                     const size_t i, const size_t c) {
  // i + c + 1 <= SIZE_MAX
  // => i <= SIZE_MAX - c - 1
  // => c <= SIZE_MAX - i - 1
  if (i > SIZE_MAX - c - 1 || c > SIZE_MAX - i - 1 || i + c > s->len)
    panic();

  struct String *restrict const str = heap_malloc(sizeof(struct String));
  str->len = c;
  str->hc = 5381;
  str->r_cnt = 1;
  str->s = heap_calloc(str->len + 1, sizeof(char));

  char *s_p = s->s + i;
  char *d_p = str->s;
  size_t l = str->len;

  while (l-- != 0) {
    str->hc = ((str->hc << 5) + str->hc) + (unsigned char)*s_p;
    *d_p++ = tolower(*s_p++);
  }
#ifdef MULTI_THREADED
  mutex_init(&str->mtx);
#endif
  return str;
}

inline struct String *String_toupper(const struct String *restrict s,
                                     const size_t i, const size_t c) {
  // i + c + 1 <= SIZE_MAX
  // => i <= SIZE_MAX - c - 1
  // => c <= SIZE_MAX - i - 1
  if (i > SIZE_MAX - c - 1 || c > SIZE_MAX - i - 1 || i + c > s->len)
    panic();

  struct String *restrict const str = heap_malloc(sizeof(struct String));
  str->len = c;
  str->hc = 5381;
  str->r_cnt = 1;
  str->s = heap_calloc(str->len + 1, sizeof(char));

  char *s_p = s->s + i;
  char *d_p = str->s;
  size_t l = str->len;

  while (l-- != 0) {
    str->hc = ((str->hc << 5) + str->hc) + (unsigned char)*s_p;
    *d_p++ = toupper(*s_p++);
  }
#ifdef MULTI_THREADED
  mutex_init(&str->mtx);
#endif
  return str;
}
