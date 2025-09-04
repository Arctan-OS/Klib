/**
 * @file mutex.c
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
#include "lib/mutex.h"
#include "util.h"
#include "lib/atomics.h"
#include "lib/util.h"
#include "mm/allocator.h"
#include "mp/scheduler.h"

int init_mutex(ARC_Mutex **mutex) {
	if (mutex == NULL) {
		return 1;
	}

	*mutex = (ARC_Mutex *)alloc(sizeof(**mutex));

	if (*mutex == NULL) {
		return 1;
	}

	memset(*mutex, 0, sizeof(**mutex));

	return 0;
}

int uninit_mutex(ARC_Mutex *mutex) {
	free(mutex);

	return 0;
}

int init_static_mutex(ARC_Mutex *mutex) {
	if (mutex == NULL) {
		return 1;
	}

	memset(mutex, 0, sizeof(*mutex));

	return 0;
}

int mutex_lock(ARC_Mutex *mutex) {
	if (mutex == NULL) {
		return 1;
	}

	while (__atomic_test_and_set(mutex, __ATOMIC_ACQUIRE)) {
		// TODO: Would be good to do a bit of a hybrid so that
		//       the core spins a little to see if it can lock it
		//       before yielding to the thread that currently owns
		//       the mutex
		sched_yield_to(mutex->wake);
	}

	mutex->wake = sched_get_current_thread();

	return 0;
}

int mutex_unlock(ARC_Mutex *mutex) {
	if (mutex == NULL) {
		return 1;
	}

	__atomic_clear(mutex, __ATOMIC_RELEASE);

	return 0;
}

// List

int init_list_mutex(ARC_ListMutex **mutex) {
	if (mutex == NULL) {
		return 1;
	}

	*mutex = (ARC_ListMutex *)alloc(sizeof(**mutex));

	if (*mutex == NULL) {
		return 1;
	}

	memset(*mutex, 0, sizeof(**mutex));

	return 0;
}

int uninit_list_mutex(ARC_ListMutex *mutex) {
	free(mutex);

	return 0;
}

int init_static_list_mutex(ARC_ListMutex *mutex) {
	if (mutex == NULL) {
		return 1;
	}

	memset(mutex, 0, sizeof(*mutex));

	return 0;
}

int list_mutex_lock(ARC_ListMutex *mutex, ARC_ListMutexElement *elem) {
	if (mutex == NULL || elem == NULL) {
		return 1;
	}

	// Save interrupt flag

	ARC_DISABLE_INTERRUPT;
	elem->wake = sched_get_current_thread();
	ARC_ListMutexElement *_elem = elem;
	ARC_ListMutexElement *t = NULL;
	ARC_ATOMIC_XCHG(&mutex->last, &_elem, &t);
	if (t != NULL) {
		t->next = elem;
	} else {
		mutex->current = elem;
	}
	ARC_ENABLE_INTERRUPT;

	while (mutex->current != elem) {
		if (mutex->current == NULL) {
			return -1;
		}

		sched_yield_to(mutex->current->wake);
	}

	// Return interrupt flag to original state

	return 0;
}

int list_mutex_unlock(ARC_ListMutex *mutex) {
	if (mutex == NULL) {
		return 1;
	}

	// Save interrupt flag
	ARC_DISABLE_INTERRUPT;
	if (mutex->current != NULL) {
		mutex->current = mutex->current->next;
	}

	// Return interrupt flag to original state

	return 0;
}
