/**
 * @file sysv.h
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
#include "global.h"
#include <lib/convention/sysv.h>
#include <lib/util.h>

#define AT_NULL 0
#define AT_IGNORE 1
#define AT_EXECFD 2
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_BASE 7
#define AT_FLAGS 8 
#define AT_ENTRY 9
#define AT_LIBPATH 10
#define AT_FPHW 11
#define AT_INTP_DEVICE 12
#define AT_INTP_INODE 13

static uint64_t *sysv_insert_auxvec(uint64_t type, uint64_t data) {

}

int sysv_prepare_process_stack(struct ARC_Thread *thread, struct ARC_ELFMeta *meta, char **env, int envc, char **argv, int argc) {
        if (thread == NULL) {
                return -1;
        }

        uint64_t *rsp = (uint64_t *)thread->ctx.rsp;

        if (rsp == NULL) {
                return -2;
        }

        for (int i = 0; i < envc; i++) {
                size_t size = strlen(env[i]);

                rsp -= size + 2;
                memcpy(rsp, env[i], size + 1);
        }

        for (int i = 0; i < argc; i++) {
                size_t size = strlen(argv[i]);

                rsp -= size + 2;
                memcpy(rsp, argv[i], size + 1);
        }

        rsp = (uint64_t *)ALIGN((uintptr_t)rsp, 16);
        rsp -= 16;

        rsp = sysv_insert_auxvec(AT_ENTRY, (uint64_t)meta->entry);
        rsp = sysv_insert_auxvec(AT_PHDR, (uint64_t)meta->phdr);
        rsp = sysv_insert_auxvec(AT_PHENT, (uint64_t)meta->phent);
        rsp = sysv_insert_auxvec(AT_PHNUM, (uint64_t)meta->phnum);

        

        return 0;
}