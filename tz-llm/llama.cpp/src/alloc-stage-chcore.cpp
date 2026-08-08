#include "ggml.h"
#include "pipeline.h"
#include <atomic>
#include <vector>
#include <io-frontend.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <chcore/llm.h>
#include <chcore/bug.h>

#define ROUND_UP(x, n)   (((x) + (n)-1) & ~((n)-1))
#define PAGE_SIZE 0x1000

std::mutex alloc_mtx;
std::mutex gather_mtx;
std::mutex cma_mtx[TZASC_NR];

struct tzasc_cma_meta *tzasc_cma_meta_arr;

void tzasc_cma_init(void) {
    // Match the CA-side fix (fake_ca.cpp): this TA runs as a long-lived
    // process across many inference requests, so fully-buffered stdout
    // means [MEM_PROBE] output can sit unflushed indefinitely instead of
    // reaching UART when we actually need it (e.g. mid-hang).
    setvbuf(stdout, NULL, _IONBF, 0);

    vaddr_t vaddr;
    vaddr = chcore_alloc_vaddr(PAGE_SIZE << 10);
    BUG_ON(vaddr == 0);

    int ret = usys_map_tzasc_cma_meta(vaddr);
    BUG_ON(ret != 0);

    tzasc_cma_meta_arr = (struct tzasc_cma_meta *)vaddr;
    for (int i = 0; i < TZASC_NR; i++) {
        auto tzasc_cma_meta = tzasc_cma_meta_arr + i;
        printf("%s %d base %#lx size %#lx\n", __func__, __LINE__, tzasc_cma_meta->base, tzasc_cma_meta->size);
    }
    printf("[MEM_PROBE] baseline (before any real tensor push_pages), chcore free_mem_size=%lu bytes (%lu MiB)\n",
        usys_get_free_mem_size(), usys_get_free_mem_size() >> 20);
    fflush(stdout);
}

static std::once_flag tzasc_flag;

// One-off measurement of ChCore's own physmem_map free capacity (5 pools,
// see mmparse.c physmem_map[0..4]) while real per-tensor CMA allocation is
// happening, to check how much of physmem_map[3] (the ~8GB "rgn5" pool,
// entirely separate from the Linux-side tzasc_cma region that actually
// backs the model weights) the TA/llama-cli genuinely consumes for its own
// bookkeeping (page tables, capability objects, etc.) versus leaves idle.
static std::atomic<int> push_pages_call_ctr{0};

