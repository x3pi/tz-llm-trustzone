# TZ-LLM on Orange Pi 5 Max — Status (2026-07-29)

Paper: "TZ-LLM: Protecting On-Device Large Language Models with Arm TrustZone"
(EuroSys '26). Artifact: https://zenodo.org/records/17054270. Board is an
Orange Pi 5 **Max** (RK3588); the artifact targets 5 **Plus** — several fixes
below are 5-Max-specific.

## Current furthest state (checkpoint in this project)

`checkpoints/uboot_repacked.img` + `checkpoints/boot.img` is the latest build,
byte-verified as flashed correctly (see `SHA256SUMS`). Boot sequence on this
checkpoint, confirmed live:

1. TEE-OS boots clean, no FIT/hash errors.
2. `chanmgr` launches `llama-cli` directly from `main()` (correct, matches
   upstream's own `if (1)` structure).
3. llama-cli's SMC handshake with the REE succeeds — receives a real shared
   memory address (`received shm addr 0x1e2900000`), not null.
4. The **second** SMC round trip also succeeds: the CA (`fake`) fills
   `task_queue` with real prompt/model info, llama-cli parses it correctly
   (`prompt tinyllama#0`, `inner_model_path tinyllama-...-meta.gguf`).
5. GGUF metadata loads (201 tensors, vocab list, etc.).
6. **Crashes** with `terminate called after throwing an instance of
   'std::bad_alloc'` right after tensor-type listing, before
   `llm_load_tensors`/`llm_load_print_meta`. This is the current blocker —
   not yet root-caused. Full crash dump: this session's transcript, or
   reproduce by flashing the checkpoint and running the repro command below.

This is further than any previous attempt got: two prior blockers (`/rknpu.srv`
launched too early; `/rknpu.srv` launched in the wrong process after a
protocol-breaking reorder) are both fixed and no longer reproduce.

## The 5 bugs found this session, in the order they were hit

1. **`chanmgr/main.c`: `if (1)` had been flipped to `if (0)`**, disabling the
   entire llama-cli launch (a local regression vs. upstream, found via
   `git diff` inside `tz-llm/tee_os_kernel`). Fixed: restored `if (1)`.

2. **`/rknpu.srv` (the NPU co-driver self-test) launched too early.** First
   attempt put it in `main()`, before Linux/the REE NPU driver exist. It
   issued one real fp16 matmul then blocked in `waitpid()` forever — the
   TEE-side NPU driver is data-plane only (paper §4.3), it needs the REE
   control plane, which doesn't exist yet at that point in boot.

3. **`/rknpu.srv` had a real, unconditional infinite loop**
   (`user/system-services/system-servers/rknpu/src/main.c`):
   `while (1) { run_matmul(100, 1536, 4096, 1); }`, ignoring the argv-supplied
   M/K/N entirely and never returning. This is why `chanmgr`'s
   `waitpid(rknpu_pid, ...)` never printed "rknpu test finish" even when
   correctly positioned. Fixed: removed the loop, restored the 5 bounded
   `run_matmul` calls that were dead code below it (matches argv M/K/N).
   **Verified independently**: with this fixed, `/rknpu.srv` ran dozens of
   consecutive `Waiting for IRQ` → `irq 142 handled` → `Woke up from IRQ,
   ret=0` → `Multiplication ... succesful` cycles with zero failures — the
   NPU co-driver design in §4.3 (GIC routes the secure completion interrupt
   straight to the TEE) genuinely works on this hardware.

4. **Moving `/rknpu.srv` + llama-cli into `master()`** (after Linux was up,
   after rknpu.srv's self-test completed cleanly) looked like the fix for #2,
   but broke a different, non-obvious invariant: `usys_tee_wait_switch_req()`
   is a **shared FIFO rendezvous** — whichever TEE thread is blocked in it
   next consumes the *next* SMC from the REE, regardless of who it was
   "meant" for. `master()`'s own two `wait_switch_req` calls (pre-existing,
   unrelated worker-thread/TZASC scaffolding, nothing to do with rknpu or
   llama) ate two SMC slots ahead of llama-cli's own handshake in
   `llama.cpp/examples/main/main.cpp`, so llama-cli received `paddr = 0` and
   the TEE kernel crashed (`BUG: unexpected_handler`, `IP: 0`), which cascaded
   into a full Linux kernel panic ~90s later.
   Real fix: llama-cli must launch directly from `main()`, ending in
   `return 0` **before** `master()` is ever reached — matching upstream's own
   structure. `master()` is self-contained test-harness scaffolding (worker
   thread stress test, commented-out TZASC/CMA experiments) that was never
   meant to run in the same process as the real launch; it stays dead code,
   on purpose. Actual NPU job submission during real inference does not
   depend on `/rknpu.srv` or `master()` at all — `ggml`'s rknpu backend links
   the same `rknpu-driver.c` and talks to the REE via
   `usys_tee_switch_req(SMC_EXIT_SHADOW)`, serviced by `tzdriver`'s own
   FIQ-triggered kernel worker (`smc_smp.c`:
   `fiq_shadow_work_func`/`smc_queue_shadow_worker`) — no userspace REE
   process needs to be running for that path.

5. **`std::bad_alloc` during GGUF vocab loading — NOT YET FIXED.** Current
   blocker. Happens in the TA's own process heap (not TZASC/CMA secure
   memory — that allocation happens later, in `llm_load_tensors`, which this
   run never reaches). Same model (`tinyllama-1.1b-chat-v1.0.Q8_0`, 32000
   vocab tokens) loaded successfully in earlier sessions past this exact
   point, so this isn't an inherent size problem — something about this
   build's process/heap configuration differs. Not yet instrumented.

## Two build-pipeline traps that will silently produce a dead U-Boot

- **The Docker-built `uboot.img` cannot boot this SD card at all.** Its
  U-Boot looks for a GPT partition literally named `boot` and dies with
  `FIT: No boot partition`; this card's GPT (see `rkdeveloptool ppt`) names it
  `boot_linux`. The name is hardcoded in the U-Boot binary next to the
  `FIT: No %s partition` format string — none of the Docker pipeline's own
  builds have ever produced a `uboot.img` containing the string
  `boot_linux`; only a `flash/repack.sh` repack does. Always repack after
  rebuilding; never flash `scripts/kick-the-tires/share_build/images/uboot.img`
  directly. `checkpoints/uboot_repacked.img` is a known-good repacked example.
- **`rkbin/tools/mkimage` (`tools/bin/mkimage-rkbin`) is the only `mkimage`
  that supports `-E` (external FIT data).** `u-boot-orangepi`'s own
  `tools/mkimage` (`tools/bin/mkimage-uboot`) does not; using it silently
  embeds ~55MB of TEE-OS data inline in the FIT's own device tree, which the
  SPL cannot parse (`Synchronous Abort` at `Trying fit image at 0x2000
  sector`, before any hash check even runs). `-p 0x1000` is also required
  (absolute `data-position`, not the default relative `data-offset`) or the
  SPL can't locate the data either. `flash/repack.sh` gets both right.

## Known-unreliable infra (not a code bug, budget time for it)

- **The USB/MaskROM flashing channel intermittently corrupts data**,
  confirmed repeatedly on 2026-07-29 by writing then reading back and getting
  a clean SHA256 mismatch (never a timeout, never a tool error — just wrong
  bytes). Even *reads* are flaky in isolation (~10-20% single-shot corruption
  observed on a 4MiB chunk in one test run). `flash/flash.sh` handles this:
  writes in 4MiB chunks, verifies each with a majority-of-3 readback vote,
  and retries only the bad chunk. Do not trust a single readback as proof of
  a bad write, and do not trust `rkdeveloptool`'s own "successfully" message
  — it only reflects USB transfer completion, not data integrity.
- **`rkdeveloptool rd` does not make the board boot on its own** after a
  flash from loader mode — it just resets to a state where a physical power
  cycle is needed. A 0-byte UART capture right after flashing means "halted,
  needs power cycle", not "bricked". Once real OS/Linux is up, `reboot
  loader` over a shell (UART or hdc) puts the board back in loader mode
  without touching the MaskROM button at all.
- **UART line-editing garbles multi-word `sh -c` style commands** typed
  directly; keep on-device commands short/simple over UART, or (once
  network is up) use `hdc shell` instead — far more reliable, no line
  buffer/FIQ-debugger-trigger issues. `hdc` over WiFi was not gotten
  working this session (`persist.hdc.mode`/`persist.hdc.port.num` params
  don't exist on this build's `hdcd`; the actual TCP-mode trigger for this
  OS image is still unresolved) — worth solving before the next long
  debugging session, it would have saved hours here.
- **The Rockchip FIQ debugger can intercept the UART mid-transfer** on
  large/fast writes (a specific byte sequence appears to trigger it); if the
  console starts echoing `debug>`, send `console` to return to the normal
  shell.

## Architectural point worth knowing before further work

**This artifact is a benchmark harness, not an always-on inference service.**
`chanmgr`'s `main()` launches `llama-cli` exactly once
(`create_process`+`waitpid`+`return 0`) then the whole TEE-OS process is done
with that code path — there is no request loop. The paper's own
`scripts/run-mem-retry.sh` reboots the board before every single test case,
which only works because each benchmark run wants exactly one clean
measurement anyway. The paper's §4.1/4.2 design (partial parameter caching:
"the next inference can resume from the computation stage of the cached
parameters") explicitly implies a persistent, multi-request TA in the
*intended* production design — but wiring that up (a request loop over the
existing `all_ring_buffer_header` ring buffer protocol, reusing
`master()`'s scaffolding or replacing it) is real, additional engineering
not present in this artifact as released. Don't expect "one boot, many
prompts" to work without building that first.

## Directory map

```
tz-llm/                  source (tee_os_kernel, linux-5.10-opi, tzdriver,
                          llama.cpp, drivers_hdf_core_full, vendor_opi5plus_full)
                          tee_os_kernel_REAL/ deliberately excluded: dead tree,
                          oh-builder-hdf.sh bind-mounts tee_os_kernel, not _REAL.
scripts/kick-the-tires/   build scripts + device_opi5plus_REAL (board config)
                          + repack/ (known-good U-Boot binary + ATF/dtb blobs)
tools/bin/                mkimage-rkbin (the one with -E), mkimage-uboot,
                          dumpimage -- prebuilt, don't need to rebuild u-boot
tools/rkdeveloptool/      flashing tool
checkpoints/              uboot_repacked.img + boot.img + SHA256SUMS (latest
                          verified-flashed state, see above)
patches/                  tee_os_kernel_vs_upstream.patch -- the 4 chanmgr/
                          rknpu source changes as a diff, for reference
rebuild.sh                rebuild TEE-OS+kernel (~40 min, Docker)
flash/repack.sh           repack a fresh uboot.img with the known-good U-Boot
flash/flash.sh            verified flash to SD card (see infra notes above)
```

## Repro command for the current bug (#5)

After flashing `checkpoints/{uboot_repacked.img,boot.img}` and letting the
board boot (needs a physical power cycle after flash, see infra notes),
mount the model SSD and trigger the one-shot TEE call:

```sh
mount /dev/block/nvme0n1p1 /data/ssd
mkdir -p /dev/shm
LD_LIBRARY_PATH=/data/local/tmp/rknpu/ /data/local/tmp/rknpu/ld-linux-aarch64.so.1 \
  /data/local/tmp/rknpu/fake -c 0 -l 0 -m tinyllama -n 64 -s 0 > /data/tz.log 2>&1 &
```

Run in background (not foreground on a UART console — a hang holds the tty
and Ctrl+C does not reach it). This is the *only* TZ call this boot will
service; a fresh power cycle is needed to try again.
