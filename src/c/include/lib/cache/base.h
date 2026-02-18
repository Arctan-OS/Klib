/**
 * @file base.h
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan-OS/Kernel - Operating System Kernel
 * Copyright (C) 2023-2026 awewsomegamer
 *
 * This file is part of Arctan-OS/Kernel.
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

#include "lib/spinlock.h"
#include <stddef.h>
#include <stdint.h>

typedef struct ARC_CacheEntry {
        void *vbase; // Virtual Base  - Where this data is stored in virtual memory
        void *pbase; // Physical Base - Where this data is stored on physical media (RAM, disk, etc...)
        uint32_t ref_count;
        struct {
                uint32_t sort_count; // Number of sort cycles entry has undergone
                union {
                        void *ptr;
                        uint64_t count;
                } v;
        } criterion;
} ARC_CacheEntry;

typedef struct ARC_Cache {
        size_t e_size;
        int (*sort)(struct ARC_Cache *);
        int e_count;
        uint32_t grace_period; // sort_count >= grace_period: entry may be evicted
        uint32_t ref_count;
        ARC_Spinlock reorder_lock;
        ARC_CacheEntry *entries[];
} ARC_Cache;

typedef int (*ARC_CacheSorter)(ARC_Cache *);

int cache_add(ARC_Cache *, void *, void *);
int cache_evict_idx(ARC_Cache *, int);
int cache_evict_vbase(ARC_Cache *, void *);
ARC_CacheEntry *cache_get(ARC_Cache *, void *);
int cache_schedule(ARC_Cache *);
int uninit_cache(ARC_Cache *);
ARC_Cache *init_cache(ARC_CacheSorter, int e_count, size_t e_size, uint32_t grace_period);

#endif