// soft=false (default, existing behavior): after 200 retries, BUG_ON --
// appropriate for callers with no fallback (a genuinely unrecoverable
// situation for them). soft=true: after 200 retries, return a negative
// value instead of crashing, so the caller can try a different cma_index
// (used by tensor_pool_alloc/npu_scratch_alloc's incremental-growth,
// overflow-to-next-index allocators, where "this index has no more room"
// is an EXPECTED, recoverable outcome, not a fatal one -- 200 retries
// with a yield between each is already enough to rule out the transient
// -EINTR condition described below, so a failure surviving all 200 is a
// reliable signal of genuine exhaustion, not bad luck).
int push_pages_ex(size_t len, int cma_index, bool soft) {
    std::call_once(tzasc_flag, tzasc_cma_init);

    GGML_ASSERT(cma_index >= 0 && cma_index < TZASC_NR);
    auto tzasc_cma_meta = tzasc_cma_meta_arr + cma_index;

    // BUG FIX: the kernel's cma_alloc() can fail transiently even when
    // the pool is mostly free -- confirmed on hardware, dmesg showed
    // "tzasc2: alloc failed, req-size: 256 pages, ret: -4" (ret -4 =
    // -EINTR, NOT -ENOMEM) with "158938 free of 196608 total pages"
    // still available. -EINTR from cma_alloc() is a well-known transient
    // condition (internal migration/compaction got interrupted) -- the
    // standard, correct handling is to just retry, not treat it as
    // real exhaustion. Previously this one-shot ret fell straight into
    // BUG_ON(ret < 0), which prints once and then spins in an infinite
    // empty for(;;) loop FOREVER (chcore/bug.h) -- not a crash, not a
    // clean failure: a silent, CPU-pegged hang that looks exactly like
    // slow computation from the outside (confirmed: this is what made
    // the -s 0 NPU test appear to run for 35+ minutes with climbing CPU
    // and zero progress, right after a push logged this exact ret: -4).
    // Retry a bounded number of times with a yield in between.
    int ret;
    int attempt;
    for (attempt = 0; attempt < 200; attempt++) {
        struct smc_registers req = {0};
        req.x1 = SMC_EXIT_SHADOW;
        req.x2 = 1;
        req.x3 = ROUND_UP(len, PAGE_SIZE) | cma_index;
        ret = usys_tee_switch_req(&req);
        if (ret >= 0) break;
        usys_yield();
    }
    if (attempt > 0 && ret >= 0) {
        printf("[PUSH_RETRY] push_pages succeeded after %d retr%s (cma_index=%d, len=%#zx)\n",
            attempt, attempt == 1 ? "y" : "ies", cma_index, len);
        fflush(stdout);
    }
    if (ret < 0) {
        if (soft) {
            printf("[PUSH_SOFT_FAIL] push_pages exhausted 200 retries (cma_index=%d, len=%#zx) -- reporting failure to caller instead of BUG_ON\n",
                cma_index, len);
            fflush(stdout);
            return ret;
        }
        BUG_ON(ret < 0);
    }

    int c = push_pages_call_ctr.fetch_add(1);
    if (c % 16 == 0) {
        printf("[MEM_PROBE] push_pages call #%d, chcore free_mem_size=%lu bytes (%lu MiB)\n",
            c, usys_get_free_mem_size(), usys_get_free_mem_size() >> 20);
        fflush(stdout);
    }
    return ret;
}
int push_pages(size_t len, int cma_index) {
    return push_pages_ex(len, cma_index, false);
}
int pop_pages(int cma_index) {
    std::call_once(tzasc_flag, tzasc_cma_init);

    GGML_ASSERT(cma_index >= 0 && cma_index < TZASC_NR);
    auto tzasc_cma_meta = tzasc_cma_meta_arr + cma_index;

    struct smc_registers req = {0};
    req.x1 = SMC_EXIT_SHADOW;
    req.x2 = 0;
    req.x3 = cma_index;
    int ret = usys_tee_switch_req(&req);
    BUG_ON(ret != 0);
    return tzasc_cma_meta->count;
}

std::atomic<int64_t> cma_time;
std::atomic<size_t> cma_size;

extern bool is_strawman;
// See io-stage.cpp for why strawman's block size was reduced from 8GiB.
//
// BUG FIX: the non-strawman 4MiB size meant every AllocTask asked the
// kernel's cma_alloc() for 1024 physically-contiguous pages from one of
// the four ~768MiB tzasc CMA pools. Under real combined TrustZone+NPU
// decoding load (many tensors' worth of push/pop churn across all 4
// threads/pools), this reliably hit genuine external fragmentation:
// dmesg showed a pool with 167469/196608 pages (~85%) still free, yet
// cma_alloc() still failed to find 1024 contiguous free pages, tripping
// the BUG_ON(ret < 0) in push_pages() and crashing the TA. Shrinking to
// 1MiB (256 pages) makes each individual contiguity requirement 4x
// easier to satisfy from fragmented free space, at the cost of more SMC
// round-trips per tensor - a pure tuning knob, doesn't change the
// pipelined-restoration design itself (EuroSys'26 S4.1-4.2).
//
// SUPERSEDED by AllocTask::step()'s pooled allocator (tensor_pool_alloc,
// below): each cma_index now does exactly ONE real push_pages()/
// cma_alloc() call for its entire ~768MiB bank, made once and reused via
// sub-allocation (bump offset) for every AllocTask on that index --
// BLOCK_SIZE is no longer a physical-contiguity request size at all, just
// a bookkeeping/pipelining granularity. The fragmentation crash this
// constant was originally tuned to avoid (see the retained comment
// history in git log / STATUS.md) is now structurally impossible: there
// is nothing left to fragment after the one whole-bank reservation.
// Matches strawman's size since neither path's block size has any
// remaining fragmentation implication.
#define BLOCK_SIZE (is_strawman ? (64UL << 20) : (64UL << 20))

