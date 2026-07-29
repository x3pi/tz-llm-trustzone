/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef LLM_H
#define LLM_H

#include <chcore/type.h>

#define TZASC_NR               (4)
/* Must match tzdriver/core/tc_client_driver.c's TZASC_TOTAL_MEM_SIZE, which
 * must in turn match arch/arm64/mm/init.c's (actual reservation size). */
#define TZASC_TOTAL_MEM_SIZE   (3UL * (1UL << 30))
#define TZASC_PER_CMA_MEM_SIZE (TZASC_TOTAL_MEM_SIZE / TZASC_NR)

struct tzasc_cma_entry {
	unsigned long paddr;
	unsigned long size;
	struct page *cma_pages;
};

struct tzasc_cma_meta {
	unsigned long base;
	unsigned long size;
	unsigned long count;
	struct tzasc_cma_entry entry[((4096 << 10) / TZASC_NR - sizeof(unsigned long) * 4) / sizeof(struct tzasc_cma_entry)];
};

#define CMD_QUEUE_SHM_SIZE (256 * 4096)

#endif