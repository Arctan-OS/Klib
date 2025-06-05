/**
 * @file atomics.h
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
#ifndef ARC_LIB_ATOMICS_H
#define ARC_LIB_ATOMICS_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define ARC_MEM_BARRIER __asm__("" ::: "memory");
// TODO: Fences

#define ARC_ATOMIC_INC(__val) __atomic_add_fetch(&__val, 1, __ATOMIC_ACQUIRE);
#define ARC_ATOMIC_DEC(__val) __atomic_sub_fetch(&__val, 1, __ATOMIC_ACQUIRE);
#define ARC_ATOMIC_LOAD(__val) __atomic_load_n(&__val, __ATOMIC_ACQUIRE);
#define ARC_ATOMIC_STORE(__dest, __val) __atomic_store_n(&__dest, __val, __ATOMIC_ACQUIRE);
#define ARC_ATOMIC_XCHG(__mem, __val, __ret) __atomic_exchange(__mem, __val, __ret, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_CMPXCHG(__ptr, __expected, __desired) __atomic_compare_exchange_n(__ptr, __expected, __desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_LFENCE __asm__("lfence" :::);
#define ARC_ATOMIC_SFENCE __asm__("sfence" :::);
#define ARC_ATOMIC_MFENCE __asm__("mfence" :::);

/// Generic spinlock
typedef int ARC_GenericSpinlock;

/// Generic mutex
typedef int ARC_GenericMutex;

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
	ARC_GenericMutex lock;
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

int init_mutex(ARC_GenericMutex **mutex);
int uninit_mutex(ARC_GenericMutex *mutex);
int init_static_mutex(ARC_GenericMutex *mutex);
int mutex_lock(ARC_GenericMutex *mutex);
int mutex_unlock(ARC_GenericMutex *mutex);

int init_spinlock(ARC_GenericSpinlock **spinlock);
int uninit_spinlock(ARC_GenericSpinlock *spinlock);
int init_static_spinlock(ARC_GenericSpinlock *spinlock);
int spinlock_lock(ARC_GenericSpinlock *spinlock);
int spinlock_unlock(ARC_GenericSpinlock *spinlock);

#endif