// Pooled tensor-loading allocator: reserve the WHOLE physical bank for
// each of the TZASC_NR_MODEL indices used for model-tensor loading, ONCE
// per index (lazily, on first use), sub-allocated (bump allocator) from
// then on instead of one push_pages()/cma_alloc() call per BLOCK_SIZE
// chunk. This mirrors the exact pattern already proven safe tonight for
// the NPU weight-scratch allocator (ggml-rknpu-re.cpp's
// npu_scratch_alloc), applied here to remove indices 0-2's OWN
// fragmentation risk (see BLOCK_SIZE's comment above) so BLOCK_SIZE can
// go back up toward strawman's 64MiB without the "many small concurrent
// cma_alloc() calls fragment the pool" crash this file's 1MiB/4MiB tuning
// was defending against. Confirmed safe to map one physical range into
// many different vaddrs: sys_map_tzasc_cma_pmo (tee_os_kernel/kernel/
// object/memory.c) is a raw map_range_in_pgtbl() call with no exclusivity
// tracking, so multiple AllocTasks aliasing the same pooled paddr range
// into their own distinct vaddrs is a supported page-table pattern.
// ADAPTIVE VERSION: instead of reserving a whole ~768MiB bank per index
// up front (which starves ggml-rknpu-re.cpp's npu_scratch_alloc() of any
// spare room in the same 4 banks -- confirmed on hardware: NPU-scratch
// needs MORE than its own dedicated bank for TinyLlama's ~1.1GB of
// NPU-tiled weight data, and hit GGML_ASSERT/an infinite spin once its
// bank filled), each index grows INCREMENTALLY in fixed-size chunks,
// each chunk its own push_pages() reservation, sub-allocated (bump
// offset) same as before. Small models use few chunks; large models use
// more -- no hardcoded total, no guessing another model's size ahead of
// time. Concurrency/fragmentation stays safe because growth only
// happens when the CURRENT chunk is exhausted (serialized by the pool's
// own mutex), not many small concurrent allocations racing each other.
struct tensor_pool_region_t {
    int entry_index = -1;
    unsigned long base_paddr = 0;
    size_t total_size = 0;
    size_t offset = 0;
};
struct tensor_pool_t {
    std::mutex mtx;
    std::vector<tensor_pool_region_t> regions;
};
static tensor_pool_t g_tensor_pool[TZASC_NR];
static const size_t TENSOR_POOL_CHUNK_SIZE = 128UL << 20; // 128MiB/grow

