/**
 * @file ringbuffer.h
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
#ifndef ARC_LIB_RINGBUFFER_H
#define ARC_LIB_RINGBUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <lib/atomics.h>

struct ARC_Ringbuffer {
	void *base; // The start of the buffer
	size_t objs; // The number of objects this buffer can fit
	size_t obj_size; // Size of each object
	size_t idx; // The current 0-based index of the next free object
	size_t data_tail;
	ARC_GenericMutex lock;
};

size_t ringbuffer_allocate(struct ARC_Ringbuffer *ringbuffer, int block);
int ringbuffer_free(struct ARC_Ringbuffer *ringbuffer, size_t idx);

size_t ringbuffer_write(struct ARC_Ringbuffer *ringbuffer, size_t idx, void *data);

struct ARC_Ringbuffer *init_ringbuffer(void *base, size_t objs, size_t obj_size);

#endif
