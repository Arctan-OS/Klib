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
#include "global.h"
#include "lib/atomics.h"
#include "lib/graph/path.h"
#include "lib/graph/base.h"
#include "lib/util.h"
#include "mm/allocator.h"
#include <stddef.h>
#include <stdint.h>

#include "interface/printf.h"

struct arc_path_node {
        struct arc_path_node *next;
        ARC_GraphNode *node;
        size_t len;
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

char *path_get_abs(ARC_GraphNode *from, ARC_GraphNode *to) {
        if (from == NULL) {
                return NULL;
        }

        if (from == to) {
                char *ret = alloc(1);
                if (ret == NULL) {
                        return NULL;
                }
                *ret = 0;
                return ret;
        }

        ARC_GraphNode *current = from;
        size_t ret_size = 0;

        struct arc_path_node *n = NULL;
        while (current != NULL && current != to) {
                ARC_GraphNode *parent = ARC_ATOMIC_LOAD(current->parent);
                ARC_ATOMIC_INC(current->ref_count); // NOTE: This is to prevent potential move operations
                                                    //       (remove(node, false) + create(parent, node, "new_name"))

                struct arc_path_node *t = alloc(sizeof(*n));
                if (t == NULL) {
                        ARC_DEBUG(ERR, "Failed to allocate new node\n");
                        goto epic_fail;
                }

                t->node = current;
                t->len = strlen(current->name);
                t->next = n;
                n = t;

                ret_size += t->len + 1; // + '/'
                current = parent;
        }

        char *ret = alloc(ret_size);

        if (ret == NULL) {
                goto epic_fail;
        }

        memset(ret, 0, ret_size);

        size_t i = 0;
        while (n != NULL) {
                strcpy(ret + i, n->node->name);
                i += n->len;
                ret[i++] = '/';

                ARC_ATOMIC_DEC(n->node->ref_count);

                void *t = n;
                n = n->next;
                free(t);
        }

        ret[ret_size - 1] = 0;

        return ret;

        epic_fail:;

        while (n != NULL) {
                ARC_ATOMIC_DEC(n->node->ref_count);
                free(n);
                n = n->next;
        }

        return NULL;
}

char *path_get_rel(ARC_GraphNode *_from, ARC_GraphNode *_to) {
        if (_from == NULL || _to == NULL) {
                return NULL;
        }

        char *to = path_get_abs(_to, NULL);
        if (to == NULL) {
                return NULL;
        }

        char *from = path_get_abs(_from, NULL);
        if (from == NULL) {
                free(to);
                return NULL;
        }

        size_t max = min(strlen(from), strlen(to));
        size_t delta = 0;
        for (size_t i = 0; i < max && from[i] == to[i]; i++) {
                if (from[i] == '/') {
                        delta = i;
                }
        }

        //             + +
        // A: a/b/c/d/e/f/g.txt
        // B: a/b/c/d/x.txt
        //            ^

        int dot_dots = 0;
        for (size_t i = strlen(from) - 1; i > delta; i--) {
                if (from[i] == '/') {
                        dot_dots++;
                }
        }

        size_t fin_size = (dot_dots * 3) + strlen(to + delta);

        char *path = (char *)alloc(fin_size + 1);
        memset(path, 0, fin_size + 1);

        for (int i = 0; i < dot_dots; i++) {
                sprintf(path + (i * 3), "../");
        }
        sprintf(path + (dot_dots * 3), "%s", to + delta + 1);

        free(from);
        free(to);

        return path;
}


ARC_GraphNode *path_traverse(ARC_GraphNode *start, char *path, ARC_PathCreateCallback callback, void *arg) {
        if (start == NULL || path == NULL) {
                return NULL;
        }

//        ARC_DEBUG(INFO, "Traversing %s from %p (%p %p)\n", path, start, callback, arg);

        ARC_GraphNode *parent = start;
        ARC_GraphNode *current = start;

        size_t max = strlen(path);
        size_t i = 0;
        size_t j = SIZE_MAX;

//        ARC_DEBUG(INFO, "%p %p %lu %lu %lu\n", parent, current, max, i, j);

        while (i < max) {
                if (path[i] != '/' && i + 1 < max) {
                        goto end;
                }

                if (i + 1 == max) {
                        i++;
                }

                if (j == SIZE_MAX) {
                        goto end_1;
                }

                size_t name_len = i - j - 1;
                char *name = &path[j + 1];

//                ARC_DEBUG(INFO, "Found component name: %.*s\n", name_len, name);

                if (name_len == 1 && path[j] == '.') {
//                        ARC_DEBUG(INFO, "Is dot\n");
                        goto end_1;
                } else if (name_len == 2 && parent != NULL && path[j] == '.' && path[j + 1] == '.') {
//                        ARC_DEBUG(INFO, "Is dot dot, going up\n");
                        ARC_GraphNode *t = ARC_ATOMIC_LOAD(parent->parent);
                        ARC_ATOMIC_INC(t->ref_count);
                        ARC_ATOMIC_DEC(current->ref_count);
                        current = parent;
                        parent = t;
                        goto end_1;
                }

//                ARC_DEBUG(INFO, "Not dot (dot) dirs (parent=%p)\n", parent);
                current = ARC_ATOMIC_LOAD(parent->child);
                while (current != NULL) {
                        ARC_GraphNode *next = ARC_ATOMIC_LOAD(current->next);
                        if (next == current) {
//                                ARC_DEBUG(ERR, "Cut off from next component due to remove\n");
                                current = NULL;
                                break;
                        }

                        if (strncmp(name, current->name, name_len) == 0) {
//                                ARC_DEBUG(INFO, "Found a match with %p (%s)\n", current, current->name);
                                break;
                        }

                        current = next;
                }

                if (current == NULL && callback != NULL) {
//                        ARC_DEBUG(INFO, "Node does not exist, trying callback\n");
                        char *_name = strndup(name, name_len);
                        current = callback(parent, _name, &path[i], arg);
                        if (graph_add(parent, current, _name) != 0) {
//                                ARC_DEBUG(ERR, "Node could not be added with name %s", _name);
                                break;
                        }
                        free(_name);
                } else if (current == NULL) {
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