// SECOND BUG FOUND (same night, right after the first adaptive-pool
// build): this pool and ggml-rknpu-re.cpp's npu_scratch_alloc() BOTH grow
// into indices 0-2 (NPU-scratch overflows there once its own dedicated
// index 3 fills up), but had entirely separate, uncoordinated bookkeeping
// -- each only knows about ITS OWN chunks, not the other's. Not a data-
// race at the physical level (push_pages_ex()/cma_alloc() is still the
// single authoritative arbiter -- whichever caller asks first for the
// last available space gets it, the other gets a clean failure), but
// tensor-loading's OWN response to that failure was BUG_ON (a silent
// infinite spin in this environment, not a clean abort) with no
// fallback, on the theory that "each worker thread is permanently bound
// to one index, there's nothing else to try". That's wrong: the POOL
// BACKING for one more chunk doesn't have to come from the thread's
// preferred index -- only which index a NEW GROW lands on needs to be
// tracked and threaded back to the caller (AllocTask now carries its own
// actual_cma_index, separate from the round-robin-assigned preferred
// one, for exactly this). Mirrors npu_scratch_alloc()'s own overflow
// try-order, just starting from whichever index this thread prefers.
//
// Returns the actual cma_index (via *out_cma_index -- may differ from
// preferred_cma_index once overflow kicks in) and entry_index the
// allocation landed in, and writes the byte offset within that entry to
// *out_offset.
static int tensor_pool_alloc(int preferred_cma_index, size_t size, unsigned long *out_offset, int *out_cma_index) {
    // BUG FIX (found via a NULL+8 page fault on the very first call):
    // tzasc_cma_meta_arr is NULL until push_pages()/pop_pages()'s own
    // std::call_once(tzasc_flag, tzasc_cma_init) runs -- reading it
    // BEFORE ever calling push_pages() dereferences a null pointer
    // (offsetof(size)==8, matching the observed faulting address 0x8
    // exactly). Trigger the same one-time init explicitly, first.
    std::call_once(tzasc_flag, tzasc_cma_init);
    size_t rounded = ROUND_UP(size, PAGE_SIZE);
    {
        auto &pool = g_tensor_pool[preferred_cma_index];
        std::lock_guard<std::mutex> _(pool.mtx);
        if (!pool.regions.empty()) {
            auto &r = pool.regions.back();
            if (r.offset + rounded <= r.total_size) {
                *out_offset = r.offset;
                *out_cma_index = preferred_cma_index;
                r.offset += rounded;
                return r.entry_index;
            }
        }
    }
    // Preferred index's current region (if any) is full -- try growing
    // it, then overflow into the OTHER indices (including
    // TZASC_NR_NPU_SCRATCH as a last resort -- by the time tensor-
    // loading is scrambling for space, NPU-scratch allocation for
    // earlier layers may not have started yet) in order.
    size_t grow = std::max(TENSOR_POOL_CHUNK_SIZE, rounded);
    int try_order[TZASC_NR];
    try_order[0] = preferred_cma_index;
    {
        int j = 1;
        for (int i = 0; i < TZASC_NR; i++)
            if (i != preferred_cma_index) try_order[j++] = i;
    }
    for (int t = 0; t < TZASC_NR; t++) {
        int cma_index = try_order[t];
        auto &pool = g_tensor_pool[cma_index];
        std::lock_guard<std::mutex> _(pool.mtx);
        // Re-check: another thread may have already grown this index
        // (including this SAME preferred index, if we're not the first
        // to notice it was full) while we didn't hold its lock.
        if (!pool.regions.empty()) {
            auto &r = pool.regions.back();
            if (r.offset + rounded <= r.total_size) {
                *out_offset = r.offset;
                *out_cma_index = cma_index;
                r.offset += rounded;
                return r.entry_index;
            }
        }
        int entry_index;
        {
            std::lock_guard<std::mutex> __(cma_mtx[cma_index]);
            entry_index = push_pages_ex(grow, cma_index, /*soft=*/true);
        }
        if (entry_index < 0) {
            printf("[TENSOR_POOL] cma_index=%d full, trying next index\n", cma_index);
            fflush(stdout);
            continue;
        }
        tensor_pool_region_t r;
        r.entry_index = entry_index;
        r.base_paddr = tzasc_cma_meta_arr[cma_index].entry[entry_index].paddr;
        r.total_size = grow;
        r.offset = rounded;
        pool.regions.push_back(r);
        printf("[TENSOR_POOL] cma_index=%d grew: entry_index=%d paddr=%#lx size=%#zx (region #%zu, preferred was %d)\n",
            cma_index, entry_index, r.base_paddr, grow, pool.regions.size(), preferred_cma_index);
        fflush(stdout);
        *out_offset = 0;
        *out_cma_index = cma_index;
        return entry_index;
    }
    // Every one of the TZASC_NR banks is genuinely full -- the real ~3GB
    // combined physical ceiling (see STATUS.md's LRU-eviction discussion
    // for what a fix beyond this point would need). Not recoverable here.
    printf("[TENSOR_POOL] ALL %d indices full -- genuine total capacity exhaustion\n", TZASC_NR);
    fflush(stdout);
    BUG_ON(true);
    return -1; // unreachable, silences -Wreturn-type
}

