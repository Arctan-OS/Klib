/**
 * @file event.h
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
#ifndef ARC_LIB_EVENT_H
#define ARC_LIB_EVENT_H

typedef struct ARC_EventElement {
        void (*handler)(void *args);
        struct ARC_EventElement *next;
} ARC_EventElement;

typedef struct ARC_Event {
        struct ARC_EventElement *current;
        struct ARC_EventElement *last;
} ARC_Event;

int event_register(ARC_Event *event, ARC_EventElement *elem);
int event_trigger(ARC_Event *event, void *args);

#endif
