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
#include <lib/ringbuffer.h>
#include <lib/util.h>
#include <mm/allocator.h>
#include <global.h>

size_t ringbuffer_push(struct ARC_Ringbuffer *ringbuffer, void *data, int block) {
	if (ringbuffer == NULL || data == NULL) {
		return -1;
	}

	mutex_lock(&ringbuffer->lock);

	while (block && ringbuffer->idx == ringbuffer->data_tail) {
		// TODO: Pause
	}

	size_t next = ringbuffer->idx++;
	mutex_unlock(&ringbuffer->lock);

	ringbuffer->idx = ringbuffer->idx % ringbuffer->objs;
	next = next % ringbuffer->objs;

	void *obj_base = ringbuffer->base + (next * ringbuffer->obj_size);
	memcpy(obj_base, data, ringbuffer->obj_size);

	return next;
}

int ringbuffer_pop(struct ARC_Ringbuffer *ringbuffer, size_t idx) {
	if (ringbuffer == NULL || abs(idx) >= ringbuffer->objs) {
		return -1;
	}

	size_t real_idx = idx;

	if (idx < 0) {
		real_idx = ringbuffer->objs - idx;
	}

	ringbuffer->data_tail = real_idx;

	return 0;
}

struct ARC_Ringbuffer *init_ringbuffer(void *base, size_t objects, size_t obj_size) {
	if (objects == 0 || obj_size == 0) {
		return NULL;
	}

	struct ARC_Ringbuffer *ring = (struct ARC_Ringbuffer *)alloc(sizeof(*ring));

	if (ring == NULL) {
		return NULL;
	}

	memset(ring, 0, sizeof(*ring));

	ring->base = base;
	ring->objs = objects;
	ring->obj_size = obj_size;
	init_static_mutex(&ring->lock);

	return ring;
}
