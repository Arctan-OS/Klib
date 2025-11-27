/**
 * @file path.h
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
#ifndef ARC_LIB_GRAPH_PATH_H
#define ARC_LIB_GRAPH_PATH_H

#include "lib/graph/base.h"

#include <stddef.h>
#include <stdbool.h>

typedef ARC_GraphNode *(*ARC_PathCreateCallback)(ARC_GraphNode *parent, char *name, char *remaining, void *arg);

char *path_collapse(char *_path);
char *path_get_abs(ARC_GraphNode *from, ARC_GraphNode *to);
char *path_get_rel(ARC_GraphNode *from, ARC_GraphNode *to);
ARC_GraphNode *path_traverse(ARC_GraphNode *start, char *path, ARC_PathCreateCallback, void *arg);

#endif
