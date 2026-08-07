# TZ-LLM Project Rules

Long-running TrustZone/NPU LLM inference port (TZ-LLM, EuroSys'26 paper,
arXiv 2511.13717) on Orange Pi 5 Max (RK3588). This project has an
extensive history of regressions caused by incomplete understanding of its
own build/flash/runtime structure. **Read this file before making any
change**, and re-check it before declaring any fix validated. The full
detailed history lives in `STATUS.md` (2500+ lines); `DEPLOYED_STATE.md` is
the single source of truth for what's actually flashed on the board right
now — read it before flashing anything, don't guess from file timestamps.

## Golden rule: never trust one code path's output as proof of a fix

Every fix in this project must be validated on **both** `-s 0` (real NPU)
and `-s 1` (CPU-only/strawman) before being considered done. A change that
"fixes" one path while silently breaking or not exercising the other is a
regression, not a fix. This is the de facto practice throughout `STATUS.md`
and is how several NPU-specific bugs were correctly isolated from
generic-pipeline bugs (e.g. `[LOGIT_DIAG]` top-5 comparison between paths).

Also compare against the paper's own architecture (`STATUS.md`'s citations
of arXiv 2511.13717, or re-fetch it) whenever changing what runs on
CPU vs NPU: **layer norm and self-attention (incl. its Q@K^T / attn@V
matmuls against the KV-cache) must run on CPU; only real static weight
matmuls (Q/K/V/O projections, FFN, lm_head) may go to the NPU.**
`ggml_backend_rknpure_supports_op()` in `ggml-rknpu-re.cpp` is the gate for
this — treat any change there as correctness-critical, not a tuning knob.

## Before touching memory/TZASC constants

- `TZASC_TOTAL_MEM_SIZE` must be changed identically in **all 4** locations:
  `tz-llm/linux-5.10-opi/arch/arm64/mm/init.c`,
  `tz-llm/tzdriver/core/tc_client_driver.c`,
  `tz-llm/tee_os_kernel/user/system-services/chcore-libc/musl-libc/install/include/chcore/llm.h`,
  and its mirror under `.../libchcore/porting/overrides/include/chcore/llm.h`.
- **3GB is the confirmed reliability ceiling on this board.** 3.5GB and 4GB
  both fail `tzasc_cma[3]` reservation (`ret -12`, NULL region) due to
  physical memory fragmentation from the `tzllm-secure-rgn5` DT carve-out.
  8GB causes real memory corruption / reboot loops (confirmed via live
  UART, "Bad page state") — never just bump this number to "make room" for
  a bigger model without re-verifying `dmesg | grep tzasc` shows all 4
  regions reserved OK, not FAILED/NULL, after every change.
- A `GGML_ASSERT`/`BUG_ON` inside the TA does **not** cleanly abort — it
  silently spins forever, indistinguishable from slow-but-working
  computation from the outside. Never conclude "no crash = OK"; look for a
  concrete state-advancing signal instead (see Testing section).

## Build/flash pipeline — do not improvise the order

Mixing these up has cost real debugging time more than once this session.

**Changed `tz-llm/llama.cpp` source (CA and/or TA logic):**
1. Build the CA/TA binaries first, via
   `./scripts/kick-the-tires/llama-builder.sh scripts/kick-the-tires/share bash -c ./build-llama-docker.sh`
   — **not** `build-llama.sh` (that script *also* redundantly tries to
   rebuild the OS kernel via `oh-builder.sh` first and can fail there for
   unrelated reasons before ever reaching the llama.cpp build).
   `llama-builder.sh` itself invokes `docker run -it` — **`-it` needs a
   real TTY and hangs/fails when run non-interactively** (e.g. from an
   agent's background Bash tool). In that case, replicate the `docker run`
   command manually without `-it`.
   Verify success by checking `common/CMakeFiles/common.dir/*.o` (or
   whichever object changed) timestamps/md5 in **both**
   `tz-llm/llama.cpp/build-chcore/` and `build-rknpure/` — a stale `fake`
   binary's md5 not changing is *expected* if only TA-side files (not
   `fake_ca.cpp`) changed; don't treat that alone as a build failure.
2. Then `./rebuild.sh` (bakes the just-refreshed `oh_tee/apps` binaries
   into the TEE-OS/kernel image via Docker).
