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
#include "arch/x86-64/config.h"
#include "drivers/resource.h"
#include "global.h"
#include "lib/atomics.h"
#include "mm/allocator.h"
#include "lib/util.h"
#include "mm/pmm.h"

/*
  The idea is this:

  Processes will hold a small L1 cache that can be accessed by operations
  which support caching - currently planned: drivers and file system.

  These L1 caches may grab entries from L2 caches and sort their grabbed
  entries to allow operations to more quickly find the right cache entry.
  If an entry is not found in an L1 cache by an operation, then it is the
  duty of the operation to give the L1 cache the L2 cache entry. If no L2
  cache entry is found, the operation is done, and an L2 cache entry created
  which is given to the L1 cache.

  The L2 caches are unordered and can be of an indefinite length.

  L2 caches may share entries amongst each other such that their vbase pointers
  correspond while their pbase pointers differ. This creates a cached page with
  multipled facets for both input and output.

  PROBLEM: How can it be ensured that the page is consistent? For instance, two
  write operations are taking place to the page, one write is slightly delayed,
  the page is now in three different states to a reader.

  SOLUTION: Write operations may happen simultaneously and so can read operations, but
  read and write operations cannot happen simultaneously. Employ two semaphores (rcount, wcount),
  if rcount is non-zero write operations must wait, if wcount is non-zero read operations must wait.
*/

// TODO: Should it be enforced that a cache is strictly e_limit elements long and that (e_count - e_limit) + 1
//       entries are evicted if e_count >= e_limit, or should this be softly enforced so just the last entry is evicted,
//       or should no entries be evicted at all and e_limit abolished?

static ARC_CacheEntry *icache_add(ARC_Cache *cache, ARC_CachePage *page, void *pbase) {
        ARC_CacheEntry *entry = alloc(sizeof(*entry));

        if (entry == NULL) {
                return NULL;
        }

        memset(entry, 0, sizeof(*entry));
        
        entry->criterion.grace = 10; // TODO: Determine this dynamically
        entry->page = page;
        entry->pbase = pbase;
        
        ARC_CacheEntry *check = cache_get(cache, pbase);

        if (check != NULL) {
                free(page);
                free(entry);
                pmm_free(page->base);
                return check;
        }
        
        ARC_CacheEntry *t = entry;
        ARC_ATOMIC_XCHG(&cache->entries, &t, &entry->next);

        if (t == NULL) {
                ARC_ATOMIC_STORE(cache->tail, entry);
        }
        
        if (ARC_ATOMIC_INC(cache->e_count) >= cache->e_limit) {
                cache_evict_entry(cache, cache->tail);
        }
        
        return entry;
}

ARC_CacheEntry *cache_add(ARC_Cache *cache, void *pbase) {
        if (cache == NULL) {
                return NULL;
        }
        
        void *vbase = pmm_alloc(cache->e_size);

        if (vbase == NULL) {
                return NULL;
        }
        
        ARC_CachePage *page = alloc(sizeof(*page));

        if (page == NULL) {
                free(vbase);
                return NULL;
        }

        page->base = vbase;
        
        return icache_add(cache, page, pbase);
}

ARC_CacheEntry *cache_grab(ARC_Cache *to, ARC_CacheEntry *entry, void *pbase) {
        if (to == NULL || entry == NULL) {
                return NULL;
        }
        
        return icache_add(to, entry->page, pbase);
}

ARC_CacheEntry *cache_get(ARC_Cache *cache, void *pbase) {
        ARC_ATOMIC_SFENCE;

        ARC_CacheEntry *entry = ARC_ATOMIC_LOAD(cache->entries);
        ARC_ATOMIC_INC(entry->ref_count);
        while (entry != NULL && entry->pbase != pbase) {
                ARC_CacheEntry *t = entry;
                entry = ARC_ATOMIC_LOAD(entry->next);
                ARC_ATOMIC_INC(entry->ref_count);
                ARC_ATOMIC_DEC(t->ref_count);                
        }

        if (entry != NULL) {
                ARC_CacheEntry *t = NULL;
                ARC_ATOMIC_LFENCE;
                ARC_ATOMIC_XCHG(&entry->prev->next, &entry->next, &t);
                ARC_ATOMIC_XCHG(&entry->next->prev, &entry->prev, &t);
                t = entry;
                ARC_ATOMIC_XCHG(&cache->entries, &t, &entry->next);
                ARC_ATOMIC_SFENCE;
        }
        
        return entry;
}

