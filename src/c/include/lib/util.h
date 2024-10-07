/**
 * @file util.h
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan - Operating System Kernel
 * Copyright (C) 2023-2024 awewsomegamer
 *
 * This file is part of Arctan.
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
#ifndef ARC_LIB_UTIL_H
#define ARC_LIB_UTIL_H

#include <stdint.h>
#include <stddef.h>

int strcmp(char *a, char *b);
int strncmp(char *a, char *b, size_t len);

void memset(void *a, uint8_t value, size_t size);
void memcpy(void *a, void *b, size_t size);
size_t strlen(char *a);
char *strdup(char *a);
char *strndup(char *a, size_t n);
long strtol(char *string, char **end, int base);

#endif
