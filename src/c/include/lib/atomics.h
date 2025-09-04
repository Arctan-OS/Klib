/**
 * @file atomics.h
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
#ifndef ARC_LIB_ATOMICS_H
#define ARC_LIB_ATOMICS_H

#include <stdatomic.h>
#include <stdbool.h>

#define ARC_ATOMIC_INC(__val)                            __atomic_add_fetch(&__val, 1, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_DEC(__val)                            __atomic_sub_fetch(&__val, 1, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_LOAD(__val)                           __atomic_load_n(&__val, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_STORE(__dest, __val)                  __atomic_store_n(&__dest, __val, __ATOMIC_RELEASE)
#define ARC_ATOMIC_XCHG(__mem, __val, __ret)             __atomic_exchange(__mem, __val, __ret, __ATOMIC_ACQUIRE)
#define ARC_ATOMIC_CMPXCHG(__ptr, __expected, __desired) __atomic_compare_exchange_n(__ptr, __expected, __desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)

#define ARC_MEM_BARRIER   __asm__("" ::: "memory");

#ifdef ARC_TARGET_ARCH_X86_64
#define ARC_ATOMIC_LFENCE __asm__("lfence" :::);
#define ARC_ATOMIC_SFENCE __asm__("sfence" :::);
#define ARC_ATOMIC_MFENCE __asm__("mfence" :::);
#endif

#endif
