/**
 * @file base.h
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan-OS/Klib - Generic Kernel Functions and Data Structures
 * Copyright (C) 2023-2026 awewsomegamer
 *
 * This file is part of Arctan-OS/Klib.
 *
 * Arctan is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @DESCRIPTION
*/
#ifndef ARC_LIB_CACHE_BASE_H
#define ARC_LIB_CACHE_BASE_H

#include "drivers/resource.h"
#include <stddef.h>
#include <stdint.h>

typedef struct ARC_CachePage {
        void *base;
        uint32_t ref_count;
} ARC_CachePage;

typedef struct ARC_CacheEntry {
        struct ARC_CacheEntry *next;
        struct ARC_CacheEntry *prev;
        ARC_CachePage *page;
        void *pbase;
        uint32_t ref_count;
        struct {
                uint32_t grace;
                union {
                        void *ptr;
                        uint64_t count;
                } v;
        } criterion;
} ARC_CacheEntry;

typedef struct ARC_Cache {
        ARC_Resource *res;
        int (*sort)(struct ARC_Cache *);
        ARC_CacheEntry *entries;
        ARC_CacheEntry *tail;
        size_t e_size;
        int e_limit; // Maximum number of entries
        int e_count; // Current number of entries
        uint32_t ref_count;
} ARC_Cache;

typedef int (*ARC_CacheSorter)(ARC_Cache *);

ARC_CacheEntry *cache_add(ARC_Cache *, void *);
ARC_CacheEntry *cache_grab(ARC_Cache *, ARC_CacheEntry *, void *);
ARC_CacheEntry *cache_get(ARC_Cache *, void *);

int cache_evict_entry(ARC_Cache *, ARC_CacheEntry *);
int cache_evict_page(ARC_Cache *, void *);
int cache_evict(ARC_Cache *, void *);

int cache_schedule(ARC_Cache *);

int uninit_cache(ARC_Cache *);
ARC_Cache *init_cache(ARC_Resource *, ARC_CacheSorter, int e_count, size_t e_size);

#endif
