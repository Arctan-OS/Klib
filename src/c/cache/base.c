/**
 * @file base.c
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
#include "lib/cache/base.h"
#include "global.h"
#include "lib/atomics.h"
#include "lib/util.h"
#include "mm/allocator.h"

// Probably would be smarter to do this with a XOR linked list

int cache_add(ARC_Cache *cache, void *vbase, void *pbase) {
        if (cache == NULL) {
                return -1;
        }

        cache->sort(cache);
        
        ARC_CacheEntry *entry = alloc(sizeof(*entry));

        if (entry == NULL) {
                return -2;
        }
        
        ARC_ATOMIC_INC(cache->ref_count);
        spinlock_lock(&cache->reorder_lock);
        
        entry->vbase = vbase;
        entry->pbase = pbase;

        for (int i = cache->e_count; i >= 1; i--) {
                ARC_CacheEntry *i_entry = cache->entries[i];
                if (i_entry == NULL) {
                        cache->entries[i] = entry;
                        goto done;
                }
        }
        
        cache->entries[cache->e_count] = entry;
        
        ARC_CacheEntry *tmp = NULL;
        for (int i = cache->e_count; i >= 1; i--) {
                ARC_ATOMIC_XCHG(&cache->entries[i], &cache->entries[i - 1], &tmp);
        }

 done:;
        spinlock_unlock(&cache->reorder_lock);
        ARC_ATOMIC_DEC(cache->ref_count);
        
        return 0;
}

static int cache_evict(ARC_CacheEntry **entry) {
        ARC_CacheEntry *t = ARC_ATOMIC_LOAD(*entry);
        int rc = ARC_ATOMIC_INC(t->ref_count);
        
        if (rc > 1) {
                ARC_ATOMIC_DEC(t->ref_count);
                return -1;
        }
        
        
        ARC_ATOMIC_LFENCE;

        ARC_ATOMIC_STORE(*entry, NULL);
        free(t);
        
        return 0;
}

int cache_evict_idx(ARC_Cache *cache, int idx) {
        if (cache == NULL || idx < 0 || idx > cache->e_count) {
                return -1;
        }

        ARC_ATOMIC_INC(cache->ref_count);
        spinlock_lock(&cache->reorder_lock);
        int r = cache_evict(&cache->entries[idx]);
        spinlock_unlock(&cache->reorder_lock);
        ARC_ATOMIC_DEC(cache->ref_count);
        
        return r;
}

int cache_evict_vbase(ARC_Cache *cache, void *vbase) {
        if (cache == NULL || vbase == NULL) {
                return -1;
        }

        int r = 0;
        
        ARC_ATOMIC_INC(cache->ref_count);
        spinlock_lock(&cache->reorder_lock);
        
        for (int i = 0; i < cache->e_count; i++) {
                ARC_CacheEntry **entry = &cache->entries[i];

                if (*entry != NULL && ARC_ATOMIC_LOAD((*entry)->vbase) == vbase) {
                        r = cache_evict(entry);
                        break;
                }
        }
        
        spinlock_unlock(&cache->reorder_lock);
        ARC_ATOMIC_DEC(cache->ref_count);
        
        return r;
}

ARC_CacheEntry *cache_get(ARC_Cache *cache, void *pbase) {
        if (cache == NULL) {
                return NULL;
        }

        ARC_CacheEntry *ret = NULL;
        
        ARC_ATOMIC_INC(cache->ref_count);

        for (int i = 0; i < cache->e_count; i++) {                
               ARC_CacheEntry *entry = cache->entries[i];
               void *e_pbase = entry->pbase;
               if (entry != NULL && (e_pbase >= pbase || pbase < e_pbase + cache->e_size)) {
                       ret = entry;
                       break;
                } 
        }
        
        ARC_ATOMIC_DEC(cache->ref_count);

        return ret;
}

int cache_schedule(ARC_Cache *cache) {
        (void)cache;
        ARC_DEBUG(WARN, "Definitely scheduling the sort function to happen on this cache\n");
        return 0;
}

int uninit_cache(ARC_Cache *cache) {
        if (cache == NULL) {
                ARC_DEBUG(ERR, "Cannot free NULL cache\n");
                return -1;
        }
        
        int rc = ARC_ATOMIC_INC(cache->ref_count);
        if (rc > 1) {
                ARC_DEBUG(ERR, "Cannot free cache (%p) that is in use, or already being freed\n", cache);
                ARC_ATOMIC_DEC(cache->ref_count);
                return -2;
        }

        for (int i = 0; i < cache->e_count; i++) {
                if (cache_evict_idx(cache, i) != 0) {
                        ARC_CacheEntry *entry = cache->entries[i];
                        ARC_DEBUG(ERR, "Failed to evict i=%lu (vbase=%p, pbase=%p)\n", i, entry->vbase, entry->pbase);
                        ARC_ATOMIC_DEC(cache->ref_count);
                        return -3;
                }
        }
        
        free(cache);
        
        return 0;
}

ARC_Cache *init_cache(ARC_CacheSorter sorter, int e_count, size_t e_size, uint32_t grace_period) {
        if (e_count == 0 || e_size == 0) {
                ARC_DEBUG(ERR, "Cannot initialize cache with 0 entries, or with entries of size 0 bytes\n");
                return 0;
        }

        // One more entry is allocated as a NULL to make the swapping logic easier
        ARC_Cache *cache = alloc(sizeof(*cache) + (e_count + 1) * sizeof(void *));

        if (cache == NULL) {
                ARC_DEBUG(ERR, "Failed to allocate memory for cache\n");
                return NULL;
        }

        memset(cache, 0, sizeof(*cache));

        cache->e_count = e_count;
        cache->e_size = e_size;
        cache->sort = sorter;
        cache->grace_period = grace_period;

        return NULL;
}
