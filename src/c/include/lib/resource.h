/**
 * @file resource.h
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
#ifndef ARC_RESOURCE_H
#define ARC_RESOURCE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <lib/atomics.h>

#define ARC_DRIVER_IDEN_SUPER 0x5245505553 // "SUPER" little endian
#define ARC_SIGREF_CLOSE 0xA

#define ARC_REGISTER_DRIVER(group, name, ext) \
	struct ARC_DriverDef __driver_##name##_##ext

struct ARC_Resource {
	ARC_GenericMutex prop_mutex;

	struct ARC_Reference *references;

	uint64_t ref_count;

	/// State managed by driver, owned by resource.
	ARC_GenericMutex dri_state_mutex;
	void *driver_state;

	/// Resource id.
	uint64_t id;
	/// Instance.
	uint64_t instance;
	/// Driver function group (supplied on init by caller).
	int dri_group;
	/// Specific driver function set (supplied on init by caller).
	uint64_t dri_index;
	/// Driver functions.
	struct ARC_DriverDef *driver;
};

struct ARC_Reference {
	// Functions for managing this reference.
	struct ARC_Resource *resource;
	// The signal funciton must not be NULL
	int (*signal)(int code, void *data);
	int64_t pid;
	ARC_GenericMutex branch_mutex;
	struct ARC_Reference *prev;
	struct ARC_Reference *next;
};

struct ARC_File {
	/// Current offset into the file.
	long offset;
	/// Pointer to the VFS node.
	struct ARC_VFSNode *node;
	/// Reference
	struct ARC_Reference *reference;
	/// Reference counter for the decsriptor itself.
	int ref_count;
	/// Mode the file was opened with.
	uint32_t mode;
	/// Flags the file was opened with.
	int flags;
};

// Driver definitions
// NOTE: No function pointer in a driver definition
//       should be NULL.
struct ARC_DriverDef {
	// The index of this driver
	uint64_t index;
	uint64_t instance_counter;
 	uint32_t *pci_codes; // Terminates with ARC_DRI_PCI_TERMINATOR if non-NULL
	char *name_format;
	// Specific
	uint64_t identifer;
	void *driver;
	// Generic
	int (*init)(struct ARC_Resource *res, void *args);
	int (*uninit)(struct ARC_Resource *res);
	int (*open)(struct ARC_File *file, struct ARC_Resource *res, int flags, uint32_t mode);
	int (*write)(void *buffer, size_t size, size_t count, struct ARC_File *file, struct ARC_Resource *res);
	int (*read)(void *buffer, size_t size, size_t count, struct ARC_File *file, struct ARC_Resource *res);
	int (*close)(struct ARC_File *file, struct ARC_Resource *res);
	int (*seek)(struct ARC_File *file, struct ARC_Resource *res);
	/// Rename the resource.
	int (*rename)(char *newname, struct ARC_Resource *res);
	/// Stat the given driver, if filename is NULL return the stat of the current driver, otherwise return the stat of the file at that path.
	int (*stat)(struct ARC_Resource *res, char *filename, struct stat *stat);
	/// A function to signal modification of driver parameters.
	int (*control)(struct ARC_Resource *res, void *buffer, size_t size);
};

struct ARC_SuperDriverDef {
	int (*create)(char *path, uint32_t mode, int type);
	int (*remove)(char *path);
	int (*link)(char *a, char *b);
	/// Rename the file.
	int (*rename)(char *a, char *b);
	/// Acquire the needed information for initalization of a file resource. The return value should be passed as the argument of the function.
	void *(*locate)(struct ARC_Resource *res, char *filename);
};
// /Driver definitions

struct ARC_Resource *init_resource(uint64_t dri_index, void *args);
struct ARC_Resource *init_pci_resource(uint16_t vendor, uint16_t device, void *args);
int uninit_resource(struct ARC_Resource *resource);
struct ARC_Reference *reference_resource(struct ARC_Resource *resource);
int unrefrence_resource(struct ARC_Reference *reference);

#endif
