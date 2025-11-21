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
	test("./", "");
	test("/a/.", "/a/");
	test("/../", "/");
	test("/..", "/");
	test("/a/..", "/");
	test("/a/../b/c/d", "/b/c/d");
	test("/../a", "/a");
	test("../a/b/c/d", "../a/b/c/d");
	test("./a/b/c/d", "a/b/c/d");
	test("/./..//../././//../", "/");
	test("//a/b/c/../def/.//", "/a/b/def/");
	test("//a/b/c/../def/.//.", "/a/b/def/");
	test("//a/b/c/../def/.//..", "/a/b/");
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

        size_t sep0 = SIZE_MAX;
        size_t sep1 = SIZE_MAX;
        size_t sep2 = SIZE_MAX;

        if (max == 1 && path[0] == '/') {
                return path;
        }

        if (max >= 2 && path[0] == '.' && path[1] == '/') {
                memcpy(path, path + 2, max - 1);
                removed += 2;
        }

        while (path[i]) {
                if (path[i] != '/' && i + 1 < max) {
                        goto end;
                }

                sep2 = sep1;
                sep1 = sep0;
                sep0 = i + (path[i] != '/' && i + 1 == max);

                size_t to = SIZE_MAX;

                switch (sep0 - sep1) {
                        case 1: {
                                if (path[sep0] == '/') {
                                        to = sep1;
                                }

                                break;
                        }
                        case 2: {
                                if (path[sep0 - 1] == '.') {
                                        to = sep1;
                                }

                                break;
                        }
                        case 3: {
                                if (path[sep0 - 1] == '.' && path[sep0 - 2] == '.') {
                                        to = sep2 == SIZE_MAX ? sep1 : sep2;
                                }

                                break;
                        }
                }

                if (to != SIZE_MAX) {
                        to += (path[sep0] == 0);

                        memcpy(&path[to], &path[sep0], max - sep0 + 1);
                        removed += sep0 - to;
                        break;
                }

                end:;
                i++;
        }

        if (removed > 0) {
                goto redo;
        }

        char *o_path = strdup(path);
        free(path);

        return o_path;
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

        ARC_GraphNode *parent = start;
        ARC_GraphNode *current = NULL;

        size_t max = strlen(path);
        size_t i = 0;
        size_t j = SIZE_MAX;

        while (i < max) {
                if (path[i] != '/' && i + 1 < max) {
                        goto end;
                }

                if (i + 1 < max) {
                        i++;
                }

                if (j == SIZE_MAX) {
                        goto end_1;
                }

                size_t name_len = i - j - 1;
                char *name = &path[j + 1];

                if (name_len == 1 && path[j + 1] == '.') {
                        goto end_1;
                } else if (name_len == 2 && path[j + 1] == '.' && path[j + 2] == '.') {
                        ARC_GraphNode *t = ARC_ATOMIC_LOAD(current->parent);
                        ARC_ATOMIC_INC(t->ref_count);
                        ARC_ATOMIC_DEC(current->ref_count);
                        current = t;
                        goto end_1;
                }

                current = ARC_ATOMIC_LOAD(parent->child);
                while (current != NULL) {
                        if (ARC_ATOMIC_LOAD(current->next) == current) {
                                current = NULL;
                                break;
                        }

                        if (strncmp(name, current->name, name_len) == 0) {
                                break;
                        }

                        current = ARC_ATOMIC_LOAD(current->next);
                }

                if (current == NULL) {
                        break;
                }

                ARC_ATOMIC_INC(current->ref_count);
                ARC_ATOMIC_DEC(parent->ref_count);

                parent = current;

                end_1:;
                j = i;
                end:;
                i++;
        }

        return current;
}
