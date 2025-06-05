/**
 * @file perms.c
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
#include <lib/perms.h>
#include <global.h>

int check_permissions(struct stat *stat, uint32_t requested) {
	// TODO: Look these up from some sort of list
	uint32_t UID = 0;
	uint32_t GID = 0;

	if (UID == 0) {
		// ROOT CAN DO WHATEVER!!!!
		return 0;
	}

	if (stat->st_uid == UID) {
		return (stat->st_mode ^ requested) & ((requested >> 6) & 07);
	}

	if (stat->st_gid == GID) {
		return (stat->st_mode ^ requested) & ((requested >> 3) & 07);
	}

	return (stat->st_mode ^ requested) & (requested & 07);
}
