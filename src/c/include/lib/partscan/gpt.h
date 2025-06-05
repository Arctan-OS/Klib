/**
 * @file gpt.h
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
#ifndef ARC_LIB_PARTSCAN_GPT_H
#define ARC_LIB_PARTSCAN_GPT_H

#include <fs/vfs.h>

struct ARC_GPTHeader {
	uint64_t sig;
	uint32_t rev;
	uint32_t size;
	uint32_t crc32;
	uint32_t resv0;
	uint64_t current_lba;
	uint64_t backup_lba;
	uint64_t first_usable;
	uint64_t last_usable;
	struct {
		uint64_t low;
		uint64_t high;
	}__attribute__((packed)) disk_guid;
	uint64_t entries_start;
	uint32_t entry_count;
	uint32_t entry_size;
	uint32_t entry_crc32;
}__attribute__((packed));

struct ARC_GPTEntry {
	struct {
		uint64_t low;
		uint64_t high;
	}__attribute__((packed)) part_type_guid;
	struct {
		uint64_t low;
		uint64_t high;
	}__attribute__((packed)) part_guid;
	uint64_t first_lba;
	uint64_t last_lba; // Inclusive
	uint64_t attrs;
	uint16_t name[36]; // UTF-16LE
};

int gpt_get_partitions(char *filepath);

#endif
