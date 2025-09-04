/**
 * @file ticket.h
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
#ifndef ARC_LIB_TICKET_H
#define ARC_LIB_TICKET_H

#include <stdint.h>
#include <stdbool.h>

#include "lib/mutex.h"

/**
 * Queue lock structure
 * */
struct ARC_TicketLock {
	/// Pointer to the current owner of the lock
	void *next;
	/// Pointer to the last element in the queue
	void *last;
	/// Next ticket
	uint64_t next_ticket;
	/// Synchronization lock for the queue
	ARC_Mutex lock;
	bool is_frozen;
};

/**
 * Initialize dynamic ticket lock.
 *
 * Allocate and zero a lock, return it in the
 * given doubly pointer.
 * */
int init_ticket_lock(struct ARC_TicketLock **lock);

/**
 * Uninitialize dynamic ticket lock.
 *
 * Deallocate the given lock.
 * */
int uninit_ticket_lock(struct ARC_TicketLock *lock);

/**
 * Initialize static ticket lock.
 * */
int init_static_ticket_lock(struct ARC_TicketLock *head);

/**
 * Enqueue calling thread.
 *
 * Enqueue the calling thread into the provided lock.
 *
 * @struct ARC_TicketLock *lock - The lock into which the calling thread should be enqueued.
 * @return the ticket.
 * to enqueue thread.
 * */
void *ticket_lock(struct ARC_TicketLock *lock);

/**
 * Yield current thread to lock owner thread.
 *
 * If the provided ticket is not the one which currently
 * owns the lock, then yield to the thread of that ticket.
 * @param void *ticket - The ticket to wait for.
 * */
void ticket_lock_yield(void *ticket);

/**
 * Dequeue current lock owner.
 *
 * Dequeues the current lock owner, which should
 * be the caller.
 *
 * @param void *ticket - The ticket given by ticket_lock()
 * @return the address of the ticket upon success, NULL upon failure.
 * */
void *ticket_unlock(void *ticket);

int ticket_lock_freeze(void *ticket);
int ticket_lock_thaw(void *ticket);

#endif
