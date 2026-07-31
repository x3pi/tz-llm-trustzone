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

/* Real model tensor data (AllocStage/DecryptStage, alloc-stage-chcore.cpp)
 * is restricted to indices [0, TZASC_NR_MODEL) only -- index TZASC_NR-1 is
 * reserved exclusively for NPU real-weight scratch buffers
 * (ggml-rknpu-re.cpp's rknn_mem, under MAT_COPY). This partition is what
 * lets NPU buffers call commit_tzasc() safely: commit_tzasc() extends the
 * TZASC boundary strictly in address order per cma_index starting from that
 * index's own base, so two independent subsystems issuing push_pages() on
 * the SAME index with no ordering coordination between them would race
 * (one's commit could sit stuck in the pending queue forever waiting for a
 * gap the other subsystem will never fill). Giving NPU buffers their own
 * exclusive index sidesteps that entirely -- no cross-subsystem
 * coordination needed, just the existing per-index commit_tzasc() mutex/
 * priority-queue logic (already handles out-of-order completions *within*
 * one subsystem's own concurrent threads). See STATUS.md for the full
 * design rationale (tzasc_cma routing for real NPU weight buffers).
 */
#define TZASC_NR_MODEL          (TZASC_NR - 1)
#define TZASC_NR_NPU_SCRATCH    (TZASC_NR - 1)

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