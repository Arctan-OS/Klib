/**
 * @file resource.c
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
#include <abi-bits/errno.h>
#include <mm/allocator.h>
#include <lib/resource.h>
#include <global.h>
#include <lib/util.h>
#include <lib/atomics.h>
#include <lib/perms.h>
#include <drivers/dri_defs.h>

/*
** Driver Groups:
**  Group Number | Description
**  0            | Base filesystem drivers.
**  1            | User filesystem drivers.
**  2            | User device drivers.
**  3            | Base device drivers.
*/

uint64_t current_id = 0;

struct ARC_Resource *init_resource(uint64_t dri_index, void *args) {
	if (dri_index >= ARC_DRIDEF_COUNT) {
		ARC_DEBUG(ERR, "Invalid driver index (0x%"PRIx64")\n", dri_index);
		return NULL;
	}

	struct ARC_Resource *resource = (struct ARC_Resource *)alloc(sizeof(struct ARC_Resource));

	if (resource == NULL) {
		ARC_DEBUG(ERR, "Failed to allocate memory for resource\n");
		return NULL;
	}

	memset(resource, 0, sizeof(struct ARC_Resource));

	ARC_DEBUG(INFO, "Initializing resource %lu (Index: %lu)\n", current_id, dri_index);

	// Initialize resource properties
	resource->id = ARC_ATOMIC_LOAD(current_id);
	ARC_ATOMIC_INC(current_id);

	resource->dri_index = dri_index;

	// Fetch and set the appropriate definition
	struct ARC_DriverDef *def = __DRIVER_LOOKUP_TABLE[dri_index];
	resource->driver = def;

	if (def == NULL) {
		free(resource);
		ARC_DEBUG(ERR, "No driver definition found\n");

		return NULL;
	}

	if (def->init == NULL) {
		free(resource);
		ARC_DEBUG(ERR, "Driver does not define an initialization function\n");

		return NULL;
	}

	int ret = def->init(resource, args);

	if (ret != 0) {
		free(resource);
		ARC_DEBUG(ERR, "Driver init function returned %d\n", ret);

		return NULL;
	}

	return resource;
}

uint64_t get_dri_def_pci(uint16_t vendor, uint16_t device) {
	uint32_t target = (vendor << 16) | device;

	for (uint64_t i = 0; i < ARC_DRIDEF_COUNT; i++) {
		struct ARC_DriverDef *def = __DRIVER_LOOKUP_TABLE[i];

		if (def->pci_codes == NULL) {
			continue;
		}

		for (uint32_t code = 0;; code++) {
			if (def->pci_codes[code] == ARC_DRIDEF_PCI_TERMINATOR) {
				break;
			} else if (def->pci_codes[code] == target) {
				return i;
			}
		}
	}

	return (uint64_t)-1;
}

struct ARC_Resource *init_pci_resource(uint16_t vendor, uint16_t device, void *args) {
	if (vendor == 0xFFFF && device == 0xFFFF) {
		ARC_DEBUG(WARN, "Skipping PCI resource initialization\n");
		return NULL;
	}

	ARC_DEBUG(INFO, "Initializing PCI resource 0x%04x:0x%04x\n", vendor, device);

	return init_resource(get_dri_def_pci(vendor, device), args);
}

int uninit_resource(struct ARC_Resource *resource) {
	if (resource == NULL) {
		ARC_DEBUG(ERR, "Resource is NULL, cannot uninitialize\n");
		return 1;
	}

	ARC_DEBUG(INFO, "Uninitializing resource %lu\n", resource->id);

	resource->driver->uninit(resource);

	free(resource);

	return 0;
}
