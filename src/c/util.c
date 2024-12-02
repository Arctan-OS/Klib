/**
 * @file util.c
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
#include <mm/allocator.h>
#include <lib/util.h>
#include <global.h>

int strcmp(char *a, char *b) {
	if (a == NULL || b == NULL) {
		return -1;
	}

	uint8_t *ua = (uint8_t *)a;
	uint8_t *ub = (uint8_t *)b;

	size_t max = strlen(a);
	size_t b_len = strlen(b);

	if (max > b_len) {
		max = b_len;
	}

	size_t i = 0;
	for (; i < max - 1; i++) {
		if (ua[i] != ub[i] || ua[i] == 0 || ub[i] == 0) {
			break;
		}
	}

	return ua[i] - ub[i];
}

int strncmp(char *a, char *b, size_t len) {
	if (a == NULL || b == NULL || len == 0) {
		return -1;
	}

	uint8_t *ua = (uint8_t *)a;
	uint8_t *ub = (uint8_t *)b;

	size_t i = 0;
	for (; i < len - 1; i++) {
		if (ua[i] != ub[i] || ua[i] == 0 || ub[i] == 0) {
			break;
		}
	}

	return (ua[i] - ub[i]);
}

void memset(void *a, uint8_t value, size_t size) {
	if (a == NULL || size == 0) {
		return;
	}

	for (size_t i = 0; i < size; i++) {
		*((uint8_t *)a + i) = value;
	}
}

void memcpy(void *a, void *b, size_t size) {
	if (a == NULL || b == NULL || size == 0) {
		return;
	}

	for (size_t i = 0; i < size; i++) {
		*(uint8_t *)(a + i) = *(uint8_t *)(b + i);
	}
}

size_t strlen(char *a) {
	if (a == NULL) {
		return 0;
	}

	size_t i = 0;

	while (*(a + i) != 0) {
		i++;
	}

	return i;
}

char *strdup(char *a) {
	if (a == NULL) {
		return NULL;
	}

	size_t len = strlen(a);

	char *b = alloc(len + 1);
	memset(b, 0, len + 1);
	memcpy(b, a, len);

	return b;
}

char *strndup(char *a, size_t n) {
	if (a == NULL || n == 0) {
		return NULL;
	}

	char *b = alloc(n + 1);
	memset(b, 0, n + 1);
	memcpy(b, a, n);

	return b;
}

static const char *NUMBERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

long strtol(char *string, char **end, int base) {
	if (string == NULL || base == 0) {
		return 0;
	}

	char c = *string;
	int offset = 0;
	long number = 0;

	while (c < *(NUMBERS + base)) {
		number *= base;

		if (c >= '0' && c <= '9') {
			number += c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			number += c- 'A';
		}

		c = *(string + offset++);
	}

	*end = (char *)(string + offset);

	return number;
}