3. `./flash/repack.sh` (repacks `uboot_repacked.img`; check the `optee`
   image hash in its output actually changed vs. the previous build).
4. **Manually copy** `scripts/kick-the-tires/share_build/images/boot.img`
   → `checkpoints/boot.img` — `repack.sh` does **not** do this. Skipping
   it silently reflashes a stale kernel while every later step reports
   success (this exact mistake previously cost a whole session, chased as
   a phantom NPU bug).
5. Board into MaskROM (see Flashing section below), `./flash/flash.sh`.
6. Physically power-cycle (not MaskROM) to actually boot.
7. Re-enable `hdcd` TCP + mount SSD (see Runtime section), push the fresh
   CA binaries from `scripts/kick-the-tires/share/build-rknpure/` to
   `/data/ssd/rknpu/` over `hdc`, verify md5 matches on-device.
8. Update `DEPLOYED_STATE.md` by hand once confirmed stable.

**Changed only `tz-llm/tee_os_kernel` or kernel/TZASC code (no llama.cpp
source change):** skip step 1 — running `build-llama-docker.sh`
unnecessarily just wastes time re-linking binaries that didn't change.

**If `TZASC_TOTAL_MEM_SIZE` or any of its 4 synced locations changed:** edit
all 4 together before rebuilding; nothing enforces this automatically.

## Flashing

- Board must show `ID 2207:350b ... Fuzhou Rockchip Electronics Company`
  in `lsusb` with **no `USB-MSC` suffix** before running `flash.sh`. A
  `USB-MSC` suffix means the device is in a degraded/wrong enumeration
  state — `rkdeveloptool`'s `cs 2` call has **no timeout** and will hang
  indefinitely waiting for a response the device will never send. If
  `flash.sh` produces zero output for more than ~60s, check `lsusb`
  first before assuming anything else is wrong; fix by re-entering
  MaskROM (unplug, hold MaskROM button, replug/power-cycle).
- Never flash `scripts/kick-the-tires/share_build/images/uboot.img`
  directly — it's missing the `boot_linux` GPT-partition-name fix this
  card's U-Boot needs. Always go through `flash/repack.sh` first.
- Use the fixed LBA constants `flash.sh` already hardcodes
  (`uboot@0x2000`, `boot_linux@0x88000`) — never let a script re-derive
  them by parsing `rkdeveloptool ppt` output (a past attempt at this wrote
  64MB starting at LBA 0x0 from a parsing bug, clobbering the GPT).
- Both flashing and readback over the MaskROM/USB channel are individually
  flaky (~10-20% single-shot corruption) — this is *why* `flash.sh` writes
  in 4MiB chunks with majority-of-3-vote verification. Don't simplify that
  away even though it looks paranoid.
- **`flash-full.sh` on a board that already has a working userdata (real
  activated account, wifi config, etc.) MUST be run with `SKIP_USERDATA=1`.**
  Without it, the default `assets/full-flash/userdata.img` (an empty F2FS
  template) silently overwrites the real userdata partition — confirmed
  2026-08-07 to cause the board to sit completely silent on UART with the
  USB device continuously re-enumerating after power-cycle (looked exactly
  like a reset-loop/brick). Recovered by rewriting
  `checkpoints/golden-image/idbloader_through_vendor.img` raw to LBA 0
  (restores idbloader+GPT+system+vendor, does *not* touch userdata) then
  reflashing `checkpoints/uboot_repacked.img`/`boot.img` on top via
  `flash.sh`. Note: `flash-full.sh`'s `UBOOT`/`BOOT` default args already
  correctly point at the fixed `checkpoints/` files — that part was never
  the problem, only the userdata overwrite was. After this kind of
  recovery, `/data/ssd` won't exist yet on a fresh userdata (`mkdir -p`
  it before mounting the NVMe SSD, don't assume it's pre-created).

## Runtime / testing checklist (do this every single boot)

1. Reboot before testing any new prompt/config — the TA is launched once
   per boot by `chanmgr`; every invocation after the first in the same
   boot session busy-spins forever with zero real progress, which looks
   exactly like a hang from the outside.
2. Re-enable `hdcd` TCP over UART (does not persist across reboot):
   `param set persist.hdc.port 8710` → `param set ohos.ctl.stop hdcd` →
   `/system/bin/hdcd -t &`. First attempt right after boot often returns
   garbled bytes — retry once before assuming failure.
