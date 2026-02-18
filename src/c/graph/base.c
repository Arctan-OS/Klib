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
#include "arch/x86-64/gdt.h"
#include "global.h"
#include "lib/atomics.h"
#include "lib/graph/base.h"
#include "lib/util.h"
#include "mm/allocator.h"

#include <stddef.h>

#define REMOVE_GUARD(_node_, _action_) \
        if (ARC_ATOMIC_LOAD(_node_->next) == _node_) { \
                _action_; \
        } 

static const char *graph_empty_name = "";

ARC_GraphNode *graph_create(size_t arb_size) {
        ARC_GraphNode *node = alloc(sizeof(*node) + arb_size);

        if (node == NULL) {
                return NULL;
        }

        memset(node, 0, sizeof(*node) + arb_size);

        node->arb_size = arb_size;

        return node;
}

int graph_add(ARC_GraphNode *parent, ARC_GraphNode *node, char *_name) {
        if (parent == NULL || node == NULL) {
                return -1;
        }
        
        REMOVE_GUARD(parent, return -2)

        ARC_ATOMIC_INC(parent->ref_count); // A
        
        char *name = *(char **)&graph_empty_name;
        if (_name != NULL) {
                name = strdup(_name);
        } else if (node->name != NULL) {
                name = node->name;
        }

        node->name = name;

        node->parent = parent;
        ARC_ATOMIC_XCHG(&parent->child, &node, &node->next);
        ARC_ATOMIC_INC(parent->child_count);
        ARC_ATOMIC_DEC(parent->ref_count); // A
        
        return 0;
}

ARC_GraphNode *graph_duplicate(ARC_GraphNode *node) {
        if (node == NULL) {
                return NULL;
        }

        REMOVE_GUARD(node, return NULL)
        ARC_ATOMIC_INC(node->ref_count); // A
        
        ARC_GraphNode *dup = graph_create(node->arb_size);

        if (dup == NULL) {
                return NULL;
        }

        memcpy(dup, node, sizeof(*node) + node->arb_size);

        dup->name = strdup(node->name);
        dup->child = NULL;

        ARC_ATOMIC_DEC(node->ref_count); // A
        
        return dup;
}

static int graph_recursive_free(ARC_GraphNode *node) {
        if (ARC_ATOMIC_LOAD(node->ref_count) > 1) {
                return 0;
        }

        int r = 0;

        ARC_GraphNode *current = ARC_ATOMIC_LOAD(node->child);
        while (current != NULL) {
                r += graph_recursive_free(current);
                current = ARC_ATOMIC_LOAD(current->next);
        }

        if (r == 0) {
                if (node->name != graph_empty_name) {
                        free(node->name);
                }
                free(node);
        }

        return r;
}

static ARC_GraphNode *graph_get_prev(ARC_GraphNode *node) {
        ARC_ATOMIC_INC(node->ref_count); // A
        ARC_GraphNode *parent = node->parent;
        ARC_ATOMIC_INC(parent->ref_count); // B
        ARC_GraphNode *prev = NULL;
        
        full_recheck:;
        ARC_GraphNode *current = ARC_ATOMIC_LOAD(parent->child);
        if (current == node) {
                goto std_ret;
        }

        ARC_ATOMIC_INC(current->ref_count); // C
        while (current != NULL && current != node) {
                prev = current;
                current = ARC_ATOMIC_LOAD(current->next);

                if (current == prev) {
                        goto full_recheck;
                }

                if (current != NULL) {
                        ARC_ATOMIC_INC(current->ref_count); // D
                }
                if (current != node) {
                        ARC_ATOMIC_DEC(prev->ref_count); // C, D
                }
        }
        
        if (current == NULL) {
                prev = NULL;
        } else {
                // NOTE: prev, current = node, are left incremented
                //       -1 from node as it has already been incremented
                ARC_ATOMIC_DEC(current->ref_count);
        }
        
        std_ret:;
        ARC_ATOMIC_DEC(parent->ref_count); // B
        ARC_ATOMIC_DEC(node->ref_count); // A
        // prev->ref_count is left incremented, caller must decrement it
        
        return prev;
}

