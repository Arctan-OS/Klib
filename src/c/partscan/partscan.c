/**
 * @file partscan.c
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
#include <lib/partscan/partscan.h>
#include <lib/partscan/gpt.h>
#include <fs/vfs.h>
#include <lib/perms.h>

int partscan_enumerate_partitions(char *filepath) {
	if (filepath == NULL) {
		return -1;
	}

	if (gpt_get_partitions(filepath) == 0) {
		return ARC_PARTSCAN_TYPE_GPT;
	} // else if (...) { ... }

	return ARC_PARTSCAN_TYPE_NONE;
}