3. WiFi does not auto-connect on reboot and can take 5+ minutes — don't
   poll aggressively; space out checks.
4. Remount SSD (also doesn't persist): `mount -t ext4
   /dev/block/nvme0n1p1 /data/ssd` (device node is `/dev/block/nvme0n1p1`,
   **not** `/dev/nvme0n1p1`).
5. Prefer the `[SECURE_LOGIT_DIAG]`/final-answer shared-memory relay
   channels over UART for reading results — UART is shared with unrelated
   kernel/other-thread output and has repeatedly corrupted captured text.
6. To tell "genuinely stalled" from "just slow, still working": check a
   concrete state-advancing signal (`dmesg | grep -c 'push #'` run twice
   ~10s apart, or a `[LOGIT_DIAG]`-style n_past counter moving) — not just
   whether the process is burning CPU. This project has multiple
   documented busy-spin livelocks that look identical to real computation
   from outside (e.g. the `fake_ca.cpp` relay threads spin forever via
   `sched_yield()` even after finishing, by design — don't mistake a
   finished-but-still-running process for a stuck one, and don't mistake a
   stuck one for still-computing).
7. Killing a stuck relay process: try plain `kill` (SIGTERM) first, not
   `kill -9` — `kill -9` has previously hung the whole kernel and, in a
   process blocked inside a secure-world SMC ioctl, may not work at all
   (uninterruptible sleep) regardless of signal.
8. A stale/mismatched CA binary on `/data/ssd/rknpu/` (separate from what
   was just flashed into the TEE-OS image) is a repeated root cause of
   confusing "regression" symptoms — always verify md5sum of pushed
   binaries against the freshly built ones before trusting a test result.
9. Two concurrent inference processes (secure-world relay, or CA-direct
   normal-world `llama-cli -ngl`) compete for the same limited NPU DMA
   pool and will crash/corrupt each other. Only run one at a time; check
   `ps -ef | grep -i ld-linux` for leftovers before starting a new test.

## Known open / historically-confusing issues — check here before re-diagnosing as new

- CPU-only (`-s 1`) path has a documented non-deterministic hang
  (~43% of runs historically) at kernel push counter `push #203` during
  tensor loading, suspected root cause a `std::priority_queue` race in
  `decrypt-stage.cpp`'s `commit_tzasc()` — never confirmed resolved in
  `STATUS.md`. If `-s 1` hangs with no progress for a long time, this is
  the leading suspect, not a fresh regression.
- CA-direct normal-world NPU path (`llama-cli -ngl 999` run directly, not
  through the secure-world relay) crashes with `mmap failed ...
  GGML_ASSERT(dma_ptr)` on the first real decode step — known, caused by
  the 3GB TZASC/CMA reservation starving normal-world RKNPU DMA memory.
  Not the same bug as anything secure-world-side; don't conflate the two.
- `wrap_user_chat` (in `common/arg.cpp`'s `parse_prompt()`) controls
  whether a free-text `-t`/`--text` prompt gets TinyLlama-chat's
  `<|system|>/<|user|>/<|assistant|>` template applied before tokenizing.
  It was silently flipped to `false` in commit `4eb45d612` with no
  explanation, and reverted back to `true` on 2026-08-06 after being
  identified as a likely cause of incoherent (non-chat-formatted)
  completions independent of any NPU/attention bug. If free-text-prompt
  output looks like raw continuation garbage rather than an actual answer,
  check this flag before assuming a numerical/quantization bug.
- File-based TA-side output capture (bare `fopen()`/`fwrite()`) does not
  work — the secure-world TA build has no POSIX filesystem access by
  design. Route any new diagnostic output through the existing shared-ring
  -buffer I/O-relay mechanism (`io-frontend.cpp`/`interface.h`,
  see the `[SECURE_LOGIT_DIAG]` channel as the template), not a file.
- Anything shared across the CA (glibc/normal-world) ↔ TA (musl/secure
  -world) process boundary via raw mmap'd memory must use pure hardware
  atomics (`std::atomic` on trivially-shared memory, CAS spinlocks) —
  never a libc-provided sync primitive (`std::mutex`, condvar, futex).
  glibc and musl's internal struct layouts differ; a shared `std::mutex`
  across this boundary is undefined behavior that has caused real bugs
  here before.
