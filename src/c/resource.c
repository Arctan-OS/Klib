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

extern struct ARC_DriverDef __DRIVERS0_START[];
extern struct ARC_DriverDef __DRIVERS1_START[];
extern struct ARC_DriverDef __DRIVERS2_START[];
extern struct ARC_DriverDef __DRIVERS3_START[];
extern struct ARC_DriverDef __DRIVERS0_END[];
extern struct ARC_DriverDef __DRIVERS1_END[];
extern struct ARC_DriverDef __DRIVERS2_END[];
extern struct ARC_DriverDef __DRIVERS3_END[];

uint64_t current_id = 0;

// TODO: This can most definitely be optimzied
static struct ARC_DriverDef *get_dri_def(int group, uint64_t index) {
	struct ARC_DriverDef *start = NULL;
	struct ARC_DriverDef *end = NULL;

	switch (group) {
		case 0: {
			start = __DRIVERS0_START;
			end = __DRIVERS0_END;
			break;
		}

		case 1: {
			start = __DRIVERS1_START;
			end = __DRIVERS1_END;
			break;
		}

		case 2: {
			start = __DRIVERS2_START;
			end = __DRIVERS2_END;
			break;
		}

		case 3: {
			start = __DRIVERS3_START;
			end = __DRIVERS3_END;
			break;
		}
	}

	if (start == NULL || end == NULL) {
		return NULL;
	}

	for (struct ARC_DriverDef *def = start; def < end; def++) {
		if (def->index == index) {
			return def;
		}
	}

	return NULL;
}

struct ARC_Resource *init_resource(int dri_group, uint64_t dri_index, void *args) {
	struct ARC_Resource *resource = (struct ARC_Resource *)alloc(sizeof(struct ARC_Resource));

	if (resource == NULL) {
		ARC_DEBUG(ERR, "Failed to allocate memory for resource\n");
		return NULL;
	}

	memset(resource, 0, sizeof(struct ARC_Resource));

	ARC_DEBUG(INFO, "Initializing resource %lu (%d, %lu)\n", current_id, dri_group, dri_index);

	// Initialize resource properties
	resource->id = ARC_ATOMIC_LOAD(current_id);
	ARC_ATOMIC_INC(current_id);

	resource->dri_group = dri_group;
	resource->dri_index = dri_index;
	init_static_mutex(&resource->dri_state_mutex);

	// Fetch and set the appropriate definition
	struct ARC_DriverDef *def = get_dri_def(dri_group, dri_index);
	resource->driver = def;

	resource->instance = ARC_ATOMIC_LOAD(def->instance_counter);
	ARC_ATOMIC_INC(def->instance_counter);

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
		ARC_DEBUG(ERR, "Driver init function returned %d\n", ret);
	}

	return resource;
}

// TODO: There is probably a better way to earch drivers than this
static struct ARC_DriverDef *get_dri_def_pci(uint16_t vendor, uint16_t device, int *group) {
	uint32_t target = (vendor << 16) | device;
	struct ARC_DriverDef *start = __DRIVERS2_START;
	struct ARC_DriverDef *end = __DRIVERS2_START;

	*group = 2;

	for (struct ARC_DriverDef *def = start; def < end; def++) {
		if (def->pci_codes == NULL) {
			continue;
		}

		for (uint32_t code = 0;; code++) {
			if (def->pci_codes[code] == ARC_DRI_PCI_TERMINATOR) {
				break;
			} else if (def->pci_codes[code] == target) {
				return def;
			}
		}
	}

	start = __DRIVERS3_START;
	end = __DRIVERS3_END;

	*group = 3;

	for (struct ARC_DriverDef *def = start; def < end; def++) {
		if (def->pci_codes == NULL) {
			continue;
		}

		for (uint32_t code = 0;; code++) {
			if (def->pci_codes[code] == ARC_DRI_PCI_TERMINATOR) {
				break;
			} else if (def->pci_codes[code] == target) {
				return def;
			}
		}
	}

	return NULL;
}

