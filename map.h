/* $SchulteIT: map.h 15189 2025-10-27 05:41:45Z schulte $ */
/* $JDTAUS: map.h 9517 2026-06-14 22:19:40Z schulte $ */

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

#ifndef MAP_H
#define MAP_H

#ifdef HAVE_HOST_H
#include "host.h"
#endif

#include <stdbool.h>
#include <stddef.h>

struct Map;
struct MapIterator;

struct MapOps {
  void *(*k_copy)(void *restrict const);
  void (*k_delete)(void *restrict const);
  size_t (*k_hash)(const void *restrict const);
  bool (*k_equals)(const void *restrict const, const void *restrict const);
};

struct Map *Map_new(const struct MapOps *restrict const, const size_t);

void Map_delete(struct Map *restrict const,
                void (*v_delete)(void *restrict const));

void *Map_put(struct Map *restrict const, void *const, void *const);
void *Map_get(const struct Map *restrict const, const void *restrict const);
void *Map_remove(struct Map *restrict const, void *const);

struct MapIterator *MapIterator_new(const struct Map *restrict const);
void MapIterator_delete(struct MapIterator *restrict const);

bool MapIterator_next(struct MapIterator *restrict const);
void *MapIterator_remove(struct MapIterator *restrict const);

const void *const MapIterator_key(const struct MapIterator *restrict const);
const void *const MapIterator_value(const struct MapIterator *restrict const);

#ifdef MULTI_THREADED
void Map_lock(struct Map *restrict const);
bool Map_trylock(struct Map *restrict const);
void Map_unlock(struct Map *restrict const);
#endif
#endif
