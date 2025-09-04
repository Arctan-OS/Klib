/**
 * @file ticket.c
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
#include "global.h"
#include "lib/ticket.h"
#include "lib/util.h"
#include "mm/allocator.h"
#include "mp/scheduler.h"

struct internal_ticket_lock_node {
	uint64_t tid;
	uint64_t ticket;
	struct internal_ticket_lock_node *next;
	struct ARC_TicketLock *parent;
};

int init_ticket_lock(struct ARC_TicketLock **lock) {
	*lock = alloc(sizeof(struct ARC_TicketLock));

	if (*lock == NULL) {
		return 1;
	}

	// NOTE: This memset initializes the static mutex
	//       as well, therefore an explicit call to
	//       init_static_mutex is not needed
	memset(*lock, 0, sizeof(struct ARC_TicketLock));

	return 0;
}

int uninit_ticket_lock(struct ARC_TicketLock *lock) {
	if (lock == NULL) {
		return 1;
	}

	free(lock);

	return 0;
}

int init_static_ticket_lock(struct ARC_TicketLock *head) {
	memset(head, 0, sizeof(struct ARC_TicketLock));

	return 0;
}

void *ticket_lock(struct ARC_TicketLock *head) {
	if (head == NULL) {
		return NULL;
	}

	if (head->is_frozen) {
		return NULL;
	}

	struct internal_ticket_lock_node *ticket = (struct internal_ticket_lock_node *)alloc(sizeof(*ticket));

	if (ticket == NULL) {
		return NULL;
	}

	memset(ticket, 0, sizeof(*ticket));

	mutex_lock(&head->lock);

	struct internal_ticket_lock_node *last = (struct internal_ticket_lock_node *)head->last;

	ticket->ticket = head->next_ticket++;
	ticket->parent = head;
	ticket->tid = sched_get_current_tid();
	ticket->next = NULL;

	if (last != NULL) {
		last->next = ticket;
	} else {
		head->next = ticket;
	}

	head->last = ticket;

	mutex_unlock(&head->lock);

	return (void *)ticket;
}

void ticket_lock_yield(void *ticket) {
	if (ticket == NULL) {
		ARC_DEBUG(ERR, "Ticket is NULL\n");
		// TODO: What to do?
	}

	struct internal_ticket_lock_node *wait_for = (struct internal_ticket_lock_node *)ticket;
	struct internal_ticket_lock_node *current = (struct internal_ticket_lock_node *)wait_for->parent->next;

	if (current->ticket == wait_for->ticket) {
		return;
	}

	while (current->ticket != wait_for->ticket) {
		// sched_yield_cpu(current->tid);
		current = (struct internal_ticket_lock_node *)wait_for->parent->next;
	}
}

void *ticket_unlock(void *ticket) {
	if (ticket == NULL) {
		return NULL;
	}

	struct internal_ticket_lock_node *lock = (struct internal_ticket_lock_node *)ticket;
	struct ARC_TicketLock *head = lock->parent;

	if (head->next != lock) {
		return NULL;
	}

	mutex_lock(&head->lock);

	head->next = lock->next;

	if (head->next == NULL) {
		head->next_ticket = 0;
		head->last = NULL;
	}

	free(lock);

	mutex_unlock(&head->lock);

	return ticket;
}

int ticket_lock_freeze(void *ticket) {
	if (ticket == NULL) {
		return -1;
	}

	struct internal_ticket_lock_node *lock = (struct internal_ticket_lock_node *)ticket;
	struct ARC_TicketLock *head = lock->parent;

	if (head->is_frozen == 1) {
		ARC_DEBUG(ERR, "Lock is already frozen!\n");
		return -1;
	}

	head->is_frozen = 1;

	head->next = lock->next;

	while (head->next != NULL) {
		__asm__("pause");
	}

	head->next = lock;

	return 0;
}

int ticket_lock_thaw(void *ticket) {
	if (ticket == NULL ) {
		return -1;
	}

	struct internal_ticket_lock_node *lock = (struct internal_ticket_lock_node *)ticket;
	struct ARC_TicketLock *head = lock->parent;

	// Thaw a frozen lock
	if (head->is_frozen == 0) {
		return 0;
	}

	if (lock->parent->next != lock) {
		return -1;
	}

	head->is_frozen = 0;

	return 0;
}
