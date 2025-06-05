/**
 * @file gpt.c
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
#include <lib/partscan/gpt.h>
#include <abi-bits/seek-whence.h>
#include <fs/vfs.h>
#include <lib/perms.h>
#include <lib/checksums.h>
#include <mm/allocator.h>
#include <drivers/dri_defs.h>
#include <drivers/sysdev/partition_dummy.h>
#include <global.h>

#define GPT_HEADER_SIG 0x5452415020494645ULL // Little endian

static int gpt_check_for_gpt(struct ARC_GPTHeader *header) {
	if (header == NULL) {
		ARC_DEBUG(ERR, "Cannot check for GPT scheme, header is NULL\n");
		return -1;
	}

	if (header->sig != GPT_HEADER_SIG) {
		ARC_DEBUG(ERR, "Signature mismatch\n");
		return -2;
	}

	uint32_t given_crc32 = header->crc32;
	header->crc32 = 0;

	if (checksum_crc32((uint8_t *)header, sizeof(*header)) != given_crc32) {
		ARC_DEBUG(ERR, "CRC32 mismatch\n")
		return -3;
	}

	ARC_DEBUG(INFO, "Found valid GPT header\n");

	return 0;
}

int gpt_get_partitions(char *filepath) {
	if (filepath == NULL) {
		ARC_DEBUG(ERR, "Give filepath is NULL\n");
		return -1;
	}

	struct ARC_File *file = NULL;
	vfs_open(filepath, 0, ARC_STD_PERM, &file);

	if (file == NULL) {
		ARC_DEBUG(INFO, "Failed to open the file\n");
		return -2;
	}

	struct stat stat = { 0 };
	vfs_stat(filepath, &stat);

	// Get primary header
	struct ARC_GPTHeader header = { 0 };
	int offset = SEEK_SET;
	vfs_seek(file, stat.st_blksize, offset);
	if (vfs_read(&header, 1, sizeof(header), file) != sizeof(header)) {
		ARC_DEBUG(ERR, "Failed to read header\n");
		return -3;
	}

	if (gpt_check_for_gpt(&header) != 0) {
		// Try secondary header
		ARC_DEBUG(INFO, "Primary header may be corrupt, trying secondary\n");
		offset = SEEK_END;
		vfs_seek(file, stat.st_blksize, offset);
		if (vfs_read(&header, 1, sizeof(header), file) != sizeof(header)) {
			ARC_DEBUG(ERR, "Failed to read header\n");
			return -4;
		}
	} else {
		goto skip_second;
	}

	if (gpt_check_for_gpt(&header) != 0) {
		ARC_DEBUG(ERR, "Invalid header\n");
		return -5;
	}

	skip_second:;

	size_t entries_size = header.entry_size * header.entry_count;
	struct ARC_GPTEntry *entries = (struct ARC_GPTEntry *)alloc(entries_size);

	if (entries == NULL) {
		ARC_DEBUG(ERR, "Failed to allocate memory for entries\n");
		return -6;
	}

	vfs_seek(file, stat.st_blksize * header.entries_start, offset);
	if (vfs_read(entries, 1, entries_size, file) != entries_size) {
		ARC_DEBUG(ERR, "Failed to read entries\n");
		free(entries);
		return -7;
	}

	if (checksum_crc32((uint8_t *)entries, entries_size) != header.entry_crc32) {
		ARC_DEBUG(ERR, "CRC32 mismatch on entries\n");
		free(entries);
		return -8;
	}

	struct ARC_DriArgs_ParitionDummy dri_args = { .drive_path = filepath };

	for (uint32_t i = 0; i < header.entry_count; i++) {
		struct ARC_GPTEntry entry = entries[i];

		size_t length_in_lbas = entry.last_lba - entry.first_lba;

		if (length_in_lbas == 0) {
			continue;
		}

		dri_args.attrs = entry.attrs;
		dri_args.lba_start = entry.first_lba;
		dri_args.lba_size = stat.st_blksize;
		dri_args.size_in_lbas = length_in_lbas;
		dri_args.partition_number = i;

		init_resource(ARC_DRIDEF_PARTITION_DUMMY, &dri_args);
	}

	return 0;
}
