/**
 * @file event.c
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
#include "lib/event.h"
#include "util.h"

ARC_EventElement terminator = { 0 };

int event_register(ARC_Event *event, ARC_EventElement *elem) {
        if (event == NULL || elem == NULL) {
		return 1;
	}

	// Save interrupt flag

	ARC_DISABLE_INTERRUPT;
	ARC_EventElement *_elem = elem;
	ARC_EventElement *t = NULL;
	ARC_ATOMIC_XCHG(&event->last, &_elem, &t);
	if (t != NULL) {
		t->next = elem;
	} else {
		event->current = elem;
	}

	// Restore interrupt flag

        return 0;
}

int event_trigger(ARC_Event *event, void *args) {
	event_register(event, &terminator);

	while (event->current != NULL && event->current != &terminator) {
		void (*func)(void *args) = event->current->handler;

		if (func != NULL) {
			func(args);
		}
	}

	if (event->current == &terminator) {
		event->current = event->current->next;
	}

        return 0;
}
