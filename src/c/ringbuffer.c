/**
 * @file ringbuffer.h
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan - Operating System Kernel
 * Copyright (C) 2023-2025 awewsomegamer
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
#include <lib/ringbuffer.h>
#include <lib/util.h>
#include <mm/allocator.h>
#include <global.h>

size_t ringbuffer_allocate(struct ARC_Ringbuffer *ringbuffer, int block) {
	if (ringbuffer == NULL) {
		return -1;
	}

	mutex_lock(&ringbuffer->lock);

	if (ringbuffer->idx == ringbuffer->data_tail && block) {
		while (ringbuffer->idx == ringbuffer->data_tail);
	} else if (ringbuffer->idx == ringbuffer->data_tail) {
		mutex_unlock(&ringbuffer->lock);
		return -2;
	}

	size_t idx = ringbuffer->idx++;
	mutex_unlock(&ringbuffer->lock);

	return idx;
}

int ringbuffer_free(struct ARC_Ringbuffer *ringbuffer, size_t idx) {
	if (ringbuffer == NULL || idx >= ringbuffer->obj_size) {
		return -1;
	}

	ringbuffer->data_tail = idx % ringbuffer->obj_size;

	return 0;
}

size_t ringbuffer_write(struct ARC_Ringbuffer *ringbuffer, size_t idx, void *data) {
	if (ringbuffer == NULL) {
		return -1;
	}

	ringbuffer->idx = ringbuffer->idx % ringbuffer->objs;
	idx = idx % ringbuffer->objs;

	void *obj_base = ringbuffer->base + (idx * ringbuffer->obj_size);

	if (data != NULL) {
		memcpy(obj_base, data, ringbuffer->obj_size);
	} else {
		memset(obj_base, 0, ringbuffer->obj_size);
	}

	return idx;
}

struct ARC_Ringbuffer *init_ringbuffer(void *base, size_t objs, size_t obj_size) {
	if (objs == 0 || obj_size == 0) {
		return NULL;
	}

	struct ARC_Ringbuffer *ring = (struct ARC_Ringbuffer *)alloc(sizeof(*ring));

	if (ring == NULL) {
		return NULL;
	}

	memset(ring, 0, sizeof(*ring));

	ring->base = base;
	ring->objs = objs;
	ring->obj_size = obj_size;
	ring->data_tail = -1;
	init_static_mutex(&ring->lock);

	return ring;
}
