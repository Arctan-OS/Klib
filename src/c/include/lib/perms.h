/**
 * @file perms.h
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
#ifndef ARC_LIB_PERMS_H
#define ARC_LIB_PERMS_H

#include <stdint.h>
#include <abi-bits/stat.h>

#define ARC_STD_PERM 0700

/**
 * Check the requested permissions against the permissions of the file.
 *
 * @param struct stat *stat - The permissions of the file.
 * @param uint32_t requested - Requested permissions (mode).
 * @return Zero if the current program has requested privelleges.
 * */
int check_permissions(struct stat *stat, uint32_t requested);

#endif