class AllocTask : public Task {
public:
    int tzd_fd;
    size_t size;
    vaddr_t vaddr;
    int cma_index; // preferred (round-robin-assigned) index -- for locking/scheduling only now
    int actual_cma_index = -1; // where tensor_pool_alloc() actually landed this chunk (may overflow)
    int entry_index;
    unsigned long entry_offset = 0;

    AllocTask(size_t size, vaddr_t vaddr, int cma_index = -1): size(size), vaddr(vaddr), cma_index(cma_index) {

    }
    void step(void) override {
#ifdef TZ_LLM_MEASURE
        auto start = get_micro();
#endif
        // All AllocTasks sharing one (cma_index,entry_index) pair share
        // ONE pooled entry (see tensor_pool_alloc), distinguished from
        // each other by entry_offset instead of by separate entry_index
        // values. Downstream (io-backend.cpp's get_buf(), the kernel's
        // llm_client_mmap()) already thread entry_offset through for
        // exactly this purpose. actual_cma_index may differ from the
        // preferred cma_index once overflow-to-another-index kicks in --
        // AllocStage::submit() must record actual_cma_index, not
        // cma_index, or downstream physical-address bookkeeping
        // (msg.paddr, commit_tzasc()'s aggregate ranges) silently points
        // at the wrong bank.
        entry_index = tensor_pool_alloc(cma_index, size, &entry_offset, &actual_cma_index);
        unsigned long paddr = tzasc_cma_meta_arr[actual_cma_index].entry[entry_index].paddr + entry_offset;
        GGML_ASSERT(usys_map_tzasc_cma_pmo(vaddr, size, paddr) == 0);
#ifdef TZ_LLM_MEASURE
        cma_size += ROUND_UP(size, BLOCK_SIZE);
        cma_time += get_micro() - start;
#endif
    }
};

std::atomic<int> last_pos;

AllocStage::AllocStage(size_t off, size_t len): addr(NULL) {
    size = io_align_up(off + len) - io_align_down(off);
    addr = (void *)chcore_alloc_vaddr(size);
    msg.buf = addr;
    msg.paddr.resize(TZASC_NR);
    GGML_ASSERT(addr);

    // TZASC_NR_MODEL (not TZASC_NR): real tensor data is only ever
    // distributed across indices [0, TZASC_NR_MODEL) -- index TZASC_NR-1
    // stays untouched by this pipeline, reserved for NPU real-weight
    // scratch buffers (see chcore/llm.h). block_nr[TZASC_NR-1] is left at
    // its zero-initialized value and never incremented below.
    all_block_nr = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int cur_start = last_pos.fetch_add(all_block_nr) % TZASC_NR_MODEL;
    int cur_end = (cur_start + all_block_nr) % TZASC_NR_MODEL;
    for (int i = 0; i < TZASC_NR_MODEL; i++) {
        block_nr[i] = all_block_nr / TZASC_NR_MODEL;
        if (cur_start <= cur_end) {
            if (cur_start <= i && i < cur_end) {
                block_nr[i]++;
            }
        } else {
            if (cur_start <= i || i < cur_end) {
                block_nr[i]++;
            }
        }
    }

    GGML_ASSERT(sizeof(block_nr) / sizeof(all_block_nr) >= TZASC_NR);

    int test_sum = 0;
    for (int i = 0; i < TZASC_NR_MODEL; i++) {
        test_sum += block_nr[i];
    }
    if (test_sum != all_block_nr) {
        printf("[ALLOC_STAGE_BUG] size=%zu BLOCK_SIZE=%lu all_block_nr=%d cur_start=%d cur_end=%d "
            "block_nr=[%d,%d,%d,%d] test_sum=%d TZASC_NR_MODEL=%d\n",
            size, (unsigned long)BLOCK_SIZE, all_block_nr, cur_start, cur_end,
            block_nr[0], block_nr[1], block_nr[2], block_nr[3], test_sum, TZASC_NR_MODEL);
        fflush(stdout);
    }
    GGML_ASSERT(test_sum == all_block_nr);
}

