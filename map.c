/* $SchulteIT: map.c 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: map.c 9571 2026-06-26 02:11:30Z schulte $ */

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

struct Entry {
  void *restrict key;
  void *restrict value;
  struct Entry *restrict next;
  struct Entry *restrict prev;
};

struct Map {
  const struct MapOps *restrict ops;
  struct Entry **restrict buckets;
#ifdef MULTI_THREADED
  mtx_t mtx;
#endif
  size_t capacity;
};

struct MapIterator {
  size_t i;
  struct Entry *restrict e;
  const struct Map *restrict m;
};

inline struct Map *Map_new(const struct MapOps *restrict const ops,
                           const size_t capacity) {
  struct Map *restrict const m = heap_malloc(sizeof(struct Map));
  m->ops = ops;
  m->capacity = capacity;
  m->buckets = heap_calloc(capacity, sizeof(struct Entry *));
#ifdef MULTI_THREADED
  mutex_init(&m->mtx);
#endif
  return m;
}

inline void Map_delete(struct Map *restrict const m,
                       void (*v_delete)(void *restrict const)) {
  for (size_t i = m->capacity; i-- > 0;) {
    struct Entry *restrict e = m->buckets[i];

    while (e != NULL) {
      if (v_delete)
        v_delete(e->value);

      m->ops->k_delete(e->key);

      struct Entry *tmp = e;
      e = e->next;
      heap_free(tmp);
    }
  }

#ifdef MULTI_THREADED
  mutex_destroy(&m->mtx);
#endif
  heap_free(m->buckets);
  heap_free(m);
}

inline void *Map_put(struct Map *restrict const m, void *const k,
                     void *const v) {
  const size_t i = m->ops->k_hash(k) % m->capacity;
  struct Entry *restrict e = m->buckets[i];
  void *restrict value = NULL;

  while (e != NULL && !m->ops->k_equals(e->key, k))
    e = e->next;

  if (e != NULL) {
    value = e->value;
    e->value = v;
  } else {
    e = heap_malloc(sizeof(struct Entry));
    e->key = m->ops->k_copy(k);
    e->value = v;
    e->prev = NULL;

    if ((e->next = m->buckets[i]) != NULL)
      m->buckets[i]->prev = e;

    m->buckets[i] = e;
  }

  return value;
}

inline void *Map_get(const struct Map *restrict const m,
                     const void *restrict const k) {
  struct Entry *restrict e = m->buckets[m->ops->k_hash(k) % m->capacity];

  while (e != NULL && !m->ops->k_equals(e->key, k))
    e = e->next;

  return e != NULL ? e->value : NULL;
}

inline void *Map_remove(struct Map *restrict const m, void *const k) {
  const size_t i = m->ops->k_hash(k) % m->capacity;
  struct Entry *restrict e = m->buckets[i];
  void *restrict value = NULL;

  while (e != NULL && !m->ops->k_equals(e->key, k))
    e = e->next;

  if (e != NULL) {
    value = e->value;
    m->ops->k_delete(e->key);

    if (e->prev)
      e->prev->next = e->next;
    else
      m->buckets[i] = e->next;

    if (e->next)
      e->next->prev = e->prev;

    heap_free(e);
  }

  return value;
}

inline struct MapIterator *MapIterator_new(const struct Map *restrict const m) {
  struct MapIterator *restrict const it =
      heap_malloc(sizeof(struct MapIterator));

  it->i = m->capacity;
  it->e = NULL;
  it->m = m;
  return it;
}

inline void MapIterator_delete(struct MapIterator *restrict const it) {
  heap_free(it);
}

inline bool MapIterator_next(struct MapIterator *restrict const it) {
  if (it->e != NULL)
    it->e = it->e->next;

  while (it->e == NULL && it->i != 0)
    it->e = it->m->buckets[--it->i];

  return it->e != NULL;
}

inline void *MapIterator_remove(struct MapIterator *restrict const it) {
  void *restrict value = NULL;

  if (it->e != NULL) {
    value = it->e->value;

    if (it->e->prev)
      it->e->prev->next = it->e->next;
    else
      it->m->buckets[it->i] = it->e->next;

    if (it->e->next)
      it->e->next->prev = it->e->prev;

    it->m->ops->k_delete(it->e->key);
    heap_free(it->e);
    it->e = NULL;
    it->i++;
  }

  return value;
}

inline const void *const
MapIterator_key(const struct MapIterator *restrict const it) {
  return it->e != NULL ? it->e->key : NULL;
}

inline const void *const
MapIterator_value(const struct MapIterator *restrict const it) {
  return it->e != NULL ? it->e->value : NULL;
}

#ifdef MULTI_THREADED
inline void Map_lock(struct Map *restrict const m) { mutex_lock(&m->mtx); }
inline bool Map_trylock(struct Map *restrict const m) {
  return mutex_trylock(&m->mtx);
}
inline void Map_unlock(struct Map *restrict const m) { mutex_unlock(&m->mtx); }
#endif
