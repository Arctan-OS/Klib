/**
 * @file checksums.c
 *
 * @author awewsomegamer <awewsomegamer@gmail.com>
 *
 * @LICENSE
 * Arctan - Operating System Kernel
 * Copyright (C) 2023-2025 awewsomegamer
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
#include <lib/checksums.h>
#include <global.h>

#define CHECKSUMS_CRC32_MSB_POLYNOMIAL 0xB71DC104
#define CHECKSUMS_CRC32_LSB_POLYNOMIAL 0xEDB88320

static uint32_t crc32_little_endian_table[256] = { 0 };

static int checksum_gen_crc32_table() {
	uint32_t crc = 1;
	int i = 128;

	do {
		if (crc & 1) {
			crc = (crc >> 1) ^ CHECKSUMS_CRC32_LSB_POLYNOMIAL;
		} else {
			crc >>= 1;
		}

		for (int j = 0; j < 256; j += i * 2) {
			crc32_little_endian_table[i + j] = crc ^ crc32_little_endian_table[j];
		}

		i >>= 1;
	} while (i > 0);

	ARC_DEBUG(INFO, "Generated CRC32 table\n");

	return 0;
}

uint32_t checksum_crc32(uint8_t *data, size_t length) {
	if (length == 0 || data == NULL) {
		return 0;
	}

	uint32_t crc32 = (uint32_t)-1;

	for (size_t i = 0; i < length; i++) {
		uint32_t lkup = (crc32 ^ data[i]) & 0xFF;
		crc32 = (crc32 >> 8) ^ crc32_little_endian_table[lkup];
	}

	return ~crc32;
}

int init_checksums() {
	checksum_gen_crc32_table();

	ARC_DEBUG(INFO, "Initialized checksums\n");

	return 0;
}