void AllocStage::start(void *input)
{
    (void)input;
    for (int i = 0; i < TZASC_NR; i++) {
        finished_nr = 0;
        get_nr[i] = 0;
        submit_pos = 0;
    }
    msg.cma_indexes.clear();
}

// TEMP DIAGNOSTIC: chasing the "always stalls at the same push count"
// race (STATUS.md, Bug #2 continuation) -- log every AllocStage get_task/
// submit call with the full internal counter state, keyed by `this` (one
// AllocStage instance per tensor's Pipeline) and a global monotonic
// sequence number, so the exact last-live state of whichever AllocStage
// stalls is captured even though this whole print sits behind
// submit_pos_mtx/finished_nr (real-time cost is out-of-scope, this is a
// diagnostic build only).
static std::atomic<int> alloc_trace_ctr{0};
// Multiple ChCore threads printf() to the same physical UART concurrently
// with no line-atomicity guarantee, badly interleaving/corrupting the
// diagnostic output byte-by-byte (confirmed on hardware: first capture
// with this instrumentation was largely unreadable). Serialize just this
// diagnostic's own prints with a dedicated mutex so each line comes out
// intact, at the cost of extra contention (acceptable for a diagnostic
// build only).
std::mutex alloc_trace_print_mtx;

std::pair<std::shared_ptr<Task>, bool> AllocStage::get_task(void *arg)
{
    std::lock_guard<std::mutex> _(submit_pos_mtx);
    int cma_index = (int)(long)arg;
    GGML_ASSERT(submit_pos < size);

    for (int i = 0; i < TZASC_NR; i++) {
        GGML_ASSERT(get_nr[i] <= block_nr[i]);
    }
    if (get_nr[cma_index] == block_nr[cma_index]) {
        // TZASC_NR_MODEL: never fall back onto the NPU-reserved index
        // (its block_nr[] is always 0 here anyway, so this is belt-and-
        // suspenders, not strictly required -- see chcore/llm.h).
        for (int i = 0; i < TZASC_NR_MODEL; i++) {
            if (get_nr[i] < block_nr[i]) {
                cma_index = i;
                break;
            }
        }
    }
    get_nr[cma_index]++;

    auto task = std::make_shared<AllocTask>(ROUND_UP(std::min(BLOCK_SIZE, size - submit_pos), PAGE_SIZE), (vaddr_t)addr + submit_pos, cma_index);
    submit_pos += BLOCK_SIZE;
    bool is_last = submit_pos >= size;
    {
        // THROTTLE UPDATE (2026-08-08): fires once per BLOCK_SIZE chunk of
        // every tensor -- unthrottled, this was thousands of synchronous
        // printf+fflush calls per model load, contributing to the RCU-
        // stall/soft-lockup cascade documented in tc_client_driver.c.
        int n = alloc_trace_ctr.fetch_add(1);
        if ((n % 5000) == 0) {
            std::lock_guard<std::mutex> _p(alloc_trace_print_mtx);
            printf("[ALLOC_TRACE] #%d get_task this=%p tid_arg=%d cma=%d get_nr=[%d,%d,%d,%d] block_nr=[%d,%d,%d,%d] finished_nr=%d all_block_nr=%d submit_pos=%zu size=%zu is_last=%d\n",
                n, (void *)this, (int)(long)arg, cma_index,
                get_nr[0], get_nr[1], get_nr[2], get_nr[3],
                block_nr[0], block_nr[1], block_nr[2], block_nr[3],
                (int)finished_nr, all_block_nr, submit_pos, size, is_last);
            fflush(stdout);
        }
    }
    return { task, is_last };
}

