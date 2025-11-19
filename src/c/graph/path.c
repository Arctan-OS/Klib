/**
 * @file path.c
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
#include "lib/atomics.h"
#include "lib/graph/path.h"
#include "lib/graph/base.h"
#include "lib/util.h"
#include "mm/allocator.h"
#include <stddef.h>
#include <stdint.h>

#include "interface/printf.h"

struct arc_path_node {
        ARC_GraphNode *node;
        struct arc_path_node *next;
        size_t name_len;
};

/*
        (path_to_collapse, expected_result)
        test("/", "/");
        test("//", "/");
        test("/./", "/");
        test("/.", "/");
        test("/a/.", "/a/");
        test("/../", "/");
        test("/..", "/");
        test("/a/..", "/");
        test("/a/../b/c/d", "/b/c/d");
        test("/../a", "/a");
        test("../a/b/c/d", "../a/b/c/d");
        test("./a/b/c/d", "a/b/c/d"); (FAIL)
        test("/./..//../././//../", "/");
        test("//a/b/c/../def/.//", "/a/b/def/");
        test("//a/b/c/../def/.//.", "/a/b/def/");
        test("//a/b/c/../def/.//..", "/a/b/"); (FAIL)
*/
char *path_collapse(char *_path) {
        if (_path == NULL) {
                return NULL;
        }

        char *path = strdup(_path);

        if (path == NULL) {
                return NULL;
        }

        // Take the given path and collapse it by removing
        // patterns:
        //  * "//"
        //  * "/./"
        //  * "/../" (and prior component)
        //  * "/."
        //  * "/.." (and prior component)

        redo:;
        size_t max = strlen(path);
        size_t removed = 0;
        size_t i = 0;
        while (path[i]) {
                if (path[i] != '/') {
                        goto end;
                }

                size_t forward = 0;
                size_t backward = 0;
                if (path[i + 1] == '/') {
                        forward = 1;
                } else if (path[i + 1] == '.') {
                        forward = 2;

                        if (i + 2 < max && path[i + 2] == '.') {
                                // Calculate the length of the last component
                                // and set it as backward
                                size_t j = i - 1;
                                for (; j != SIZE_MAX; j--) {
                                        if (path[j] == '/') {
                                                backward = i - j;
                                                break;
                                        }
                                }

                                forward++;
                        }
                }

                if (forward == 2 && i + 2 >= max && backward == 0) {
                        path[i + 1] = 0;
                        removed += forward - 1;
                        goto end;
                }

                if (forward > 0) {
                        if (path[i + forward] == 0) {
                                backward--;
                        }
                        //printf("Copying %s to %s for %lu chars\n", &path[i + forward], &path[i - backward], max - (i + forward + removed) + 1);
                        memcpy(&path[i - backward], &path[i + forward], max - (i + forward + removed) + 1);
                        removed += forward + backward;
                }

                end:;
                i++;
        }

        if (removed > 0) {
                goto redo;
        }

        return path;
}

char *path_get(ARC_GraphNode *node) {
        if (node == NULL) {
                return NULL;
        }

        ARC_GraphNode *current = node;
        size_t ret_size = 1; // Zero terminator

        struct arc_path_node *n = NULL;
        while (current != NULL) {
                ARC_ATOMIC_INC(current->ref_count); // NOTE: This is to prevent potential move operations
                                                    //       (remove(node, false) + create(parent, node, "new_name"))
                struct arc_path_node *t = alloc(sizeof(*n));

                if (t == NULL) {
                        goto epic_fail;
                }

                t->node = current;
                t->name_len = strlen(current->name);
                t->next = n;
                n = t;

                ret_size += t->name_len + 1; // + '/'
        }

        char *_ret = alloc(ret_size);

        if (_ret == NULL) {
                goto epic_fail;
        }

        memset(_ret, 0, ret_size);

        size_t i = 0;
        while (n != NULL) {
                _ret[i] = '/';
                strcpy(_ret + i + 1, n->node->name);
                i += n->name_len + 1;
                ARC_ATOMIC_DEC(n->node->ref_count);
                n = n->next;
        }

        char *ret = path_collapse(_ret);
        free(_ret);

        return ret;

        epic_fail:;

        while (n != NULL) {
                ARC_ATOMIC_DEC(n->node->ref_count);
                free(n);
                n = n->next;
        }

        return NULL;
}

ARC_GraphNode *path_traverse(ARC_GraphNode *start, char *path) {
        if (path == NULL) {
                return NULL;
        }

        return NULL;
}
