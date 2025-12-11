/**
 * @file base.c
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan-OS/Kernel - Operating System Kernel
 * Copyright (C) 2023-2025 awewsomegamer
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
#include "global.h"
#include "lib/atomics.h"
#include "lib/graph/base.h"
#include "lib/util.h"
#include "mm/allocator.h"

#include <stddef.h>

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

        char *name = graph_empty_name;
        if (_name != NULL) {
                name = strdup(_name);
        } else if (node->name != NULL) {
                name = node->name;
        }

        node->name = name;

        node->parent = parent;
        ARC_ATOMIC_XCHG(&parent->child, &node, &node->next);
        ARC_ATOMIC_INC(parent->child_count);

        return 0;
}

ARC_GraphNode *graph_duplicate(ARC_GraphNode *node) {
        if (node == NULL) {
                return NULL;
        }

        ARC_GraphNode *dup = graph_create(node->arb_size);

        if (dup == NULL) {
                return NULL;
        }

        memcpy(dup, node, sizeof(*node) + node->arb_size);

        dup->name = strdup(node->name);
        dup->child = NULL;

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

int graph_remove(ARC_GraphNode *node, bool free) {
        if (node == NULL) {
                ARC_DEBUG(ERR, "No node given\n");
                return -1;
        }

        int rc = 0;
        if ((rc = ARC_ATOMIC_INC(node->ref_count)) > 1) { // NOTE: Prevents other remove operations
                ARC_DEBUG(ERR, "Node in use or already being removed %d\n", rc);
                return -2;
        }

        ARC_GraphNode *parent = node->parent;

        if (parent != NULL && ARC_ATOMIC_LOAD(parent->child) == node) {
                ARC_ATOMIC_XCHG(&parent->child, &node->next, &node->next);
                node->parent = NULL;
                // node->next = node; If not, something went wrong
        }

        ARC_ATOMIC_DEC(parent->child_count);

        if (free) {
                return graph_recursive_free(node) > 0 ? 0 : -3;
        } else {
                ARC_ATOMIC_DEC(node->ref_count);
        }

        return 0;
}

ARC_GraphNode *graph_find(ARC_GraphNode *parent, char *targ) {
        if (parent == NULL || targ == NULL) {
                return NULL;
        }

        ARC_ATOMIC_INC(parent->ref_count);
        full_recheck:;
        size_t a = ARC_ATOMIC_LOAD(parent->child_count);

        ARC_GraphNode *current = ARC_ATOMIC_LOAD(parent->child);

        if (current == NULL) {
                ARC_ATOMIC_DEC(parent->ref_count);
                return NULL;
        }
        
        ARC_GraphNode *initial = current;
        
        ARC_ATOMIC_INC(current->ref_count);

        while (current != NULL && strcmp(targ, current->name) != 0) {
                ARC_GraphNode *t = current;
                current = ARC_ATOMIC_LOAD(current->next);
                if (current == t) {
                        ARC_DEBUG(INFO, "Current == Last, full recheck needed\n");
                        goto full_recheck;
                }

                if (current != NULL) {
                        ARC_ATOMIC_INC(current->ref_count);
                }
                ARC_ATOMIC_DEC(t->ref_count);
        }

        size_t b = ARC_ATOMIC_LOAD(parent->child_count);

        if (current != NULL) {
                ARC_ATOMIC_DEC(parent->ref_count);
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
                ARC_ATOMIC_DEC(parent->ref_count);
                return NULL;
        }

        ARC_ATOMIC_INC(current->ref_count);

        int r = -1;
        while (current != NULL && (r = strcmp(targ, current->name)) != 0 && delta > 0) {
                ARC_GraphNode *t = current;
                current = ARC_ATOMIC_LOAD(current->next);

                if (current == t) {
                        goto full_recheck;
                }

                if (current != NULL) {
                        ARC_ATOMIC_INC(current->ref_count);
                }
                ARC_ATOMIC_DEC(t->ref_count);

                delta--;
        }

        ARC_ATOMIC_DEC(parent->ref_count);

        if (r == 0) {
                return current;
        }

        return NULL;
}

ARC_GraphNode *init_base_graph(size_t arb_size) {
        ARC_GraphNode *root = graph_create(arb_size);
        root->ref_count++; // Make it impossible to remove the root

        return root;
}