struct ARC_Resource *init_pci_resource(uint16_t vendor, uint16_t device, void *args) {
	if (vendor == 0xFFFF && device == 0xFFFF) {
		ARC_DEBUG(WARN, "Skipping PCI resource initialization\n");
		return NULL;
	}

	ARC_DEBUG(INFO, "Initializing PCI resource 0x%04x:0x%04x\n", vendor, device);

	int group = 0;
	struct ARC_DriverDef *def = get_dri_def_pci(vendor, device, &group);

	if (def == NULL) {
		ARC_DEBUG(ERR, "No driver definition found\n");
		return NULL;
	}

	return init_resource(group, def->index, args);
}

int uninit_resource(struct ARC_Resource *resource) {
	if (resource == NULL) {
		ARC_DEBUG(ERR, "Resource is NULL, cannot uninitialize\n");
		return 1;
	}

	if (resource->ref_count > 1) {
		ARC_DEBUG(ERR, "Resource %lu is in use! (%lu > 1)\n", resource->id, resource->ref_count);
		return 2;
	}

	ARC_DEBUG(INFO, "Uninitializing resource %lu\n", resource->id);

	// Close all references
	struct ARC_Reference *current_ref = resource->references;
	while (current_ref != NULL) {
		void *next = current_ref->next;

		int sigret = 0;
		if (current_ref->signal != NULL && (sigret = current_ref->signal(ARC_SIGREF_CLOSE, NULL)) == 0) {
			ARC_ATOMIC_DEC(resource->ref_count);
			free(current_ref);
		} else {
			ARC_DEBUG(ERR, "Cannot signal a close to reference %p (%d)\n", current_ref, sigret);
			return 1;
		}

		current_ref = next;
	}

	resource->driver->uninit(resource);

	free(resource);

	return 0;
}

struct ARC_Reference *reference_resource(struct ARC_Resource *resource) {
	if (resource == NULL) {
		ARC_DEBUG(ERR, "Resource is NULL, cannot reference\n");
		return NULL;
	}

	struct ARC_Reference *ref = (struct ARC_Reference *)alloc(sizeof(struct ARC_Reference));

	if (ref == NULL) {
		ARC_DEBUG(ERR, "Failed to allocate reference\n");
		return NULL;
	}

	memset(ref, 0, sizeof(struct ARC_Reference));

	// Set properties of resource
	ref->resource = resource;
	ARC_ATOMIC_INC(resource->ref_count);
	// Insert reference
	if (resource->references != NULL) {
		mutex_lock(&resource->references->branch_mutex);
	}

	ref->next = resource->references;
	if (resource->references != NULL) {
		resource->references->prev = ref;
	}
	resource->references = ref;

	if (ref->next != NULL) {
		mutex_unlock(&ref->next->branch_mutex);
	}

	return ref;
}

int unrefrence_resource(struct ARC_Reference *reference) {
	if (reference == NULL || reference->resource == NULL) {
		ARC_DEBUG(ERR, "Resource is NULL, cannot unreference\n");
		return EINVAL;
	}

	struct ARC_Resource *res = reference->resource;
	struct ARC_Reference *next = reference->next;
	struct ARC_Reference *prev = reference->prev;

	// Lock effected nodes
        mutex_lock(&reference->branch_mutex);
	if (prev != NULL) {
		mutex_lock(&prev->branch_mutex);
	}
	if (next != NULL) {
		mutex_lock(&next->branch_mutex);
	}

	ARC_ATOMIC_DEC(res->ref_count);

	// Update links
	if (prev == NULL) {
		res->references = next;
	} else {
		prev->next = next;
	}

	if (next != NULL) {
		next->prev = prev;
	}

	// Unlock
	mutex_unlock(&reference->branch_mutex);
	if (reference->prev != NULL) {
		mutex_unlock(&reference->prev->branch_mutex);
	}
	if (reference->next != NULL) {
		mutex_unlock(&reference->next->branch_mutex);
	}

        free(reference);

	return 0;
}
