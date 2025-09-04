/**
 * @file spinlock.c
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
#include "lib/spinlock.h"
#include "lib/util.h"
#include "mm/allocator.h"

int init_spinlock(ARC_Spinlock **spinlock) {
	if (spinlock == NULL) {
		return 1;
	}

	*spinlock = (ARC_Spinlock *)alloc(sizeof(**spinlock));

	if (*spinlock == NULL) {
		return 1;
	}

	memset(*spinlock, 0, sizeof(**spinlock));

	return 0;
}

int uninit_spinlock(ARC_Spinlock *spinlock) {
	free(spinlock);

	return 0;
}

int init_static_spinlock(ARC_Spinlock *spinlock) {
	if (spinlock == NULL) {
		return 1;
	}

	memset(spinlock, 0, sizeof(ARC_Spinlock));

	return 0;
}

int spinlock_lock(ARC_Spinlock *spinlock) {
	if (spinlock == NULL) {
		return 1;
	}

	while (__atomic_test_and_set(spinlock, __ATOMIC_ACQUIRE));

	return 0;
}

int spinlock_unlock(ARC_Spinlock *spinlock) {
	if (spinlock == NULL) {
		return 1;
	}

	__atomic_clear(spinlock, __ATOMIC_RELEASE);

	return 0;
}
