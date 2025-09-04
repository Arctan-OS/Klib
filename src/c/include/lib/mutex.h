/**
 * @file mutex.h
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
#ifndef ARC_LIB_MUTEX_H
#define ARC_LIB_MUTEX_H

#include "userspace/thread.h"
#include <stdint.h>

// NOTE: This struct being packed allows for the lock
//       to be placed prior to the thread to wake letting
//       the lock be easily accessed
typedef struct ARC_Mutex {
        uint64_t lock;
        ARC_Thread *wake;
} __attribute__((packed)) ARC_Mutex;

typedef struct ARC_ListMutexElement {
        struct ARC_ListMutexElement *next;
        ARC_Thread *wake;
} ARC_ListMutexElement;

typedef struct ARC_ListMutex {
        ARC_ListMutexElement *current;
        ARC_ListMutexElement *last;
} ARC_ListMutex;

int init_mutex(ARC_Mutex **mutex);
int uninit_mutex(ARC_Mutex *mutex);
int init_static_mutex(ARC_Mutex *mutex);
int mutex_lock(ARC_Mutex *mutex);
int mutex_unlock(ARC_Mutex *mutex);

int init_list_mutex(ARC_ListMutex **mutex);
int uninit_list_mutex(ARC_ListMutex *mutex);
int init_static_list_mutex(ARC_ListMutex *mutex);
int list_mutex_lock(ARC_ListMutex *mutex, ARC_ListMutexElement *elem);
int list_mutex_unlock(ARC_ListMutex *mutex);

#endif
