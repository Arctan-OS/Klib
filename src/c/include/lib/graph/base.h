/**
 * @file base.h
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
#ifndef ARC_LIB_GRAPH_BASE_H
#define ARC_LIB_GRAPH_BASE_H

#include <stdint.h>

typedef struct ARC_GraphNode {
        struct ARC_GraphNode *next;
        struct ARC_GraphNode *child;
        struct ARC_GraphNode *parent;
        char *name;
        uint8_t arb[];
} ARC_GraphNode;

int graph_add(ARC_GraphNode *parent, ARC_GraphNode *node);
int graph_remove(ARC_GraphNode *node);

ARC_GraphNode *init_any_graph();

#endif