int cache_sync(ARC_Cache *cache, ARC_CacheEntry *entry) {
        if (cache == NULL || entry == NULL || cache->res == NULL) {
                return -1;
        }
        
        ARC_CachePage *page = entry->page;        
        const ARC_DriverDef *def = cache->res->driver;
        ARC_File f = { .node = NULL, .offset = (uintptr_t)entry->pbase };
        return (def->write(page->base, cache->e_size, 1, &f, cache->res) != cache->e_size);
}

int cache_evict_entry(ARC_Cache *cache, ARC_CacheEntry *entry) {
        if (cache == NULL || entry == NULL) {
                return -1;
        }

        void *t = NULL;
        ARC_ATOMIC_LFENCE;
        ARC_ATOMIC_XCHG(&entry->prev->next, &entry->next, (ARC_CacheEntry **)&t);
        ARC_ATOMIC_XCHG(&entry->prev, &entry->next->prev, (ARC_CacheEntry **)&t);
        ARC_ATOMIC_DEC(cache->e_count);
        ARC_ATOMIC_SFENCE;

        if (ARC_ATOMIC_LOAD(cache->tail) == entry) {
                ARC_ATOMIC_STORE(cache->tail, t);
        }
        
        ARC_CachePage *page = entry->page;
        while (ARC_ATOMIC_LOAD(page->ref_count) > 0) {
                // TODO: Pause
        }
        
        cache_sync(cache, entry);

        free(page->base);
        free(page);
        free(entry);
        
        return 0;
}

int cache_evict_vbase(ARC_Cache *cache, void *vbase) {
        if (cache == NULL || vbase == NULL) {
                return -1;
        }

        ARC_ATOMIC_SFENCE;
        ARC_CacheEntry *entry = ARC_ATOMIC_LOAD(cache->entries);
        ARC_ATOMIC_INC(entry->ref_count);
        while (entry != NULL) {
                ARC_CachePage *page = ARC_ATOMIC_LOAD(entry->page);
                ARC_ATOMIC_INC(page->ref_count);
                if (page->base == vbase) {
                        break;
                }
                ARC_ATOMIC_DEC(page->ref_count);
                
                ARC_CacheEntry *t = entry;                
                entry = ARC_ATOMIC_LOAD(entry->next);
                ARC_ATOMIC_INC(entry->ref_count);
                ARC_ATOMIC_DEC(t->ref_count);   
        }
        
        return cache_evict_entry(cache, entry);
}

int cache_evict(ARC_Cache *cache, void *pbase) {
        if (cache == NULL) {
                return -1;
        }
        
        ARC_CacheEntry *entry = cache_get(cache, pbase);
        
        return cache_evict_entry(cache, entry);
}

int cache_schedule(ARC_Cache *cache) {
        ARC_DEBUG(WARN, "Definitely scheduling intermittent cache sorts\n");
        return 0;
}

int uninit_cache(ARC_Cache *cache) {
        if (cache == NULL) {
                ARC_DEBUG(ERR, "No cache given\n");
                return -1;
        }

        ARC_DEBUG(WARN, "Definitely uninitializing the cache\n");
        
        return 0;
}

ARC_Cache *init_cache(ARC_Resource *res, ARC_CacheSorter sorter, int e_count, size_t e_size) {
        ARC_Cache *cache = alloc(sizeof(*cache));

        if (cache == NULL) {
                ARC_DEBUG(ERR, "Failed to allocate memory for cache\n");
                return NULL;
        }

        memset(cache, 0, sizeof(*cache));
        
        cache->e_limit = e_count;
        cache->e_size = e_size;
        cache->sort = sorter; // If sorter == NULL then entries added / gotten are put at the head of the linked list
        cache->res = res;
        
        return cache;
}