int graph_remove(ARC_GraphNode *node, bool free) {
        if (node == NULL) {
                ARC_DEBUG(ERR, "No node given\n");
                return -1;
        }

        int rc = 0;
        if ((rc = ARC_ATOMIC_INC(node->ref_count)) > 1) { // NOTE: Prevents other remove operations (A)
                ARC_ATOMIC_DEC(node->ref_count); // A
                ARC_DEBUG(ERR, "Node in use or already being removed %d\n", rc);

                return -2;
        }

        ARC_GraphNode *parent = node->parent;
        ARC_ATOMIC_INC(parent->ref_count); // B
        ARC_GraphNode *prev = NULL;
        
        if ((parent != NULL && ARC_ATOMIC_LOAD(parent->child) == node)
            || (prev = graph_get_prev(node)) == NULL) {
                ARC_ATOMIC_XCHG(&parent->child, &node->next, &node->next);
        } else if (prev != NULL) {
                ARC_ATOMIC_XCHG(&prev->next, &node->next, &node->next);
                ARC_ATOMIC_DEC(prev->ref_count);              
        }

        if (node->next != node) {
                ARC_DEBUG(ERR, "Something went wrong\n");
                ARC_HANG;
        }

        node->parent = NULL;
        
        ARC_ATOMIC_DEC(parent->child_count);
        ARC_ATOMIC_DEC(parent->ref_count); // B  

        if (free) {
                return graph_recursive_free(node) > 0 ? 0 : -4;
        } else {
                ARC_ATOMIC_DEC(node->ref_count); // A
        }

        return 0;
}

// ret->ref_count is left incremented, caller must decrement it
ARC_GraphNode *graph_find(ARC_GraphNode *parent, char *targ) {
        if (parent == NULL || targ == NULL) {
                return NULL;
        }

        REMOVE_GUARD(parent, return NULL)
        
        ARC_ATOMIC_INC(parent->ref_count); // A
        full_recheck:;
        size_t a = ARC_ATOMIC_LOAD(parent->child_count);

        ARC_GraphNode *current = ARC_ATOMIC_LOAD(parent->child);

        if (current == NULL) {
                ARC_ATOMIC_DEC(parent->ref_count); // A
                return NULL;
        }
        
        ARC_GraphNode *initial = current;
        
        ARC_ATOMIC_INC(current->ref_count); // B

        while (current != NULL && strcmp(targ, current->name) != 0) {
                ARC_GraphNode *t = current;
                current = ARC_ATOMIC_LOAD(current->next);
                if (current == t) {
                        ARC_DEBUG(INFO, "Current == Last, full recheck needed\n");
                        goto full_recheck;
                }

                if (current != NULL) {
                        ARC_ATOMIC_INC(current->ref_count); // C
                }
                ARC_ATOMIC_DEC(t->ref_count); // C, B
        }

        size_t b = ARC_ATOMIC_LOAD(parent->child_count);

        if (current != NULL) {
                ARC_ATOMIC_DEC(parent->ref_count); // A
                return current;
        }

        size_t delta = b - a;

        if (a > b) {
                // Nodes have been removed, potentially added, recheck every node
                goto full_recheck;
        } else if (a == b) {
                // Check the current child node, most likely one node was removed and
                // potentially the target node added
                delta = 1;
        } else {
                // Nodes have been added, recheck delta
        }

        current = ARC_ATOMIC_LOAD(parent->child);

        if ((delta == 1 && current == initial) || current == NULL) {
                ARC_ATOMIC_DEC(parent->ref_count); // A
                return NULL;
        }

        ARC_ATOMIC_INC(current->ref_count); // D

        int r = -1;
        while (current != NULL && (r = strcmp(targ, current->name)) != 0 && delta > 0) {
                ARC_GraphNode *t = current;
                current = ARC_ATOMIC_LOAD(current->next);

                if (current == t) {
                        goto full_recheck;
                }

                if (current != NULL) {
                        ARC_ATOMIC_INC(current->ref_count); // E
                }
                ARC_ATOMIC_DEC(t->ref_count); // D, E

                delta--;
        }

        ARC_ATOMIC_DEC(parent->ref_count); // A

        if (r == 0) {
                return current;
        }
        
        return NULL;
}

ARC_GraphNode *init_base_graph(size_t arb_size) {
        ARC_GraphNode *root = graph_create(arb_size);
        root->ref_count = 100; // Make it impossible to remove the root

        return root;
}