bool AllocStage::submit(std::shared_ptr<Task> task)
{
    AllocTask *alloc_task = dynamic_cast<AllocTask *>(task.get());
    GGML_ASSERT(alloc_task);
    {
        std::lock_guard<std::mutex> _(gather_mtx);
        // Use actual_cma_index (where tensor_pool_alloc() really landed
        // this chunk), NOT cma_index (the original round-robin
        // preference) -- once overflow-to-another-index kicks in these
        // can differ, and msg.paddr is indexed BY the physical bank, so
        // recording under the wrong index would misfile this chunk's
        // range under the wrong bank's commit_tzasc() aggregation.
        GGML_ASSERT(alloc_task->actual_cma_index >= 0);
        msg.cma_indexes.push_back({alloc_task->actual_cma_index, alloc_task->entry_index, alloc_task->vaddr - (vaddr_t)addr, (off_t)alloc_task->entry_offset, alloc_task->size});
        // Pooled entries: entry->paddr is the START OF THE WHOLE POOL
        // (shared by every AllocTask on this actual_cma_index), NOT this
        // specific chunk's own address -- must add entry_offset to get
        // this chunk's real physical range. Getting this wrong would
        // silently under-report the committed TZASC range downstream
        // (IOStage::start()'s cma_region min/max, see pipeline.h/
        // io-stage.cpp) without any crash or error -- a correctness bug,
        // not a crash, so this line needs to be exactly right.
        unsigned long chunk_paddr = tzasc_cma_meta_arr[alloc_task->actual_cma_index].entry[alloc_task->entry_index].paddr + alloc_task->entry_offset;
        msg.paddr[alloc_task->actual_cma_index].push_back({
            chunk_paddr,
            chunk_paddr + alloc_task->size
        });
    }
    auto old_nr = finished_nr.fetch_add(1);
    bool is_done = (old_nr + 1 == all_block_nr);
    {
        // THROTTLE UPDATE (2026-08-08): same reasoning as get_task() above.
        // Always print on is_done (once per tensor, not per block) so
        // pipeline-completion visibility isn't lost.
        int n = alloc_trace_ctr.fetch_add(1);
        if (is_done || (n % 5000) == 0) {
            std::lock_guard<std::mutex> _p(alloc_trace_print_mtx);
            printf("[ALLOC_TRACE] #%d submit    this=%p cma=%d actual_cma=%d entry=%d old_nr=%d all_block_nr=%d is_done=%d\n",
                n, (void *)this, alloc_task->cma_index, alloc_task->actual_cma_index, alloc_task->entry_index, old_nr, all_block_nr, is_done);
            fflush(stdout);
        }
    }
    if (is_done)
        return true;
    return false;
}

void *AllocStage::get_msg(void)
{
    GGML_ASSERT(addr);
    return &msg;
}

void AllocStage::rollback(void)
{
    // Pooling (tensor_pool_alloc) means block_nr[cma_index] no longer
    // equals the number of real push_pages() calls on that index -- it's
    // now at most ONE real push per index, shared by every AllocStage
    // ever constructed. Calling pop_pages() block_nr[cma_index] times
    // here (the old behavior) would try to pop a shared entry that
    // doesn't have that many independent lives, corrupting the pool for
    // every OTHER in-flight/future AllocStage on the same index. Matches
    // the NPU weight-scratch pool's own precedent (ggml-rknpu-re.cpp):
    // pooled tzasc_cma entries are never individually freed, they live
    // for the TA process's whole lifetime -- there is no correct partial-
    // rollback for a pooled sub-allocation, only whole-process teardown.
    GGML_ASSERT(addr);
}
