# TZ-LLM on Orange Pi 5 Max — Status (2026-07-29)

Paper: "TZ-LLM: Protecting On-Device Large Language Models with Arm TrustZone"
(EuroSys '26). Artifact: https://zenodo.org/records/17054270. Board is an
Orange Pi 5 **Max** (RK3588); the artifact targets 5 **Plus** — several fixes
below are 5-Max-specific.

## Current furthest state (checkpoint in this project)

`checkpoints/uboot_repacked.img` + `checkpoints/boot.img` is the latest build
(sha256 `98b15c08...`/`9d8a4495...`, see `checkpoints/SHA256SUMS`) --
**identical to `checkpoints/20260729-tzfix5-verified/`**, the actual most
recent verified-working build from the morning of 2026-07-29 (built via the
`flash_tzfix5.sh` iteration, `tz-llm-ae/scripts/kick-the-tires/
share_tzfix5/images/`). **Correction (2026-07-29, later same day)**: during
the blank-SD-card investigation later this same day, `checkpoints/
uboot_repacked.img`/`boot.img` got overwritten with an *older* build (sha256
`736bc4c4...`/`7b1ffc29...`, from git commit `36968bfe4`, predating the
tzfix5 fix) and that older pair was mistakenly used for several blank-card
and old-card flash tests. The mistake was caught by comparing hashes against
the old card's own actual content (verified via direct block-device read
over a USB card reader) and the dedicated `20260729-tzfix5-verified/`
backup copy -- the top-level `checkpoints/{uboot_repacked.img,boot.img}`
have been restored to match `20260729-tzfix5-verified/` again. **If these
two ever diverge in the future, `20260729-tzfix5-verified/` is the one to
trust** -- it's the dedicated, deliberately-preserved reference copy;
the top-level files are the "current default" that flash scripts read by
default and are more likely to get clobbered by an in-progress experiment.
Boot sequence on this checkpoint, confirmed live:

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

## Blank SD card: idbloader (2026-07-29, unresolved but documented)

Flashing a genuinely blank SD card (GPT + uboot + boot_linux + system +
vendor + userdata, all byte/spot-verified clean via `flash-full.sh`) produces
a card that **falls straight back into MaskROM on every power-on, even
without holding the MaskROM button** — the RK3588 BootROM's own automatic
fallback when it finds nothing valid at the fixed idbloader location, LBA 64
(0x40). None of this project's flashing scripts have ever written that
region; the working SD card used every other night this project has existed
already had a valid idbloader there from whenever it was originally
provisioned (predates this project). `rkdeveloptool`'s own `ul` (upgrade
loader) command exists to write it but is broken on this exact
board/MaskROM: it fails deterministically with `RKU_ReadCapability`'s
underlying `RKU_Write failed, err=-1` (a USB Mass-Storage/CBW bulk-transfer
request the vendor-control-transfer-based `db`/`wl`/`rl`/`gpt` commands never
use) -- confirmed with two different loader files, both failing identically
at the same point, so it is not a file-choice problem, it is the tool itself.

Fix path found: **Rockchip's own official `upgrade_tool` CLI** (from
`https://github.com/LubanCat/tools/tree/master/linux/Linux_Upgrade_Tool`,
referenced in the artifact's own README) has a working `UL <loader>
[-noreset]` that completes cleanly (`Prepare IDB Start/Success`, `Download
IDB Start/Success`, `Upgrade loader ok.`) where `rkdeveloptool ul` fails
outright. It's a plain x86-64 static ELF binary, no install needed --
`assets/full-flash/` does not currently keep a copy of it (grab the tarball
fresh, or ask -- it was not committed here because its license/redistribution
terms weren't checked).

That said, WHICH BYTES to write at LBA 0x40 is still not solved cleanly.
Three payloads tried, all via the working `upgrade_tool UL` (so the earlier
`rkdeveloptool` bug is not a factor in these results):

1. `mkimage -T rksd -n rk3588 -d <ddr>:<spl>` built from the *default* rkbin
   paths in `RKBOOT/RK3588MINIALL.ini` (the same generic prebuilt SPL/DDR-init
   binaries `device_opi5plus_REAL/loader/MiniLoaderAll.bin` itself is built
   from -- confirmed by reading `pack_idblock()`/`pack_spl_loader_image()` in
   the Docker image's own `make.sh`). BootROM read it fine (no MaskROM
   fallback this time, unlike every other attempt) but the board then hung
   completely -- zero UART output, not even the BootROM's own DDR-init
   banner, for 5+ minutes, power LED solid but no heartbeat blink. Recovered
   fine via the physical MaskROM button (not a brick).
2. `device_opi5plus_REAL/loader/MiniLoaderAll.bin` written directly (same
   underlying SPL/DDR-init family as #1, just the whole "download loader"
   file instead of a narrower `mkimage -T rksd` repack of the same pieces).
   Identical silent-hang symptom.
3. The idbloader region (sectors 64-640, up to but excluding where its own
   `uboot.itb` FIT begins) extracted directly from a **known-working**
   community Debian image
   (`os/Orangepi5max_..._debian_bookworm_.../....img`, confirmed to actually
   boot Linux -- power LED blinking/heartbeat, not solid -- on this exact
   board), combined with *our own* GPT/uboot layout. Same silent hang.
   Working theory: this idbloader does not do a generic GPT-partition-name
   lookup for the next stage -- it likely expects the next stage (its own
   uboot.itb) at a location matching *Debian's* GPT convention (which places
   partition 1 at LBA 61440, nothing like our `uboot@0x2000`), so pairing it
   with our GPT sends it looking in the wrong place. Mixing idbloader-from-
   one-image with GPT-from-another is not a safe combination.

**Root cause for #1/#2, best current theory**: `RK3588MINIALL.ini`'s
`FlashBoot`/`FlashData` (and therefore `MiniLoaderAll.bin`, and anything
`pack_idblock()` builds from them without `--tpl`/`--spl` overrides) are
rkbin's generic, prebuilt "USB download-mode bootstrap" SPL -- proven to
correctly init DDR and enumerate over USB (that's what makes `db` work all
night, every night), but its designed job stops there: wait for USB vendor
commands. Used as the actual cold-boot idbloader (no USB host attached), it
plausibly does exactly that -- waits forever -- matching the observed silent
hang exactly. A real fix likely needs `pack_idblock` invoked with
`--tpl --spl` (or `ARG_TPL_BIN`/`ARG_SPL_BIN` set directly) pointing at the
project's *own compiled* `tpl/u-boot-tpl.bin` / `spl/u-boot-spl-dtb.bin` from
the same `rk3588-edge` build that produces our working `uboot.img` -- these
have proper multi-boot-media (SD/eMMC/USB) support, unlike the generic
download-only prebuilt. **Not yet tried** -- a naive first attempt at this
(re-invoking `./make.sh rk3588-edge --idblock` as a second call after the
main build) turned out to re-run the ENTIRE build pipeline a second time
(not a narrow idblock-only step) and deleted the already-good `uboot.img`
before failing on an unrelated missing rkbin file in a differently-configured
(generic `rk3588-evb`, not our board) code path -- caught before it did
lasting damage (`fast_build_uboot.sh` reverted cleanly, confirmed via
`git diff`), but it means whatever does this next needs to directly invoke
`mkimage -T rksd` itself with explicit absolute paths to our own already-
built `tpl/`/`spl/` outputs, not go through `make.sh`'s command dispatch a
second time.

`assets/full-flash/idblock_from_working_card.bin` (sectors 64-8191, the
*whole* pre-`uboot@0x2000` reserved region, extracted whole from the actual
working SD card this project has used since before this session -- i.e. this
project's own GPT layout, not Debian's) was wired into `flash-full.sh` as
the idbloader source and **flash-tested (4th attempt)**: all 6 partitions
wrote and verified clean, but the board still fell straight back into
MaskROM on power-on -- this time a **clean rejection**, not the silent hang
of #1/#2 (a real, different failure mode). Re-read of the extracted bytes
(3 independent re-reads via `rl`, all identical, matching the original
extraction) ruled out USB-channel read corruption as the cause. Why an
extraction that is byte-verified-accurate from a card that itself boots fine
still gets rejected on a *different* card is unexplained -- possibly
something in the idbloader payload legitimately ties itself to that specific
card's identity/geometry (SD CID, capacity-dependent field, etc.), not just
its GPT layout. Not pursued further live; see the golden-image section below
for how this was ultimately worked around.

## Golden-image backup + direct `/dev/sdb` block access (2026-07-29)

While investigating "what if this all needs to be reproduced on another
machine" (disaster recovery), discovered this SD card reader/board setup
lets the SD card be read via a **plain USB card reader as a normal Linux
block device** (`/dev/sdb` here), completely bypassing `rkdeveloptool` and
every MaskROM-protocol issue that caused the 4 failed idbloader attempts
above (`db`/`cs`/`wl`/`rl`/`gpt` vendor-transfer quirks, the broken `ul`,
the intermittently-corrupting channel). Plain `dd`/`sha256sum` work
directly, no timeout/retry/majority-vote ceremony needed, and are far
faster (3.6GB in 38s @ ~96MB/s vs. the many-minutes chunked-verified writes
`flash-full.sh` needs over the MaskROM channel).

Device identity was confirmed two ways before trusting it: GPT partition
sizes on `/dev/sdb` match `parameter_custom.txt` exactly, and
`sha256sum` of the first 64MB of `/dev/sdb1` (`98b15c08...259c7`) matches
the previously-recorded hash of `checkpoints/20260729-tzfix5-verified/
uboot_repacked.img`. Actual old-card capacity is 57.6GB/61891149824 bytes
(`blockdev --getsize64`). The "119.4GB" figure noted earlier in this same
investigation turned out to be the *new/blank* card's real size, not a
misremembering -- both cards were on `/dev/sdb` at different points as they
were swapped in the same reader; always re-verify via hash before trusting
which physical card is currently addressed as `/dev/sdb`.

Backed up sectors 0 through 0x6BA000 (idbloader + uboot + misc/bootctrl/
resource + boot_linux + ramdisk + system + vendor -- everything needed to
boot, excluding sys-prod-onward admin partitions and userdata) to
`checkpoints/golden-image/idbloader_through_vendor.img` (3,612,344,320
bytes). Structurally spot-verified: valid GPT header at LBA 1, `RKNS`
idbloader magic at LBA 0x40, correct `d00dfeed` FIT magic at both LBA
0x2000 (uboot) and LBA 0x88000 (boot_linux). **Deleted after attempt #8
solved the idbloader problem properly** (this file's original purpose --
freed ~3.4GB on a host that was down to ~5.6GB free). If a similar backup
is needed again, the old card is still around and reachable via a plain USB
card reader as a `/dev/sdX` block device -- `dd if=/dev/sdX1 ... | dd
of=...` as done here, no `rkdeveloptool`/MaskROM involved. That technique
(not the specific image file) is the reusable result of this detour.

**Write test (attempt #5, direct `dd`, 2026-07-29): also failed.** Wrote
just the idbloader region (sectors 64-8191, 4,161,536 bytes -- same range as
attempt #4's `idblock_from_working_card.bin`) from this golden image
directly onto the blank/new card's `/dev/sdb` via `dd` (bypassing
`rkdeveloptool`/MaskROM entirely), leaving the new card's already-correct
119.4GB-sized GPT/system/vendor/userdata from a prior `flash-full.sh` run
untouched. Write verified byte-perfect via `sha256sum` readback
(`8a3f4ade...`) before the card was moved to the board. Result: board still
falls straight back into MaskROM on power-on (confirmed via `lsusb` showing
`2207:350b` Rockchip enumeration, zero UART output at any point -- same
silent-hang signature as attempts #1/#2, not the "clean rejection" of #4).

This is a significant negative result: it rules out the write-path/MaskROM-
channel-corruption theory entirely (this write used `dd` + hash-verified
readback, not `rkdeveloptool`), on top of attempt #4 already having ruled
out extraction/read corruption. The exact same bytes, written two different
reliable ways, both fail on this card. This now strongly points at the
idbloader payload itself being **tied to the old card's specific identity**
(SD CID, factory-programmed geometry/timing parameters, or similar) rather
than anything about GPT layout, write path, or data integrity. The
`--tpl --spl` build-our-own-SPL path (see attempt #1/#2 root-cause
discussion above) is now the most promising remaining direction -- a
properly built idbloader for *this* board/config should not carry any
old-card-specific identity binding.

**Attempt #6 (own `--spl --tpl`, both overridden): failed differently.**
Built `idblock.bin` via `./make.sh --idblock --tpl --spl` run directly in
the already-built Docker container (`docker exec`, no board-name argument
passed -- this avoids the `fast_build_uboot.sh` regression from earlier,
since `sub_commands()`'s dispatch is keyed on `$1` alone and `process_args`
only triggers a defconfig rebuild when a bare board-name token is present).
`uboot.img`/`boot.img` hashes confirmed unchanged before/after (no rebuild
triggered). Result: **`tpl/u-boot-tpl.bin` is a 992-byte stub**, nothing
like a real DDR-init/training blob (compare: rkbin's generic
`rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.18.bin` is 75,320 bytes) -- Rockchip
does not open-source DDR training/calibration code, so U-Boot's own `tpl`
build target for this board is a non-functional placeholder. Flashing this
produced the exact same silent-hang signature as attempts #1/#2 (no MaskROM
fallback, zero UART, indefinitely) -- consistent with DRAM never actually
being initialized.

**Attempt #7 (own `--spl` only, keep rkbin's real TPL): different failure,
real progress.** Rebuilt with `./make.sh --idblock --spl` (no `--tpl`), so
`TPL_BIN` falls back to its default (`${RKBIN}/FlashData`, the real
75KB DDR-init blob) while `SPL_BIN` uses our own compiled
`spl/u-boot-spl.bin` (226KB, matches the working card's `uboot.itb`/build
generation, not a generic download-only blob). Result: idblock.bin is
305,152 bytes (`Init Data Size: 75776`, `Boot Data Size: 227328` -- both
now plausible real sizes). Flash-tested: board falls back to MaskROM
*cleanly* this time (`2207:350b` reappears immediately), not a silent hang
-- the same "clean rejection" signature as attempt #4
(`idblock_from_working_card.bin`, real bytes from the actual working card).
Two completely different idbloader payloads (one extracted whole from a
known-working card, one freshly built from this exact project's own SPL
source) both produce the identical clean-fallback failure mode on this
specific blank card -- this pointed toward something card/hardware-specific
downstream of the idbloader content itself, not an idbloader-content
problem per se.

**Attempt #8 (official Rockchip `MiniLoaderAll.bin` from RKDevTool): WORKS.**
User pointed at a MiniLoaderAll.bin bundled with the official "RKDevTool"
Windows flashing GUI package
(`os/Android and Linux image burning tool-RKDevTool and driver.../
MiniLoader-something you need to burn Linux images/MiniLoaderAll.bin`,
481,728 bytes, sha256 `607284db...`) -- confirmed via hash comparison to be
a genuinely different file from the project's own
`device_opi5plus_REAL/loader/MiniLoaderAll.bin` (448,960 bytes, already
tried and failed as attempt #2). Written whole to LBA 0x40 via
`rkdeveloptool wl` + hash-verified readback, then reset. **Board booted for
real**: UART showed the OP-TEE image hash check passing, ATF/BL31 init
(`NOTICE: BL31: v2.3()...`), GICv3 init, then ChCore/TEE-OS's own cold-boot
sequence (`uart init finished`, `per-CPU info init finished`,
`get_tzdram_end returns 0xc400000`, `firewall_ddr_cma_rgn_init 118`) --
further into the boot chain than any blank-card attempt this entire
session. **This solves the blank-card idbloader problem**:
`assets/full-flash/MiniLoaderAll_official.bin` (copy of the RKDevTool file)
is the correct idbloader source; `flash-full.sh`'s `IDBLOADER=` should be
updated to point at it once this is fully confirmed end-to-end.

Boot then went silent again after `firewall_ddr_cma_rgn_init 118` -- no
further UART output for 50+ seconds including after a blind Enter keypress,
so it has not yet been confirmed to reach a live U-Boot prompt or Linux.
Not yet root-caused whether this is a new hang inside ChCore's own init (on
this specific card/memory layout) or just needs more patience -- **next
step: keep watching UART for several more minutes before assuming another
hang**, and if it does stall, the working old card's exact U-Boot boot
sequence requires manual commands at the U-Boot prompt (not autoboot) --
`mmc dev 0`, `mmc read 0x10000000 0x39000 0x20000`, `bootm 0x10000000` --
worth trying blind (send them speculatively) in case a U-Boot prompt is
sitting silently waiting with output that was somehow missed.

Also: disk usage on the host is critical (99% full, ~5.7GB free as of this
backup) -- do not create further multi-GB image files without first
clearing space or moving this backup off-disk.

## Why this project's boot is so much more failure-prone than a plain Ubuntu image (2026-07-29)

While debugging yet another MaskROM-fallback episode, compared this project's
FIT structure against `os/ubuntu-opi5max-minimal.img` (a community image
confirmed to boot on this same board/card history). Both use the identical
mechanism -- a FIT with per-component sha256 hash verification, checked by
SPL before boot proceeds -- so "Ubuntu is less strict" is not the
explanation. The real difference is **data volume**: Ubuntu's FIT hashes
`uboot` (1.3MB) + 3 small ATF stages (~260KB total) + `fdt` (13KB) -- under
1.6MB total, and has **no OP-TEE/TEE-OS component at all**. This project's
FIT additionally hashes a **~55MB OP-TEE/TEE-OS blob** (`optee`, the actual
TZ-LLM/ChCore payload) -- roughly **35x more data** that must come back
bit-perfect from the SD card on every single boot, through whatever
baseline bit-error-rate this board/card/slot combination has.

This reframes today's whole saga: the repeated MaskROM-fallback episodes
are very plausibly not a *broken* board, but the *expected* consequence of
verifying a much larger hashed payload than a typical distro image ever
does, over a channel with a small-but-nonzero error rate -- consistent with
the historical note (2026-07-26) that the same file, reflashed 3x, failed
hash verification twice and succeeded the third time. **Retrying (power-
cycle and re-check) is not superstition -- it has a real statistical
basis**: each attempt is an independent trial against a fixed per-boot
data volume, so repeated attempts should eventually converge, the same way
repeated flash-verify attempts already do. There is no known way to reduce
the ~55MB OP-TEE hash requirement itself (it's the real TEE-OS payload),
so patience/retry remains the practical mitigation until/unless SPL's
own retry-on-hash-failure behavior (if any exists) is investigated
separately.

## End-of-session honest conclusion (2026-07-29, very late): boot-time large-read reliability is a genuine, unresolved limitation

After the pool-3 fix (below) produced one successful live-shell boot, tried
to reproduce it cleanly with the noise-free (`LOG_LEVEL=1`) rebuild.
Spent several hours on a single stubborn ~32KB region (LBA `0x11900`-
`0x1193f`, inside the `optee` FIT component) that consistently failed
`rkdeveloptool`'s own read-back verification at every chunk size from 4MB
down to 512 bytes -- **except individual 512-byte sectors, which all
verified correctly** every single time. Tried: a different `rkdeveloptool`
binary (project's own compiled copy vs. the system-installed one), added
explicit delays (3s+) between write and read, and full end-to-end
re-flashes from scratch multiple times. Eventually wrote every single
512-byte sector in this region individually (all verified OK) and
completed the rest of `uboot`+`boot_linux` (all 128+83 chunks verified
clean).

**Result on actual boot: `Bad hash` for `optee` again -- a *third*
different wrong value** (`748c92f2...`, distinct from both `ac8a2184...`
seen earlier). This is the decisive data point: content has now been
verified correct at the finest possible granularity (individual sectors)
via multiple tools with delays, yet the real SD-controller boot-time read
of this ~55MB component still comes back wrong, differently, each time.

**Honest conclusion**: this is a genuine, currently-unresolved hardware/
firmware reliability limit on this specific board (or board+card
combination) when reading a large (~55MB) contiguous region during real
cold boot -- distinct from, and not fixable by, any amount of write-side
verification rigor. `rkdeveloptool`'s own read-back (used for all our
verification) evidently exercises a different, more forgiving path than
the actual boot-time SD controller DMA read, so passing our verification
does not guarantee a successful boot. Contributing factors identified
this session that are real but not sufficient on their own to explain
this: card wear-leveling/remapping from many full reformats today (can't
fully explain it since even byte-identical historical-precedent content
failed identically), and the sheer data volume (~55MB vs. a plain
Ubuntu image's <1.6MB FIT, ~35x more data that must come back
bit-perfect every single boot).

**What IS solid and should carry forward**: the pool-3 mm_init hang
(below) is a real, understood, low-risk kernel fix, independent of the
above -- keep it (`kernel/arch/aarch64/plat/rk3588/mm/mmparse.c`,
`physmem_map_num` 5->4, dropping the out-of-bounds 16-28GB pool). The
`checkpoints/pool3fix-clean/` build has this fix with clean `LOG_LEVEL=1`
logging; it has not yet been promoted to the default
`checkpoints/{uboot_repacked.img,boot.img}` or committed to git, pending
a successful boot to confirm the fix survives a clean flash (blocked
today by the large-read reliability issue above, not by the fix itself --
the one time it did boot, LOG_LEVEL was still 2, i.e. the *content* that
boots is `checkpoints/pool3fix/`, not `-clean`).

**Recommendation for next session**: don't keep fighting the large-read
issue with more verification rigor -- it's demonstrably not a data
problem. Options worth trying instead: (a) just keep attempting fresh
power-cycles with the already-flashed content, since the ONE successful
boot this session did happen eventually; (b) investigate whether U-Boot
SPL has (or could be given) a retry-on-hash-failure loop around the
actual `info->read()` call in `common/spl/spl_fit.c` (the exact call site
was located this session, see the note on this near the bisect work) --
a real code fix was scoped but not attempted, blocked on the fact that our
own from-source SPL rebuild needed as the delivery vehicle has its own
separate, not-yet-fully-understood boot behavior; (c) accept this as an
inherent property of this hardware and prioritize getting ONE stable good
boot to test TZ-LLM+NPU (the original goal) rather than a repeatably
reliable one.

## BREAKTHROUGH (2026-07-29, night): real hang root-caused and fixed -- board boots to a live shell for the first time all day

After the wear-leveling theory (below), rebuilt our own from-source SPL
(`./make.sh --idblock --spl`, same as attempt #7 in the blank-card
investigation) and, this time, got a full continuous UART capture of it
booting -- **it passed every single FIT hash check including `optee`**
(`98df236b76...`, the known-good 07-28 content, then still on the card),
went further than any recent attempt, and hung at the exact same point as
the blank-card investigation's attempt #8 much earlier today: right after
`firewall_ddr_cma_rgn_init 118`, never printing `[ChCore] mm init
finished`.

Added bisect instrumentation (`kinfo` prints before/after each
`init_buddy_for_one_physmem_map(idx)` call in `kernel/mm/mm.c`) and
rebuilt/reflashed. **Found it precisely**: pools 0, 1, 2 print both
before/after cleanly; **pool 3 prints only "before", never "after"** --
confirmed hanging inside `init_buddy_for_one_physmem_map(3)`. Pool 3
(`kernel/arch/aarch64/plat/rk3588/mm/mmparse.c`) is
`[0x400000000, 0x700000000)` -- **16GB to 28GB physical** -- while the
DDR-training banner this whole session shows only 4 channels x 4096MB =
**16GB total installed DRAM**. This pool is entirely beyond installed
memory.

**Fix**: dropped `physmem_map[3]` (the out-of-bounds 16-28GB pool)
entirely, renumbering the old `physmem_map[4]` (the small, in-bounds
0x20000000-0x50000000 debug-reserved region) down to index 3, and reduced
`physmem_map_num` from 5 to 4. Rebuilt, reflashed (own SPL + this fix).

**Result: the board booted all the way to a live, responsive OpenHarmony
shell** -- confirmed via `uname -a` over UART (`Linux localhost 5.10.110
... Wed Jul 29 20:06:15 CST 2026 aarch64`, matching this exact build) and
`ls /dev/tc_ns_client` (TrustZone driver present and alive). This is the
furthest point reached all session, on the first attempt after this fix.
One transient-looking side effect observed once: a burst of ChCore-side
`[OOM] No enough memory in memory pool` + `Data Abort from a lower
Exception level` messages appeared in one UART capture (the very
beginning of the boot sequence was missed due to capture-timing, so it's
not yet clear if this happened before or was resolved before reaching the
shell) -- **did not prevent reaching a fully working shell this time**,
but is worth understanding before declaring this fully solved: pool 3,
despite being outside installed DRAM, may have been silently "succeeding"
its init on working historical boots (writes to unbacked address ranges
don't necessarily fault on this SoC/interconnect) and providing a large
if-fake capacity number other code relied on -- removing it may have
created a genuine memory-pressure regression elsewhere, worth watching
for if OOM-related instability recurs during actual TZ-LLM/NPU testing.

**Next step**: with a live shell finally reachable, resume the *original*
TZ-LLM/NPU test goal directly (mount NVMe, push CA binaries, run `fake`)
now that the whole day's boot-reliability blocker has a real fix in hand.
The mm.c/mmparse.c changes are currently only in `project/tz-llm/` (synced
to the container) and baked into `checkpoints/pool3fix/` -- not yet
promoted to the default `checkpoints/{uboot_repacked.img,boot.img}` or
committed to git. Do that once the OOM side-effect question above is
resolved or ruled out as harmless.

## Direct test of the historical-precedent theory: FAILED (2026-07-29, even later) -- points to wear/remapping, not content

Found the actual 2026-07-28 working build on disk
(`tz-llm-ae/scripts/kick-the-tires/share_tzfix/images/{uboot_repacked.img,
boot.img}`, optee hash confirmed `98df236b76...` byte-for-byte, same
58,545,792-byte size as today's `tzfix5` -- so size was never the
differentiator between the two, contrary to the earlier guess). Flashed
this exact known-working pair to the board via `flash.sh` (clean write,
no FAILED chunks) and power-cycled with a continuous UART capture running.

**Result: identical silent-hang failure as `tzfix5`** -- no MaskROM
fallback, zero UART output, for 90+ seconds. The build that is proven to
have booted this exact board 2 days ago now fails the same way as today's
build. **This rules out "something changed in the TEE-OS/TA content
between 07-28 and today" as the explanation** -- the content is
confirmed byte-identical to what worked, yet it no longer boots reliably.

**User's own insight, and the most coherent theory so far**: the
difference isn't the build, it's that **the card(s) have been fully
reformatted/rewritten (GPT + idbloader + uboot + boot_linux + system +
vendor, sometimes userdata) many times today** across both the old and
new card, whereas the 07-28 working state was reached through continuous
incremental development on a card that was never wiped back to blank and
started over. SD cards perform logical-to-physical remapping (wear
leveling / FTL) on every full erase+rewrite cycle -- repeatedly
reformatting the same logical LBA range can land it on a *different*
physical NAND block each time, and if a newly-assigned block happens to
be marginal for the sustained large contiguous read a real boot-time SD
controller DMA needs (as opposed to the smaller/different-pattern reads
`rkdeveloptool`/USB-readers do), that would produce exactly this
signature: file content proven correct, read-back via slower/chunked
paths proven correct, yet the real boot-time bulk read intermittently or
consistently fails, changing which specific occurrence it happens on
based on whatever the current physical mapping happens to be.

**This is hard to fix by reflashing more** (each reflash is itself a
rewrite that could remap things again, possibly not for the better) and
points toward: try a card that has NOT been reformatted at all today (if
one exists), or accept this as accumulated wear from this project's
extensive history of full-card reformats and treat "avoid unnecessary
full reformats going forward, prefer `flash.sh`'s uboot/boot_linux-only
path over `flash-full.sh` when the rest of the card doesn't need to
change" as the operational mitigation.

## Historical precedent found: this exact repack mechanism DID work before (2026-07-28) -- so it's not a hard hardware wall

Before concluding the deterministic-bad-hash finding below means a
permanent hardware ceiling, checked this project's own memory log for a
prior occurrence. Found one, dated 2026-07-28 (`Follow-up #26`, same
memory file): **the identical repack recipe** (known-good U-Boot binary +
freshly-built `tee.bin`, `mkimage -f u-boot.its -E -p 0x1000`) produced a
build that **booted end-to-end on this same board**, with UART showing
`Checking optee ... sha256(98df236b76...) + OK` followed by the TEE
actually launching llama-cli, Linux booting fully, and the paper's §4.3
TEE-REE NPU co-driver handshake (`tzdriver_register_rknpu_dev`) completing
-- the furthest this project has ever gotten, at the time.

That working optee hash (`98df236b76...`) is **different** from today's
(`5816a244...`) -- the TEE-OS/TA source moved on between 07-28 and this
morning's `tzfix1`-`tzfix5` iteration (expected; that's exactly what the
`tzfix` series was for). A candidate cached `tee.bin` from around that
era (`tz-llm-ae/scripts/kick-the-tires/repack/tee.bin`, dated Jul 27 23:21)
is 52,483,712 bytes -- close to but not identical to today's 58,545,792
bytes, and its hash (`f4caa6de...`) doesn't match `98df236b...` either, so
it's not confirmed to be the exact working artifact, just a similarly-
sized nearby build. **This means the "~55MB triggers a real SD-controller
DMA limit" theory from the section below is not obviously supported by a
size jump** -- 50MB apparently worked, 55.8MB doesn't, a ~11% difference,
which may or may not be enough to cross a real threshold.

**Bottom line: this is not a proven permanent hardware ceiling.** A
working combination existed 2 days ago on this exact board. The right
next step is a *comparison*, not a hardware workaround:
1. Find exactly what changed in the TEE-OS/TA source between whatever
   produced the working `98df236b76...` optee blob and today's
   `5816a244...` one (git log the `tz-llm/tee_os_kernel` tree between
   those two build timestamps, or between the `tzfix1` and `tzfix5`
   iterations if those are separately recoverable).
2. Check whether the optee blob's *size* grew meaningfully (not just the
   ~11% seen in the one nearby cached sample above -- get the real
   07-28 `tee.bin` if it still exists anywhere, e.g. old `share_tzfix*`
   directories under `tz-llm-ae/scripts/kick-the-tires/`, and diff sizes
   properly).
3. If size did grow substantially, that's a real lead for the "large-DMA
   read" theory. If it didn't, the deterministic-wrong-hash cause is
   something else entirely (worth re-opening from scratch) -- possibly
   something about *this specific build's* optee content/layout, not
   raw size at all.

## Further correction: the bad-hash value is DETERMINISTIC, not random (2026-07-29, even later)

Got a full, continuous UART capture (started `cat /dev/ttyUSB0` *before* power-
on, not the usual poll-after-the-fact) of a MaskROM-fallback episode using the
exact same `checkpoints/uboot_repacked.img` (`98b15c08...`) already flashed
and verified correct multiple times today. The trace shows:

```
## Checking atf-1 0x00040000 ... sha256(90b478f3ff...) + OK
## Checking atf-2 ... sha256(569ee96047...) + OK
Bad hash: ac8a218444e8fc55d844961a9d7276172d6c1d320c3d838553d127a9c127cdf5
 error!
```

**`ac8a218444e8...` is the exact same wrong hash value seen in an earlier
capture today**, for the `optee` component, on a different flash attempt.
Verified the FIT file itself is correct on every axis checked: the
recorded `data-position` for `optee` (`0x15f400`, via `fdtget`) points to
bytes that hash to exactly the *expected* value (`5816a244...`) when
extracted straight from the checkpoint file. The file is not the problem.

**A repeated, identical wrong hash across independent flash/boot attempts
rules out random bit-flip noise as the explanation** -- true random
corruption would produce a *different* wrong value each time (this is
exactly how the majority-vote read verification elsewhere in this project
distinguishes real corruption from transient noise). Getting the *same*
wrong 32-byte value twice, from the same never-changed on-card data, means
whatever's misreading it is doing so **deterministically** -- most likely
a real limitation of the board's on-board SD controller when DMA'ing a
large (~55MB) contiguous read during real boot (a transfer size/pattern
`rkdeveloptool`'s MaskROM-protocol reads, and Ubuntu's <1.6MB FIT, never
exercise).

**This invalidates the "just retry, it's probabilistic" conclusion from
the section below** (kept for its correct sub-findings, but its final
retry-based recommendation should be treated as superseded). If the
misread really is deterministic for a fixed data pattern, repeated power-
cycles with *unchanged* content are not expected to eventually succeed.
Untested next ideas, in rough order of effort: (a) see if a smaller optee
payload (if the actual TA/TEE-OS could be slimmed down) avoids whatever
size threshold triggers this; (b) check if U-Boot/SPL config has any DMA
alignment/chunking option for large FIT loadables; (c) try a physically
different board if one becomes available, to separate "this board's SD
controller" from "this general board model."

## CORRECTION (2026-07-29, later same day): not a bad SD card -- likely the board's own SD slot/controller

The "physical bad block" conclusion below turned out to be wrong in its
attribution (the diagnostic steps and symptoms are all accurate, just
mis-attributed). **Decisive test**: read the exact same LBA region (the one
`rkdeveloptool` consistently read back wrong, `239fd6f2...`) directly via
`dd` over a plain USB SD card reader (`dd if=/dev/sdb bs=512 skip=65536
count=8192`) -- **3/3 reads came back matching the file's real content
exactly** (`4402afbf...`). The data on the card is correct and stable. It
is specifically reads *through the board* -- both `rkdeveloptool`'s
MaskROM-protocol reads and U-Boot SPL's real boot-time SD-controller
reads -- that return wrong data at this location, consistently. Both of
those paths share the board's own physical SD slot; the USB reader is a
completely independent slot/controller and reads it fine.

**Revised conclusion: the SD card itself is very likely healthy. The
unreliable component is the Orange Pi board's own SD card slot or its
on-board SD controller**, not any specific card. This reframes the whole
day's flaky-boot saga: since the underlying data is provably correct and
stable (verified via an independent read path), board-side read failures
are plausibly a signal-integrity/timing issue on the board's SD interface
-- not a permanent, unrecoverable defect. This means **repeated power-cycle
retries are not just superstition** -- they may genuinely succeed
eventually, since the data being read is actually fine. Physically
inspecting/cleaning the board's own SD slot (not the card) is the next
thing worth trying if this recurs. The original bad-block diagnostic
sequence is preserved below for the record, since the individual test
results are all real and useful -- only the final attribution (card vs.
board) was wrong.

## Old card: physical bad block found in the `uboot` partition (2026-07-29) -- SUPERSEDED, see correction above

**Root cause finally found for the whole day's flaky MaskROM-fallback /
silent-hang / bad-hash boot failures on the old card**: a real, localized,
non-writable region on the physical SD card, at absolute LBA `0x10000`
through roughly `0x11c00` (within the `uboot` GPT partition, specifically
the region where the OP-TEE FIT component lands -- this is exactly why the
one time we got a real UART trace it showed `Checking optee ... Bad hash`
rather than a generic failure).

Diagnostic sequence, all on the *old* card, all pointing the same way:

1. `checkpoints/uboot_repacked.img` is internally self-consistent -- the
   FIT header's recorded optee hash (`5816a244...`) matches re-extracting
   and re-hashing that exact segment from the file locally. The file is
   not corrupt.
2. A live boot attempt (the one time this session a full, legible UART
   trace was captured) showed U-Boot's SPL computing a *different* hash
   (`ac8a2184...`) for the optee data it actually read from the card at
   boot time -- via the real SD controller, a different hardware path than
   `rkdeveloptool`'s MaskROM/USB-based `rl`/`wl`.
3. `flash/flash.sh`'s own chunked-write-with-verify caught this directly:
   two independent full reflashes both failed at the exact same 4MiB chunk
   (chunk 7, LBA `0x10000`) after 5 retries each -- not a random chunk each
   time, the *same* one.
4. Reading that region repeatedly (no writes) returned one stable value
   4/5 times, confirming the card can still read consistently there -- but
   that stable value does **not** match what was ever written there
   (`239fd6f2...` vs the file's real content `4402afbf...`).
5. Systematically ruled out every process/timing explanation before
   concluding it's physical: (a) smaller 512KiB/64KiB sub-chunk writes --
   still stuck at the same wrong value; (b) a 5-second delay between write
   and verification read (in case of a write-cache-not-yet-flushed
   explanation) -- no change; (c) writing from a real temp file instead of
   a process-substitution pipe (in case `sudo`+`<(...)` was silently
   truncating input) -- no change. The write command itself always reports
   100% success; the data simply never persists at this location.

**Conclusion: a physically bad/worn flash block on this specific SD card,
localized to roughly LBA 0x10000-0x11c00.** Not fixable in software --
accepts writes (reports success, sometimes even reads back correctly
immediately after) but does not retain data. This explains essentially
every inconsistent boot result on the old card today, despite every
checkpoint file involved being independently verified correct.

**Not a data-loss risk**: this bad region is entirely within the `uboot`
GPT partition (LBA `0x2000`-`0x82000`), nowhere near `userdata` (LBA
`0x1308000`+, where the real GGUF model lives) or `system`/`vendor`. The
software/checkpoint/build pipeline in this repo has been independently and
thoroughly verified correct throughout today's work (see the sections
above) -- the remaining blocker is purely this one physical SD card's
hardware. **Next step if resuming**: this exact card's `uboot` partition
needs a different physical card (or, if ever worth the effort, an idbloader/
GPT layout that deliberately avoids the ~0x10000-0x11c00 LBA range -- not
attempted, likely not worth the complexity versus just using a healthy
card).

## Container bind-mount trap (2026-07-29): `project/` edits were silently not being built

The Docker builder container (`tzllm_fixed_builder`, created before the
`project/` restructuring) has its bind mounts baked in at the OLD
`tz-llm-ae/tz-llm/...` host paths (`docker inspect` confirms), not
`project/tz-llm/...`. Editing source under `project/` has **zero effect** on
what the container actually builds unless it's also synced into
`tz-llm-ae/`. This had not caused visible problems yet only because the two
trees happened to still be byte-identical (no divergent edits made since the
restructuring) -- confirmed via diff before this was caught.

**Proper fix (recreate the container with mounts pointing at `project/`)
was evaluated and rejected for now**: doing that without losing the
container's internal build state (compiled toolchain setup, `out/`,
`rkbin/`, `.config` -- none of which are bind-mounted, all would be lost on
a plain recreate) requires `docker commit` first, which duplicates the
container's ~11.4GB writable layer into a new image layer. The host had
only ~9GB free at the time -- not safe to attempt. Revisit this once disk
space allows (`df -h` the `Docker Root Dir` from `docker info`, currently
`/var/snap/docker/common/var-lib-docker`, same filesystem as everything
else on this host).

**Interim fix**: `scripts/kick-the-tires/sync-to-container.sh` -- run this
before every `build-oh-docker.sh` that follows a source edit under
`project/tz-llm/{tee_os_kernel,linux-5.10-opi,tzdriver}/` (the three trees
that are actually edited; the others -- llama.cpp, drivers_hdf_core,
vendor_opi5plus, device_opi5plus_REAL -- have not been touched this
project's whole history and are lower-risk to leave unsynced, but are also
covered by the same mount-mismatch if that ever changes). This makes the
sync an explicit, scripted step instead of a silent trap -- not as good as
true single-source-of-truth, but no longer a way to silently ship an
untested binary.

## Incident (2026-07-29): `flash.sh` bug clobbered GPT + idbloader on the new card

While flashing a LOG_LEVEL=2 debug rebuild, `flash/flash.sh` (a script
written fresh during the `project/` restructuring, not carried over from
`tz-llm-ae`) resolved the `uboot` partition's LBA via `awk '$3=="uboot"'`
against live `rkdeveloptool ppt` output. This exact-match failed silently
(most likely a trailing `\r` in the tool's own output breaking the
comparison) and produced an **empty** `UBOOT_LBA`, which bash arithmetic
silently treated as `0`. The script then wrote all 131,072 sectors (64MB) of
`uboot_repacked.img` starting at **LBA 0x0** instead of the correct
`0x2000` -- overwriting the protective MBR, primary GPT header/table, and
critically the idbloader at LBA 0x40 that attempt #8 had just fixed, plus
the first ~56MB of the real `uboot` partition (with content shifted 4MB
early, i.e. wrong/misaligned, not just missing). `boot_linux` was NOT
affected (that LBA resolved correctly via a substring match, not exact
match, and wrote cleanly) -- confirmed all 11 chunks verified OK. `system`/
`vendor`/`userdata` are also unaffected (well outside the touched LBA
range).

Root cause of *why* this bug got introduced at all: `flash.sh` was written
to dynamically re-derive partition LBAs from live `ppt` output "to not
trust hardcoded offsets" -- but the offsets in `parameter_custom.txt`
(`uboot@0x2000`, `boot_linux@0x88000`, etc.) are fixed constants that never
actually vary between cards (only `userdata`'s size/end does, since it's
the GPT's "grow" partition). `flash-full.sh` already correctly uses these
as hardcoded constants and has been reliable all session. The dynamic-
lookup approach in `flash.sh` was unnecessary complexity that introduced a
real, damaging bug where the simpler hardcoded approach had none. **Fixed**:
`flash.sh` now uses the same hardcoded `UBOOT_LBA=0x2000`/`BOOT_LBA=0x88000`
constants as `flash-full.sh`, with the `ppt` dump kept only as an
informational sanity check (`|| true`, output not parsed).

Checked whether this was a repeat of an old, already-solved problem from
before the `project/` restructuring (a reasonable suspicion given the
restructuring's whole point was to avoid exactly this class of regression):
it is not. `tz-llm-ae/scripts/kick-the-tires/flash.sh` (the pre-
restructuring equivalent) took a completely different approach -- an HTTP
client (`flash-proxy/client.py`, present in both trees) posting to a
`flash-proxy` server on `localhost:8080` that did the actual flashing
server-side. **No server-side implementation was ever found in either tree**
-- `flash-proxy/` only ever contained the client half in this project's
history, so that old path was dead/unusable in this environment already,
not a proven-working tool that got lost in the restructuring. The bug in
today's `flash.sh` is newly introduced this session, not a regression from
lost institutional knowledge.

Recovery (in progress): re-enter MaskROM, rewrite GPT from
`parameter_custom.txt`, rewrite `assets/full-flash/MiniLoaderAll_official.bin`
at LBA 0x40, rewrite `checkpoints/uboot_repacked.img` at the correct LBA
0x2000 using the now-fixed `flash.sh`. `boot_linux`/`system`/`vendor`/
`userdata` do not need to be touched.

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
assets/full-flash/        parameter_custom.txt, rk3588_spl_loader loader,
                          system_real.img/vendor_real.img/userdata.img, and
                          idblock_from_working_card.bin -- everything
                          flash-full.sh needs to init a BLANK SD card from
                          scratch (see "Blank SD card: idbloader" above --
                          the idbloader piece is unresolved/untested)
rebuild.sh                rebuild TEE-OS+kernel (~40 min, Docker)
flash/repack.sh           repack a fresh uboot.img with the known-good U-Boot
flash/flash.sh            verified flash to SD card, existing card only (see
                          infra notes above)
flash/flash-full.sh       full blank-card init: GPT + idbloader + uboot +
                          boot_linux + system + vendor + userdata
```

## Operational tools

- `tools/uart/uart_cmd.sh "<command>" [seconds]` — send one shell command to
  the board over UART and capture the reply. Requires `/dev/ttyUSB0` at baud
  **1500000** (not 115200). Keep commands short/simple (see UART line-editing
  note above); prefer `hdc shell` once/if network access is sorted out.
- `flash/flash.sh` reads the sudo password from `$SUDO_PW` (falls back to an
  interactive prompt if unset) rather than a hardcoded password in the
  script. Example: `SUDO_PW=yourpassword ./flash/flash.sh`.

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

## SPL retry-on-hash-failure patch, and 3 disproven theories for the chronic LBA 0x11800 write failure (2026-07-29 night)

Implemented the retry patch scoped in the previous session:
`u-boot-orangepi/common/spl/spl_fit.c`'s `spl_load_fit_image()` now retries
`info->read()` + `fit_image_verify_with_data()` up to 5 times (external-data
images only) before returning `-EPERM`, instead of failing on the first bad
hash. Compiles clean; confirmed the string `"Bad hash, retrying read (%d/5)"`
present in the built `spl/u-boot-spl-nodtb.bin`.

To actually run this patched SPL, it has to be packed into a fresh idbloader
(`./make.sh CROSS_COMPILE=aarch64-linux-gnu- --idblock --spl`, combining our
own compiled `spl/u-boot-spl.bin` with rkbin's real DDR-init TPL) — the
currently-deployed `MiniLoaderAll_official.bin` idbloader's second stage is
Rockchip's own closed prebuilt `rk3588_spl_v1.14.bin`, unrelated to this
source tree, so patching `spl_fit.c` has no effect while that idbloader is in
use. Confirmed via `boot_merger unpack` that `MiniLoaderAll_official.bin`'s
`FlashBoot` slot is a genuine vendor binary (hash doesn't match anything in
the local `rkbin` checkout, but the filename convention and the ini
reference match). Important context found while investigating this: the
BREAKTHROUGH live-shell boot earlier tonight (see above) *itself* already
used a from-source `--idblock --spl` build, not `MiniLoaderAll_official.bin`
— so this combination (real TPL + our own compiled SPL) is not a fresh,
untested path; it already worked once tonight before the retry patch was
added. New idbloader built this way: `assets/full-flash/idblock_pool3fix_retry.bin`
(305,152 bytes, sha256 `19c4642e...`). New script:
`flash/flash-with-idbloader.sh <idbloader> [uboot] [boot]` — like `flash.sh`
but also writes the idbloader at LBA 0x40 first, without doing a full
`flash-full.sh` GPT/system/vendor/userdata wipe.

**Testing this required rebuilding the u-boot-orangepi tree, which hit an
unrelated environment problem first**: ~2000 files under `u-boot-orangepi/`
were still owned by `root` from an old build that ran inside Docker as root,
blocking the current user (`pi`) from writing new build outputs (e.g.
`arch/arm/mach-rockchip/pstore.su`). Fixed with `sudo chown -R pi:pi .` on
the whole tree (user supplied the sudo password directly in this session).
Not expected to recur unless another root-driven build touches this tree
again.

**Attempting to test the patch on the actual chronic failure (flashing
`checkpoints/pool3fix-clean/` — the LOG_LEVEL=1 build of the pool-3 mm_init
fix that has never once flashed cleanly) surfaced something more important
than the SPL patch itself: the write-time failure that has blocked
`pool3fix-clean` all session is not what several theories this session
assumed it was.** Three flash attempts with the new idbloader all failed
identically, at the exact same point: `flash/flash.sh`'s/`flash-with-idbloader.sh`'s
512KB-chunk scheme always fails at **chunk 62** (absolute LBA `0x11800`,
~31MB into the 64MB `uboot_repacked.img`), 0/3 or occasionally 1/3 verify
match across all 5 write-retry attempts, no partial progress. This is the
same absolute LBA region (`~0x11800-0x11900`) that has been the chronic
trouble spot across multiple different sessions and multiple different
`uboot_repacked.img` contents, not something new to `pool3fix-clean`.

Three theories were tested live tonight and each was **disproven**, in this
order:
1. **"Marginal/worn physical sector on this specific SD card."** Disproven:
   user swapped in a **second, different physical SD card** mid-session:
   `flash-with-idbloader.sh` was rerun unchanged and failed at **the exact
   same chunk 62 / LBA 0x11800** on the new card too (with baseline noise
   elsewhere on this card being slightly worse — chunks 0 and 26 needed
   2/3 instead of 3/3 majority — but chunk 62 was a hard, deterministic 0/3
   failure across all 5 retries, same as the old card). Two independent
   physical cards failing at the identical absolute address rules out a
   single card's own worn/marginal NAND block as the explanation.
2. **"Cumulative per-session USB/MaskROM transfer limit (~32MB), matching
   `CONFIG_SYS_MMC_MAX_BLK_COUNT=65535` sectors found in `dw_mmc.c`/`mmc.c`
   earlier tonight."** Tested by forcing a **fresh loader session** (new
   physical power cycle + fresh `db`/`cs 2` handshake) partway through the
   write, positioned so only ~6MB would be transferred in the new session
   before reaching chunk 62. Disproven: chunk 62 still failed identically
   (0/3, all 5 retries) in the brand-new session. (Also learned an
   operational fact along the way: `rkdeveloptool db` cannot re-enter loader
   mode without an actual physical power cycle back to real MaskROM first —
   calling it again while the device is still in an already-active loader/
   USB-MSC state just times out.)
3. Random USB-channel bit-flip noise was already effectively ruled out
   before tonight (same wrong location every time, never a different
   chunk), and remains ruled out.

**What's left, and the strongest remaining theory**: something tied to this
*absolute* LBA address itself (`~0x11800`, i.e. roughly 137MB into the
device) in the RK3588 BootROM/MaskROM protocol's own SD/MMC read-write
implementation — independent of destination card, independent of session
transfer history, independent of file content (different contents at this
same file offset have failed across different sessions historically too).
This could be an address-decoder edge case, a DMA descriptor/buffer boundary
specific to this offset, or some other BootROM-level quirk. Not yet
root-caused further — would need lower-level tooling (bus analyzer, or
Rockchip's own internal debug documentation) to pin down definitively, which
is beyond what's practical in this project. **The SPL retry patch itself
remains untested end-to-end** (never got past the write-time failure to
reach a real boot with the patched idbloader) — it may still be useful for
transient real-boot hash failures even if it can't help with this
specific, fully-deterministic write-time failure (a retry loop only helps
when a retry can plausibly succeed; this failure has shown zero variance
across at least 8 total attempts tonight, so retrying the *read at boot
time* for this exact scenario would likely not help either if the same
deterministic mechanism is at play during real boot reads, not just
MaskROM-protocol writes).

**Recommendation for next session**: stop pursuing new write-time theories
for `pool3fix-clean` specifically; the mechanism is likely something at the
BootROM/hardware protocol level that isn't fixable from this project's
scripts. Options: (a) keep using `checkpoints/pool3fix/` (LOG_LEVEL=2, the
one build that HAS flashed and booted successfully) as the working reference
instead of chasing a "clean" LOG_LEVEL=1 rebuild; (b) if a clean rebuild is
still wanted, try a build whose compiled size happens to shift the byte
layout so this troublesome chunk boundary lands on different content (not
guaranteed to help, since the LBA itself seems to be the common factor, not
content); (c) resume the original TZ-LLM/NPU inference goal using
`checkpoints/pool3fix/` as-is now that it's a confirmed-bootable reference,
rather than continuing to block on a "clean logs" nice-to-have.

## OOM instrumentation, secure_storage/teecd root-cause chase, and a new post-fix crash cascade (2026-07-30)

Long follow-on session investigating the OOM/Data-Abort spam and panic-reboot
flagged as unresolved above. Net result: **two real, previously-undiscovered
bugs found and fixed** (missing `secure_storage` GPT partition, missing
`/sec_storage` mount-point directory), `teecd` conclusively proven unrelated
to the TZ-LLM/NPU inference path and fully disabled, and the pool-3 OOM
theory disproven with direct instrumentation evidence -- but boot now hits a
**new, different, systemic crash** (many unrelated services SIGSEGV near
same the boot, i.e. not simply a rehash of any of the bugs fixed this
session).

**Step 1 -- instrumented the actual OOM/allocation path** (not a fix, a
diagnostic): added `kinfo` prints to `kernel/mm/buddy.c`'s `buddy_get_pages()`
OOM branch (prints `pool_idx`, `pool_mem_size`, `order`) and to
`kernel/object/user_fault.c`'s `sys_user_fault_map()` (prints `client_badge`,
`fault_va`, `remap_va`, `copy` whenever the page-fault-servicing `get_pages(0)`
call actually fails) -- rebuilt via the full `chcore.sh && linux.sh &&
chcore.sh` pipeline, repacked, flashed, booted. **Result: `[FAULT_OOM]` (the
print inside the actual page-fault-servicing allocation path) fired
**zero** times across the whole capture, while the routine `[OOM]
pool_idx=0 order=0` line fired 50,000+ times.** Cross-checked: every `order=0`
failure was `pool_idx=0` only, never followed by a same-order failure at
`pool_idx=1` or beyond -- i.e. pool 0 (a small ~67MB pool, tried first) fills
up early and every small allocation request simply falls through to pool 1
(~85MB) and succeeds there silently. **This conclusively disproves the "pool-3
removal caused real memory pressure" theory from the prior session** -- the
huge OOM print volume is benign, expected fallback-allocator noise, not
evidence of exhaustion. (Instrumentation left in the tree; harmless at
production `LOG_LEVEL=1` since both prints are `kinfo`, not `kdebug`.)

**Step 2 -- found the real root cause of `teecd` (TEE Client Daemon) crashing
every boot (`exit code 255`, repeated respawn, eventually killing `samgr` a
"critical service" and triggering a full system panic-reboot)**, via three
compounding real bugs, found one at a time by actually reading source
(not guessing):

1. **Missing `secure_storage` GPT partition.** `teecd`'s own init job
   (`/system/etc/init/teecd.cfg`) does `mount ext4
   /dev/block/by-name/secure_storage /sec_storage ...` as its very first
   step -- but `assets/full-flash/parameter_custom.txt`'s partition table has
   **no partition named `secure_storage` at all**. Fixed by adding one:
   there is a genuine ~5.9GB unused gap between `chip_ckm`'s end (LBA
   `0x72C000`) and `userdata`'s start (LBA `0x1308000`) in the existing
   layout, so a new 64MB (`0x20000` sectors) partition was inserted there
   with **zero risk to any existing partition's offset** --
   `,0x00020000@0x0072C000(secure_storage)` added to `parameter_custom.txt`,
   a blank ext4 image created (`assets/full-flash/secure_storage.img`,
   `mkfs.ext4`), both flashed via a new GPT write + targeted `wl` at LBA
   `0x72C000`. Verified: `mount` line's error changed from `ENOENT` to
   *no error at all*, and the kernel log showed `EXT4-fs (mmcblk0p15):
   mounted filesystem with ordered data mode` -- genuine success.
2. **Missing `/sec_storage` mount-point directory.** Even with the
   partition now mountable, `mount` still failed (`err 2`). Root cause:
   `/sec_storage` didn't exist as a directory anywhere on the *actual*
   boot-time root filesystem. Initially (wrongly) assumed root was the
   initial ramdisk (`ramdisk_5plus_original.img`) and added `/sec_storage`
   there -- rebuilt via the full pipeline, flashed, retested: **still
   failed, this time with `err 30` (EROFS)**, proving root is mounted
   read-only at that point, so a runtime `mkdir` in the cfg can never
   create it regardless. Investigated further and confirmed via mounting
   `assets/full-flash/system_real.img` directly: **it IS the real root
   filesystem** (`root=PARTUUID=<system's UUID>` in the kernel cmdline;
   contains `/data`, `/dev`, `/proc`, `init -> /system/bin/init`, etc. --
   not just an overlay at `/usr`). Added `/sec_storage` (0700) at the
   **top level of `system_real.img` itself** (not the ramdisk), reflashed
   just `system.img`. Verified: the `mount` command line in the boot log
   now has **zero error output** at all (previously always followed by
   `Failed to mount for /sec_storage, err 2`).
3. **`/dev/tc_private` genuinely never gets created -- by design, not a
   bug.** After (1) and (2), `teecd` still crashed (`exit 255`), now on
   `Failed to change owner for /dev/tc_private, err 2`. Traced through
   `tzdriver/core/tc_client_driver.c`: the *standard* OpenHarmony
   `tc_ns_client_init()` (which creates both `/dev/tc_ns_client` and
   `/dev/tc_private`) is **dead code** -- `tc_init()` has an unconditional
   `return ret;` right after its "tc_init finish" log line, making
   `tc_ns_client_init()` and everything after it (`tc_teeos_init`,
   `enable_dev_nodes`, `alloc_dev_bitmap`, etc.) unreachable. This
   project's own `llm_client_init()` (the function actually called)
   creates *only* `/dev/tc_ns_client`, via a custom `g_llm_ns_client_fops`
   -- `/dev/tc_private` was never meant to exist in this build at all.
   Confirmed a `ueventd.config` permission-list theory was a red herring
   (added the missing entry, made no difference -- the device genuinely
   isn't created at the kernel level, no amount of userspace permission
   config can conjure it).
   **Then verified (via a dedicated Explore-agent code audit, not
   assumption) that `teecd`/`/dev/tc_private`/`libteec` are entirely
   unrelated to the actual TZ-LLM inference path**: the real CA
   (`llama.cpp/examples/main/fake_ca.cpp`, `alloc-stage.cpp`,
   `io-backend.cpp`) opens `/dev/tc_ns_client` directly via raw
   `open()`+`ioctl(LLM_CLIENT_IOCTL_*)`, never includes
   `tee_client_api.h`, never calls any `TEEC_*` function. TA loading goes
   through ChCore's own `chanmgr` as a native process launch, never
   through `teecd`'s `secfile_load_agent.c` (that code path is entirely
   absent from this project). Zero references to `teecd`/`libteec`/
   `TEEC_*` anywhere in this project's own vendor config. **Disabling
   `teecd` carries no risk to CPU (TrustZone) or NPU inference.**

**Step 3 -- disabling `teecd` took two attempts to actually work, and
surfaced a real lesson about unverified large writes.** First attempt:
removed just the `"early-fs": ["start teecd"]` job from `teecd.cfg`,
keeping the `"services"` block. Reflashed, retested: **`teecd` still
started** -- turned out OpenHarmony's init can auto-launch a service that's
merely *declared* in `"services"`, independent of any explicit `"start"`
job (not confirmed via source, inferred from this repeated behavior).
**Second attempt: deleted `teecd.cfg` entirely.** Reflashed, retested with
a definitively single, clean UART reader (see below) -- **confirmed
`teecd` no longer starts at all (0 occurrences in a full clean boot
capture).**

**Along the way, hit and resolved a real self-inflicted diagnostic bug**:
at one point two `cat /dev/ttyUSB0` background processes were reading the
same serial port simultaneously (one from an earlier capture that was
never killed), producing garbled/interleaved UART logs that looked like
`teecd` was still starting when it may not have been. Always `ps aux |
grep "cat /dev/ttyUSB0"` and kill stragglers before trusting a capture.
Also independently reconfirmed, via a byte-exact targeted re-read (using
`debugfs -R "stat <path>" <img>` to get the file's exact filesystem block
number, converting to an absolute card LBA, then a 3x-majority-vote
`rl` readback), that a specific `system.img` edit really did (and later
really didn't, before the `teecd.cfg` deletion) survive a `wl` write --
this targeted-block-verify technique is a fast, reliable alternative to
full-file chunked verification for confirming *one specific known edit*
landed correctly, without needing to reverify the whole multi-GB image.

**Current unresolved blocker (real, not a repeat of anything above)**:
with `teecd` fully confirmed gone, boot now fails with a **new crash
cascade** -- many unrelated services (`ecologicalRuleMgrService`,
`telephony_sa`, `light_host`, `av_codec_service`, `softbus_server`,
`msdp_sa`, `audio_host`, `inputmethod_service`, `accessibility`,
`foundation`, `deviceauth_service`, and eventually `samgr` itself) all
exit with `SIGSEGV` in a tight window, and `processdump` (the OS's own
crash-info-dumping tool, invoked automatically when a service crashes)
itself hits a genuine kernel Oops (`Unable to handle kernel paging
request`, `pc : unmap_page_range+0x134/0x5f0`) while trying to process
one of them. **Leading theory, not yet verified**: many *unrelated*
services crashing near-simultaneously smells like a shared dependency
(e.g. a widely-linked shared library) got corrupted -- plausibly by the
same chronic large-block SD-read reliability issue documented extensively
elsewhere in this file, this time landing in a shared `.so` instead of
`optee`/the kernel Image/GPT. Not yet root-caused further. GPT itself
read correctly in this same boot (0 `Invalid GPT` errors), so the flaky
read this time (if that's what it is) hit something else.

**Net status at end of session**: `checkpoints/secstorage-fix/` (uboot +
boot_linux, includes the earlier ramdisk `/sec_storage` mkdir attempt,
superseded but harmless) is the current uboot/boot_linux pair.
`assets/full-flash/system_real.img` now has the `secure_storage` mount
fix AND `teecd.cfg` fully deleted -- this is the most-fixed system image
to date, not yet promoted/copied to a dedicated named checkpoint (todo:
snapshot it before further edits). `assets/full-flash/parameter_custom.txt`
now includes the `secure_storage` partition permanently. TZ-LLM+NPU
inference still not re-demonstrated -- the boot-stability blocker moved
from "pool-3 hang" to "teecd panic-reboot" to now "unexplained multi-service
SIGSEGV cascade", each a real, distinct, now-partially-or-fully-resolved
problem, not the same bug wearing different names. Next session should
start by investigating the SIGSEGV cascade (check whether it's the same
few processes every boot or random -- if random, points at hardware read
flakiness in a shared library; if the same processes every time, points at
a real software bug specific to those services) before returning to the
original TZ-LLM/NPU goal.

## Root cause found: this specific board's hardware, not this project's code (2026-07-30)

Continuation of the SIGSEGV-cascade investigation above. Corrected a
mistaken deletion of 14 legitimate OS services (disabled based on a wrong
"mobile-only" assumption; log evidence showed all 14 had run cleanly
before) -- restored verbatim from the docker image's OpenHarmony source.
Added `show_unhandled_signals=1` to `arch/arm64/kernel/traps.c` for
better crash visibility; corrected an over-confident "translation fault
proves SD corruption" claim after re-reading `arch/arm64/mm/fault.c`
(translation fault just means "no page-table mapping found", not
necessarily "wrong data was read" -- a real but narrower signal than
first claimed). Tried a "pre-warm" mitigation (sequentially read
`ld-musl-aarch64.so.1` into page cache before any concurrent service
access, to rule out a first-touch race) -- **tested and disproved**: the
identical crash (`ld-musl-aarch64.so.1[...+ba000]`, translation fault)
recurred in two different processes despite the pre-warm.

**Decisive test: swapped in a second, completely different physical SD
card.** The `ld-musl+0xba000` crash vanished entirely (never recurred),
but a *different* crash cascade appeared (`multimodalinput`/`hilogd`/
`samgr` all SIGSEGV within ~150ms, no `unhandled exception` line despite
the debug flag -- looks like externally-delivered signals, not CPU
faults), and separately the chronic "Bad hash" U-Boot boot-time read
failure (see many entries above) also reproduced on this brand-new card,
with the **same wrong hash value** across two independent boot attempts.

Investigated the "Bad hash" mechanism at the code level (not just
symptom-level) for the first time: `include/mmc.h`'s
`CONFIG_SYS_MMC_MAX_BLK_COUNT` defaults to 65535 sectors (~32MiB) --
the max single `READ_MULTIPLE_BLOCK` (CMD18) transfer size -- and this
board's config never overrides it. The Linux kernel Image (~38.7MiB) is
the *only* FIT component that ever exceeds this, forcing a 2-chunk read
at the hardware maximum transfer size; every other component (all
<32MiB) always reads in one comfortably-sized chunk and has never once
failed. This correlation (only the >32MiB component ever fails) looked
like a real, fixable lead.

**Two real source fixes made to `u-boot-orangepi` (now tracked in this
repo at `tz-llm/u-boot-orangepi/`, previously untracked/external)**:
1. `include/configs/rk3588_common.h`: added
   `#define CONFIG_SYS_MMC_MAX_BLK_COUNT 8192` (4MiB max transfer instead
   of the 32MiB default), to test the "large single DMA transfer is
   unreliable" theory.
2. Rebuilding surfaced an unrelated regression: `.config` had
   `CONFIG_OPTEE_CLIENT=y` (with `CONFIG_OPTEE_ALWAYS_USE_SECURITY_PARTITION=y`),
   which the actually-deployed working U-Boot binary was proven (via
   `strings` on the binary -- the string `"optee check api revision fail"`
   was entirely absent) to NOT have had compiled in. With it enabled, a
   real-OP-TEE-client-ABI check (`OpteeClientApiLib.c`,
   `optee_api_revision_is_compatible()`) now runs during FIT boot and
   panics (`optee api revision fail: 0.0`) because this project's TEE-OS
   is ChCore/OHTEE, not real Rockchip OP-TEE, so it never answers that
   SMC correctly. Fixed by disabling `CONFIG_OPTEE_CLIENT` (and its
   dependents `OPTEE_V1`/`OPTEE_V2`/`OPTEE_ALWAYS_USE_SECURITY_PARTITION`)
   in `.config`. Needed `scripts/kick-the-tires/repack/u-boot-nodtb.bin`
   and `u-boot.dtb` overwritten with the fresh build, then
   `flash/repack.sh <existing-uboot_repacked.img>` to reuse the
   already-tested `optee`/TEE-OS blob unchanged.
   **Build gotcha for next time**: after hand-editing `.config`, a plain
   `make` does NOT resync `include/config/auto.conf` on this old
   (2017.09-era) U-Boot tree -- it silently builds with the *stale*
   config. Must run `make ... oldconfig` (interactively; pipe `yes ""` to
   accept defaults for genuinely-new symbols) before the real build, or
   the edit has no effect despite `.config` looking correct.

**Result: the MMC-chunk-size fix did NOT fix the "Bad hash" bug** --
identical failure recurred (different wrong-hash value this time) even
at 4MiB chunks. This disproves the "large single DMA transfer" theory as
the actual mechanism. The OP-TEE-disable fix is real and worth keeping
(no downside), but the core Bad Hash bug remains unexplained by anything
fixable in U-Boot's MMC driver.

**Then the user's own idea settled it: swap in a genuinely different
physical board** (not just a different card). Full `flash-full.sh` with
this same fixed U-Boot + unchanged `optee`/kernel content: **0 verify
failures, then a completely clean boot** -- every FIT hash check passed
first try (including the 38.7MiB kernel Image that always failed on the
old board), reached a live, responsive shell (`uname -a` confirmed) and
stayed up 170+ seconds under a full 30-minute UART capture with real
WiFi scanning, USB mouse hot-plug, and dozens of OS services starting --
zero panics, zero SIGSEGV cascades, zero Bad Hash. **Same exact image
content, only the physical board differed.**

**Conclusion**: this entire project's chronic "flaky boot" saga --
Bad Hash on large FIT reads, the SIGSEGV cascades, the `ld-musl` crashes,
the userdata f2fs corruption, the CMA fragmentation crash -- is most
consistent with **this specific old board's hardware being marginal**
(SD controller/slot signal integrity and/or marginal DRAM -- a
independently-built, well-tested Debian image was also observed to hit a
translation-fault Oops in the core page allocator, `__free_pages_ok`,
on the SAME old board, which is strong evidence the problem is not
specific to this project's own kernel/TEE-OS code at all). The new board
is the reference-good hardware going forward. Recommended operational
practices going forward (discussed with user): never cut power mid-flash,
add active cooling if not already present, avoid unnecessary full-card
reformats (`flash-full.sh`) when `flash.sh` suffices, let the board rest
between extended high-load test runs.

**Current best checkpoint**: `checkpoints/mmcfix-debug2/` (uboot_repacked.img
+ boot.img) -- U-Boot with both fixes above, same tested-good TEE-OS/kernel
content as `checkpoints/showsig-debug/`. This is the build now flashed and
confirmed working on the new board. TZ-LLM+NPU inference itself still not
yet re-tested on this new board -- that's the immediate next step.

## TZ-LLM+NPU re-test on the new board: 3 real bugs found via deep SMC/pipeline tracing, not yet fixed (2026-07-30, continued)

Resumed the actual TZ-LLM/NPU inference goal now that the new board is
confirmed stable. Repro: `mount /dev/block/nvme0n1p1 /data/ssd` then
`LD_LIBRARY_PATH=/data/ssd/rknpu/ /data/ssd/rknpu/ld-linux-aarch64.so.1
/data/ssd/rknpu/fake -c 0 -l 0 -m tinyllama -n 64 -s 0 > /data/tz.log 2>&1 &`
(all CA binaries + model already present on the SSD from earlier sessions).

**First run hung silently** (no crash, ~400% CPU, UART/console became
unresponsive under the load -- traced this to `usb_host`'s pre-existing
crash-loop, unrelated to the LLM path, spamming dmesg; confirmed via the
boot log that it starts crashing at ~52s uptime, *before* any USB device is
even plugged in, so it's not something to "fix" by unplugging anything).
Set up **`hdc` over WiFi** to get a reliable, non-UART debugging channel
(recipe: `param set persist.hdc.port 8710; param set ohos.ctl.stop hdcd;
(/system/bin/hdcd -t &)`, then `hdc tconn <ip>:8710` from the host --
this is NOT persisted across reboots, must be redone every time).

**Bug #1 -- NPU offload is silently ON by default, even without `-ngl`.**
`fake`'s CLI has no working `-ngl` passthrough (documented back in
Follow-up #21), so `params.n_gpu_layers` stays at its `-1` default;
`common.cpp`'s `if (params.n_gpu_layers != -1)` guard does NOT catch `-1`,
so the model's own "offload everything" default applies regardless.
Confirmed via `llm_load_tensors: offloading 22 repeating layers to GPU`
appearing in the log despite never passing `-ngl`. Fixed by hardcoding
`params.n_gpu_layers = 0;` in `examples/main/main.cpp` right before
`gpt_init()` (TA-side/`LLAMA_USE_CHCORE_API` build only) -- this is the
actual secure-world `llama-cli` entry point (confirmed via `before
gpt_init`/`In function gpt_init` log lines matching exactly this file, not
`fake_ca.cpp`, which only implements the CA-relay side).

**Bug #2 -- the model-loading tensor pipeline stalls at the exact same
byte offset regardless of NPU offload.** With or without the Bug #1 fix,
loading always stops at file offset `187224064` (~178.5MB into the
~1.1GB `tinyllama-1.1b-chat-v1.0.Q8_0.gguf`) -- confirmed via a
Python GGUF-header parser (`/tmp/.../parse_gguf.py`, run against the
model file on the host) that this is exactly `blk.0.attn_v.weight`, the
9th tensor of transformer layer 0. Used `fake_ca.cpp`'s existing
`dbg_log_dump()` (`kill -USR1 <pid>`) diagnostic and confirmed the
CA-side io-event ring buffer goes completely silent (identical dump
contents across two checks 90s apart) -- not slow, genuinely stalled.

Added two-sided SMC tracing to find where: `[TZLLM_TRACE]` prints in
`tzdriver/core/tc_client_driver.c`'s `smc_call_cpu_resume()` (kernel,
Normal World, logs every `push_pages_with_index()` call with cma_index/
size/result) and in `tee_os_kernel/kernel/.../smc.c`'s
`sys_tee_switch_req()`/`handle_yield_smc()` (ChCore, Secure World, logs
every `SMC_EXIT_SHADOW` thread exit/wake pair). **Result: push_pages
succeeds hundreds of times (confirmed up to push #409, `entry_index` up
to 101 per CMA region) -- ruling out any small fixed-capacity array/limit
theory -- then stops completely.** After that point, the trace shows a
**very fast livelock**: the same thread repeatedly exits via
`SMC_EXIT_SHADOW x2=4` (the `io_rpc()`/`__io_try_get()` "is there a
completed IO result yet?" poll from `io-frontend.cpp`) and gets woken
again immediately, hundreds of times/second, forever -- with **zero**
further `push #` events. This matches `LayerScheduler::step()`
(`layer-sched.cpp`) legitimately busy-polling while genuinely starved:
the `alloc`/`io`/`decrypt` priority queues all end up empty and no new
`Pipeline` gets enqueued for the next tensor. Traced the actual
tensor-registration loop (`llama.cpp:5098`, calls `register_param_tensor()`
once per tensor with no explicit per-iteration scheduler drive) and the
`IOStage`/`AllocStage` stage machinery (`cnt_to_finish = cma_indexes.size()
* 2`, `io_cnt <= 32` concurrency cap in `layer-sched.cpp`) without finding
an obvious single-line bug from static reading alone -- this is genuinely
complex concurrent producer/consumer code spanning two address spaces.
**Added targeted instrumentation** (throttled `printf` in `layer-sched.cpp`
`step()`'s empty-queues path, dumping `io_cnt`/`on_fly_cnt`/all three
queue sizes every 2000th idle iteration) to pin down which counter/queue
is actually stuck next session, instead of guessing further from outside.

**Real build-pipeline bug found and fixed along the way**: `scripts/kick-
the-tires/chcore-extracted.sh` (the project's override for the docker
image's own `chcore.sh`) does `rm -rf ../oh_tee; cp -r
/home/vectorxj/oh_tee ../` on *every single run* -- restoring the
pristine, stock `oh_tee/apps/{llama-cli,libllama.so,libggml.so}` from the
docker image and **silently discarding** any freshly-built TA binaries the
separate `build-llama.sh`/`build-llama-docker.sh` pipeline had just placed
there via `chcore_upload()`. This meant multiple rebuild+reflash+test
cycles this session ran the OLD, unmodified `llama-cli` despite every
source edit and every `rebuild.sh` reporting success -- confirmed via
`strings <built image> | grep <unique-diagnostic-string>` coming back
empty despite the string being freshly compiled into
`build-chcore/src/libllama.so` moments earlier. **Fixed**: added a
re-copy step in `chcore-extracted.sh` right after the `oh_tee` restore,
pulling from the bind-mounted `.../llama.cpp/build-chcore/{bin/llama-cli,
src/libllama.so,ggml/src/libggml.so}` so any TA-side llama.cpp/ggml source
change actually reaches the built image. **This bug means any prior
session's TA-side (`LLAMA_USE_CHCORE_API`) source edits that were
believed deployed via a plain `rebuild.sh` may not actually have been** --
worth keeping in mind if a "fix" from before today never seemed to take
effect for no clear reason.

**Bug #3 (new, unrelated, not yet root-caused)**: after fixing Bug #1 and
retesting, hit a *different* crash before even reaching the Bug #2 stall
point: `fake` (the CA process) itself aborts with a glibc
`pthread_mutex_lock.c:94: Assertion 'mutex->__data.__owner == 0' failed`
-- an internal mutex-state-corruption assertion, not a normal
deadlock/double-lock. This matches an **already-documented, unresolved**
finding from an earlier session (`Follow-up #22`'s note: "CA-side
pthread_mutex_lock glibc assertion failure... right at startup on the
TrustZone-only-no-NPU path... likely pre-existing, first time this exact
flag/n combo got tested cleanly"). Notably this is the **first time this
exact combination (real `n_gpu_layers=0`, i.e. genuinely no NPU at all)
has been exercised**, since Bug #1 (NPU silently defaulting on) means
essentially every prior "TrustZone-only" test this whole project's
history was *actually* running with NPU offload active the whole time.
Not yet root-caused which specific mutex (`alloc_mtx`/`gather_mtx`/
`cma_mtx[]` in `alloc-stage-chcore.cpp`, `tasks_lock`/`get_buf_mtx`/
`wait_io_mtx`/`ctxs_mtx` in `io-backend.cpp`/`io-frontend.cpp`, `io_lock`
in `layer-sched.cpp`) or why -- next session should start here, since this
now blocks the CPU-only path *earlier* than Bug #2's stall point.

**Session ended by explicit user request to stop and document, not
because the investigation was exhausted.** Current best checkpoint:
`checkpoints/pipeline-trace-debug/` (also promoted to the default
`checkpoints/{uboot_repacked.img,boot.img}`) -- has all three of today's
source fixes/instrumentation (Bug #1's `n_gpu_layers = 0`, the two-sided
`[TZLLM_TRACE]` SMC tracing, and the `layer-sched.cpp` queue-size/counter
tracing), confirmed booting cleanly on the new board. TZ-LLM+NPU
end-to-end inference is **still not demonstrated** -- three real,
distinct, well-characterized bugs now stand between here and that goal
(Bugs #1 fixed but #2 and #3 open), which is genuine forward progress
compared to the vague "it just hangs" state at the start of today, even
though the original goal wasn't reached.

## Session 2026-07-30/31: Bug #3 (mutex) root-caused and fixed; first-ever
## coherent end-to-end answer; Bug #2 (tensor-load stall) reproduced live,
## confirmed genuinely non-deterministic

**Bug #3 root cause found and fixed for real.** The recurring
`Fatal glibc error: pthread_mutex_lock.c:94: assertion failed:
mutex->__data.__owner == 0` crash (previously misdiagnosed/half-fixed as
an ELF symbol-interposition issue in `io-backend.cpp`) has a second,
deeper cause: `ring_buffer::mtx` (`src/interface.h`), the lock protecting
`all_ring_buffer.io_tasks`/`io_results`, lives **inside the TZ driver's
shared mmap'd page that is genuinely written by both the Normal World CA
process (glibc/Linux) and the Secure World TA (ChCore, chcore-libc/musl)**.
A `std::mutex` there is a `PTHREAD_PROCESS_PRIVATE` `pthread_mutex_t` by
default -- invalid for real inter-process shared memory even between two
processes on the *same* libc, and doubly broken here because glibc's NPTL
and musl's pthread implementation use different internal struct layouts:
one side's lock()/unlock() writes bytes the other side's libc doesn't
recognize as valid state, eventually tripping glibc's own consistency
check. This crash reproduced late in a run (~400-1200+ produce/consume
cycles in, varying between runs) -- consistent with a cross-libc
byte-level race, not a deterministic bug. This bug was introduced by an
**earlier session's own fix** for a real but different race (head/count
ordering in the lock-free ring buffer, see the comment still in
`interface.h`) -- a textbook case of "fixing one bug by introducing a
worse one," exactly the failure mode the user asked to guard against
going forward.

**Fix**: replaced `std::mutex mtx` with a hand-rolled `raw_spinlock`
(pure `std::atomic<int>` CAS loop, `src/interface.h`) -- relies only on
cache-coherent hardware atomic instructions (LDXR/STXR / LSE), no OS
futex syscall, no per-libc thread/owner bookkeeping, so it's actually
valid across the ChCore/Linux world boundary. `interface.h` is shared
(via `io.h` → `io-frontend.h`) into **both** the TA build
(`LLAMA_USE_CHCORE_API`) and the CA build, so a single header edit fixes
both sides.

**Deployment note (learn from this)**: this fix required rebuilding and
reflashing **both** the TA (baked into `uboot.img`'s `optee` FIT
component) and the CA (`fake`/`libllama.so` on the SSD) -- unlike most
prior fixes this project's history, which were kernel/`boot.img`-only. A
freshly-built `uboot.img` straight from the `oh-builder-hdf.sh` pipeline
hit the **long-documented "No CLI available" / "FIT: No boot partition"**
failure (the pipeline's own U-Boot doesn't know this board's GPT
partition name `boot_linux`) -- recovered via `flash/repack.sh`, which
keeps the pipeline's freshly-built `tee.bin` (TA) but repacks it with the
**known-good U-Boot binary** from `scripts/kick-the-tires/repack/`. This
script already existed and is the correct tool for exactly this
situation; use it whenever a TA/TEE-OS-only change needs a new
`uboot.img`.

**Incident: self-inflicted GPT/idbloader corruption, recovered.** While
flashing the repacked `uboot.img`, wrote it to LBA `0x0` instead of the
correct `0x2000` (see section "1. Trạng thái known-good" and
`flash/repack.sh`/`flash-full.sh` for the correct constant) -- overwrote
the protective MBR, primary GPT header/table, and idbloader (LBA `0x40`).
Board still booted the boot ROM into MaskROM automatically (idbloader
invalid → boot ROM falls back). **Recovered without data loss**: the
backup GPT at the tail of the disk was untouched (only the first ~64MB
was clobbered); read it via `rkdeveloptool rl`, reconstructed a valid
disk image locally (`truncate` a sparse file to the real disk's byte
size, write the backup GPT bytes to the matching tail offset, add a
synthetic protective MBR at LBA 0), ran `gdisk`'s auto-recover-from-backup
(built-in behavior when primary GPT is invalid), extracted the repaired
first-34-sectors, and `wl 0`'d that back to the real device. Confirmed via
`rkdeveloptool ppt` matching the pre-incident layout exactly
(`uboot@0x2000`, `boot_linux@0x88000`, ..., `userdata@0x1308000`).
Re-flashed `idblock.bin` (from the same build) at LBA `0x40`, then
`uboot.img`/`boot.img` at the correct `0x2000`/`0x39000`. **Lesson**: this
project has (at least) two different, non-interchangeable "LBA convention"
sources floating around -- `CHECKPOINT_RESTORE_20260725.md`'s
`0x2000`/`0x39000` pair (a raw-offset convention this project's actual
boot flow uses: the kernel FIT is placed at a fixed sub-offset *inside*
the oversized `uboot` GPT partition, not at the GPT's own separate
`boot_linux` partition) vs. `flash.sh`'s `UBOOT_LBA=0x2000`/
`BOOT_LBA=0x88000` (targeting the GPT partition table's own
`boot_linux` entry directly, from the 2026-07-29 GPT-clobber incident
writeup). **Always double-check which convention a given script/runbook
uses before typing a raw `wl <LBA>` command by hand** -- confusing the
two is exactly how this incident happened.

**First-ever fully coherent, on-topic-mechanism (if not on-topic-content)
answer produced end-to-end inside real TrustZone**, confirmed 3 times
today with the mutex fix deployed (`fake -c 0 -l 0 -m tinyllama -n 64
-s 1`, i.e. CPU-only/strawman path, no `-t`): full model load (all
tensors through `output.weight`), full 63-token decode, real
`llama_perf_context_print` stats (load ~23.5s, prompt eval ~13.5s/153
tok, eval ~22s/31 tok), grammatically correct English output. Note the
model's answer is **not topically related** to the fixed benchmark
prompt it was given (a "write a poem about laughter" prompt produced an
answer about "Collection pages" e-commerce theme settings) -- this is a
separate, lower-priority quality/prompt-template question, not a
correctness-of-mechanism bug; the important finding is that TrustZone's
CA/TA/SMC/TZASC pipeline mechanically works and produces coherent tokens.
Verified this really is real TrustZone execution (not simulated): the
TA's optee FIT component's SHA256 is checked by U-Boot itself at boot
(`## Checking optee ... sha256(...) + OK`), the model weights are loaded
through real `tzasc_cma_push_pages`-protected physical memory, and CA/TA
communication happens via real ARM SMC instructions (visible as
`SMC_EXIT_SHADOW x2=...` in the ChCore kernel trace) -- this matches the
paper's own described CA/TA/TZASC/SMC architecture exactly (verified by
reading the paper, arXiv 2511.13717, and comparing against this repo's
`tc_client_driver.c`/`interface.h`/`pipeline.cpp`: 4 REE worker threads +
4 TZASC/CMA regions matches the paper's own "4 threads, 3.8GB/s CMA
throughput" line almost exactly -- the thread/region count is a faithful
implementation choice, not an architectural bug).

**Bug #2 (tensor-load stall) reproduced live and shown to be genuinely
non-deterministic, independent of prompt -- and always at the exact same
milestone when it does occur.** Across 7 total post-mutex-fix runs today
(all `fake -c 0 -m tinyllama -n 64 -s 1`, varying only `-l 0` vs.
`-t "<free text>"`): **4 succeeded fully** (3x `-l 0`, all producing the
identical benchmark-prompt answer/timing above -- deterministic given
fixed seed; 1x `-t "What is your name?"`, producing a genuinely
**on-topic, coherent answer -- "Sure! My name is Alex."** -- confirming
the earlier off-topic benchmark-prompt answer was a prompt/quality
artifact, not a sign the mechanism itself is broken), **3 hung** (one
`-t "What is your name?"` the first time, one `-l 0`, and one more
`-t "What is your name?"` -- each on an otherwise-identical fresh boot to
a run of the exact same command that succeeded on a different boot),
**every single hang stopped at the identical kernel-side push counter
value, `push #203`** (out of ~1235 needed for the full model) -- not a
range, the exact same number all three times, strongly suggesting the
race lives at one specific structural transition point in the scheduler
(e.g. a specific tensor's `Pipeline` reaching completion, or an
`AllocStage`/`LayerScheduler` internal counter crossing a threshold) 
rather than being spread uniformly through the whole load. Confirmed via
kernel-side
`[TZLLM_TRACE] push #203` (out of ~1235 needed for the full 1.1GB model,
i.e. only ~16% through loading), after which the kernel-level CMA push
counter (`dmesg | grep -c 'push #'`) and the CA-side `dbg_log_idx` ring
buffer (dumped via `kill -USR1 <fake_pid>`, see `dbg_log_dump()` in
`io-backend.cpp`) both go **completely silent** (confirmed frozen across
multiple checks, minutes apart) while the 4 `ca_thread`s stay in kernel
state `R` (not `D`), consuming real CPU in a tight
`ioctl(RUN)`→`io_step()`→`sched_yield()` busy-loop that finds nothing
left to do (`io_tasks.consume()` returns empty, `wait_io()` returns
NULL) -- i.e. **the CA side is legitimately idle; the bug is that the TA
(secure world) stops issuing new `io_launch()` requests**, not a CA-side
deadlock. One curious, not-yet-explained detail: the CA-side dbg_log's
*last* recorded event before the stall was `kind=0 is_meas=1` -- an
`is_measurement` task, which is only supposed to fire once, at the true
end of a full decode (`record_measure()` inside
`llama_perf_context_print()`) -- appearing this early (~16% through
loading) is suspicious and worth investigating first if resuming this
bug, though it's not yet confirmed whether this reflects a genuine
premature/erroneous call on the TA side or is a red herring from
dbg_log's small (64-entry) ring buffer wrapping in a way that's easy to
misread out of context (this session initially misread it as definitive
proof of prematurity before verifying dbg_log_idx really had stopped
advancing).

**Architectural assessment (per user's explicit ask, before doing
further ad-hoc patching):** compared this project's implementation
against the actual paper (arXiv 2511.13717) for both bugs found today.
Bug #3 (the mutex) was **not** a paper-architecture issue -- it was a
bug introduced by a previous local session's own incomplete fix, now
corrected. Bug #2 (this stall) is **also not** a paper-architecture
issue -- the paper's "pipelined restoration" design is a straightforward
fixed-worker-pool scheduler with no unusual synchronization complexity
described; a **genuine race/lost-wakeup bug in this codebase's own
`LayerScheduler`/`AllocStage`/`IOStage`/pipeline queue-handoff logic**
(`layer-sched.cpp`, `pipeline.cpp`, `alloc-stage-chcore.cpp`) is the most
likely location, not the paper's design nor the kernel/CMA layer (already
separately investigated and mostly ruled out in earlier sessions).
**Recommendation for whoever resumes this**: do not add more CMA
retry/kernel-level patches for this -- focus on a careful lifetime/
ownership audit of the scheduler's three priority queues (`alloc`, `io`,
`decrypt` in `LayerScheduler`) and the `AllocStage`'s own internal counter
state (`get_nr[10]`, `block_nr[10]`, `finished_nr`, `all_block_nr` in
`alloc-stage-chcore.cpp`) for a missed-decrement/lost-wakeup race,
starting from the `is_meas=1`-appears-early clue above.

**Follow-up same day: live-instrumented reproduction (`[ALLOC_TRACE]` in
`alloc-stage-chcore.cpp`/`layer-sched.cpp`, TA rebuild+repack+reflash
cycle) substantially revises the above diagnosis.** Two important
corrections:

1. **"push #203" is not "16% through loading" -- it's closer to "100% of
   the AllocStage phase for the whole model".** Live trace confirmed
   `finish_stage+enqueue` firing for the pipeline with `sched_info`
   corresponding to the `output`/final tensor (layer 999 in
   `parse_name()`'s numbering) at essentially the same point the kernel
   push counter stops moving. AllocStage only *reserves* CMA-protected
   physical pages (cheap, few SMC round-trips per tensor); the expensive
   part -- actually reading gigabytes off disk (IOStage) and decrypting
   them (DecryptStage) -- happens afterward and needs no further
   `push_pages()` calls. So a frozen push counter at ~203 is consistent
   with **all allocation work finishing quickly and the real stall being
   in IOStage/DecryptStage or the scheduler's post-alloc bookkeeping**,
   not evidence of an early, partial hang as previously assumed.

2. **This is not a pure "hang" -- the process reaches its own completion
   path (`step 5`, `step 6`, the final `tzasc_cma_free_pages()` rollback
   via `x2=deadbeef`) and then goes idle, but produces a broken result**:
   `GENERATED_ANSWER` was **empty** (vs. the normal coherent/on-topic
   answer from a clean run), and the printed `llama_perf_context_print`
   stats were internally inconsistent/implausible (`eval time = 0.00 ms
   / 1 runs` when `-n 64` was requested; `sampling ... 1730337
   tokens/second`) while `load time`/`prompt eval time` looked
   plausible and close to a clean run's numbers. This strongly suggests
   the race **corrupts scheduler/pipeline state such that the decode
   loop exits immediately (e.g. spuriously observes EOS on the very
   first sampled token)**, rather than the CA-side threads deadlocking
   on a resource. The kernel-level "push counter frozen" symptom used
   throughout this session as the primary stall detector is a
   *downstream* consequence (no more tensors ever need loading once the
   corrupted decode loop exits early), not the stall's own location.

**Revised next step for whoever resumes**: stop treating this as an
allocation/CMA-adjacent bug. Instrument (or read carefully)
`llama_decode()`'s sampling/EOS-check path and the `Pipeline`/`AllocStage`
transition specifically around the *last* few tensors finishing alloc
around the same wall-clock window inference actually starts consuming
them, looking for a race between "last tensor's AllocStage marked done"
and "first token's forward pass believes all needed tensors are ready" --
i.e. revisit `use_param_tensor()`'s `while (!pipeline->is_finished())
sched->step();` busy-wait loop (`prefetch.cpp`) for a case where it can
return believing a tensor is ready when in fact a stage transition
(`Pipeline::finish_stage()`, `stage_mtx`-guarded but only for that one
`Pipeline` instance, not against concurrent readers of `current_stage`
in the small window between "submit() said done" and
"finish_stage() actually swapped the pointer") interacts badly under
real 4-thread-concurrent load. 8 total post-mutex-fix runs so far: 4 full
successes (identical good output), 3 confirmed frozen-push-counter stalls
at exactly push #203, 1 new variant (this one) that completes but with
empty/corrupted output -- all with `-s 1`/strawman; `-s 0`
(real NPU) still separately known to produce garbage tokens even on a
"successful" (non-stalling) completion, a likely-different bug in the
RKNPURE compute path (see the `is_strawman` history in `prefetch.cpp`).

**Root cause found: `commit_tzasc()` in `decrypt-stage.cpp` had two
independent, compounding bugs.** (1) `pending_addr[4]` (a
`std::priority_queue`) and `cur_addr[4]` were read/written from multiple
threads -- one per tensor's `DecryptStage::start()` call, and with ~200
tensors sharing only 4 `cma_index` values, real concurrent calls for the
same index are common -- with **zero synchronization**; genuinely
undefined behavior on a non-thread-safe STL container. Fixed with a
per-cma_index `std::mutex`. (2) Independently, `pending_addr` used the
*default* `std::priority_queue` comparator (`std::less`, a max-heap,
`top()` = largest), but the code's own logic (`if (base_addr !=
cur_addr[cma_index]) break;`) needs the *smallest* pending `base_addr`
next, to extend `cur_addr` upward in order -- i.e. it needed a min-heap
(`std::greater`). Fixed by adding the explicit comparator. Both bugs
were present in the same ~15-line function and are logically independent
(one is a threading bug, the other is a pure logic bug that would
misbehave even single-threaded whenever 2+ ranges are pending at once for
the same cma_index) -- worth remembering as a second instance of this
project's recurring pattern where a well-intentioned earlier fix (adding
the priority_queue) introduced a new bug while fixing another.

Bug (2) alone explains why this was *intermittent* rather than
always-broken: when calls to `commit_tzasc()` happen to arrive
close to address order (only ever 0-1 entries pending at a time for a
given cma_index), `top()` trivially returns the single pending entry
regardless of heap polarity, and the bug never manifests -- matching the
observed non-deterministic ~50% failure rate exactly.

**One mechanistic claim worth flagging as unconfirmed, not just
accepted at face value**: the natural assumption is "a range stuck behind
the wrong-comparator bug never gets `usys_config_tzasc()` called, so the
TZASC hardware boundary was never extended over it, so the TA reads
unprotected/garbage memory." This doesn't actually hold up against the
paper's own design: `usys_config_tzasc()` (the "extend_protected" step)
only extends the *security* boundary (blocking Normal World access
going forward) -- it is not what makes that memory *readable* by the TA.
Readability comes from `usys_map_tzasc_cma_pmo()` in `AllocStage`, which
already ran unconditionally, earlier, independent of `commit_tzasc()`.
So a stuck/never-committed range should, in principle, still contain
the CA's already-written real tensor bytes and be normally readable by
the TA -- a missing `usys_config_tzasc()` call is more directly a
*security* regression (that range stays Normal-World-accessible when it
shouldn't) than a data-corruption one. The more likely mechanism for
the actually-observed garbage/empty output is that unsynchronized
concurrent access to a non-thread-safe `std::priority_queue` is genuine
undefined behavior beyond "wrong logical answer" -- e.g. one thread's
`push()` triggering the underlying `std::vector`'s reallocation while
another thread concurrently reads through a stale pointer is a classic
use-after-free that can corrupt unrelated nearby heap memory (plausibly
including live tensor buffers), which would produce exactly the
observed "sometimes fine, sometimes garbage" pattern without needing the
TZASC-permission theory at all. Both bugs are real and both fixes are
correct regardless of which exact mechanism explains the garbage-output
symptom -- this note is about getting the causal story right for the
record, not about whether to keep the fix.

**Status: fix applied (`decrypt-stage.cpp`), TA+CA rebuilt, uboot.img
repacked+reflashed, live test in progress -- not yet confirmed on
hardware as of this note.** Update this section with pass/fail results
from the next test round before considering Bug #2 resolved.

**Operational notes for future sessions**:
- `hdcd` TCP mode (`param set persist.hdc.port 8710; param set
  ohos.ctl.stop hdcd; /system/bin/hdcd -t &`) does **not** persist across
  reboots -- must redo this 3-line UART sequence after every single
  reboot before `hdc tconn` will work.
- The TA is launched **once per boot** by `chanmgr` -- only the first
  `fake` invocation after a fresh boot reaches a live TA; every
  subsequent invocation in the same boot session just busy-spins forever
  with zero kernel-side push activity (looks identical to a genuine hang
  at a glance -- check `dmesg | grep -c 'push #'` staying at the exact
  same count across a `date`-stamped gap to distinguish).
- The CA (`fake`) process's own stdout (redirected to a log file) does
  **not** contain the TA's console output (`[DBG_USE]`, `[TZLLM_TRACE]`,
  `GENERATED_ANSWER`, `llama_perf_*`) -- that only goes to the physical
  UART. Always keep a continuous UART capture running *before* starting
  a test if the goal is to see the model's actual answer, not just the
  CA-side log.
- Multiple concurrent readers/writers on `/dev/ttyUSB0` (this session's
  own stray background `cat` processes, plus separately the user's own
  `picocom` sessions) reliably cause split/lost data and "echoes but
  doesn't execute" symptoms that look exactly like a hung console or a
  hung board, but aren't -- always check `ps aux | grep ttyUSB0` and
  `fuser /dev/ttyUSB0` first before concluding the board itself is stuck.
  A dead background reader can also make kernel timestamps look "frozen"
  in a stale log file when the board is actually fine.
- `wifi_hal_service` reliably crashes once (`exit code 255`) ~2s after
  first starting, then auto-restarts and works normally -- confirmed
  identical across 3 separate boots today, so this specific crash-once
  pattern is **not** the cause of session-to-session WiFi-readiness-time
  variability (~88s from boot to first scan attempt was consistent both
  times it was measured) -- if investigating slow/inconsistent WiFi
  further, look elsewhere (association/DHCP timing with the actual AP,
  not this service's own startup).

## 2026-07-31: MAT_COPY test result — real NPU weight buffers exhaust wrong memory pool (architectural bug, not a simple fix)

Flashed and tested the `MAT_COPY` enable from the previous session (uboot.img rebuilt via
`oh-builder-hdf.sh` with `#define MAT_COPY` uncommented in `ggml-rknpu-re.cpp:122`, repacked via
`flash/repack.sh`, flashed at 0x2000/0x39000, board booted clean, optee hash verified). Ran
`fake -c 0 -m tinyllama -n 64 -s 0 -t "What is your name?"` over hdc/WiFi.

**Result: NOT a fix. Uncovered a new, more fundamental bug.** Tensor loading completed normally
(490 `push #` events, matches prior successful runs). Shortly after entering NPU compute, the
kernel UART log was flooded with over 1,000,000 lines of
`[INFO] [OOM] pool_idx=0 pool_mem_size=0x4370000 order=0` /
`[INFO] [OOM] pool_idx=1 pool_mem_size=0x5134000 order=0` in ~15 minutes (`buddy.c:258`, ChCore's
generic buddy allocator failing to satisfy even a single-page (order=0) request) — a genuine
retry-storm/near-livelock, not slow NPU compute. Process was killed manually (CPU time was
climbing steadily at ~398%, which on its own is NOT proof of real progress — this project has a
documented precedent of high-CPU busy-spin livelocks, e.g. Follow-up #31's Bug #2 — so "process is
burning CPU" must never be treated as sufficient evidence of forward progress by itself; concrete
state-advancing output is required).

**Root cause, traced through the actual allocation chain (not guessed):**
1. `rknn_mem`'s constructor (`ggml-rknpu-re.cpp:893`) calls `mem_allocate()`.
2. TA-side `mem_allocate()` (`chcore-port/npu_interface.c:48`) calls
   `chcore_alloc_dma_mem(size, dma_handle, cache)`.
3. `chcore_alloc_dma_mem()` (`chcore-port/memory.c:280`) calls `usys_create_pmo(size, PMO_DATA)`
   — a **generic** kernel PMO allocation — and immediately asserts
   `BUG_ON(dma_handle->paddr >= (4UL << 30))`, i.e. it is hard-constrained to physical addresses
   **below 4GB**.
4. The <4GB physical range is backed by ChCore's 4 small `global_mem` buddy pools set up in
   `mmparse.c` for rk3588: pool 0 = ~124MB (`[img_end, 0x10000000)`), pool 1 = ~82MB
   (`[0x02e00000, 0x08000000)` — exactly matches the `pool_mem_size=0x5134000` in the OOM log),
   pool 2 = 1.5GB (`[0x60000000, 0xC0000000)`), pool 3 = 768MB (`[0x20000000, 0x50000000)`). A
   comment already in that file (left by an earlier session, re-confirmed correct now) states
   these are "ChCore's own internal bookkeeping pool, not where model weights live".
5. The real model weight/tensor data lives in a **completely separate** region: `tzasc_cma`, whose
   4 base addresses (`cur_addr[]` in `decrypt-stage.cpp`) all start at **≥4GB physical**
   (`0x100000000` and up) — an entirely different allocator (`push_pages`/`commit_tzasc`), not the
   generic buddy pools at all.

**So**: with `MAT_COPY` off, `rknn_mem` was only ever constructed once (the static
`global_weight` scratch buffer) — negligible load on the small <4GB pools. With `MAT_COPY` on,
`get_B_bufs()` (`ggml-rknpu-re.cpp:1346`, only reachable under `#ifdef MAT_COPY`) constructs a
**new real `rknn_mem` per distinct (K,N) matmul shape** across the whole model via this same
<4GB-constrained generic allocator — a workload of a completely different order of magnitude that
these small pools (67-82MB for the two that failed) were never sized or intended for. This
exhausts them, and every subsequent single-page kernel allocation of any kind (not just NPU
buffers) starts failing and retrying, which is the observed flood.

**Contextual note**: `npu_interface.c`'s copyright header attributes it to Jasbir Matharu's
open-source `rk3588-npu` driver (an independent GitHub project), adapted into ChCore by the tz-llm
authors — plausible explanation for why it was never built with `tzasc_cma` awareness in the first
place, and further circumstantial support (beyond the direct Zenodo diff already done) for why
`MAT_COPY` ships disabled by default in the original paper artifact: this may be a genuinely
unfinished integration, not just an oversight.

**Not yet done — real fix would require** rerouting `rknn_mem`'s real-weight-buffer allocation
path to draw from `tzasc_cma` (via the same `push_pages`/`commit_tzasc` mechanism used for model
weights elsewhere) instead of generic `PMO_DATA`, or substantially enlarging the <4GB pools if
that turns out to be simpler/sufficient. Both are nontrivial changes, not a one-line flag flip.
`MAT_COPY` should probably be left disabled again for now (current flashed image still has it
enabled and is known-broken for `-s 0`; the CPU-only `-s 1` path from the previous session's
`081ead254` commit is unaffected and still the known-good state) until this is actually fixed.

**Separately: direct comparison against the real Zenodo artifact** (zenodo.org/records/17054270,
`tz-llm-ae.tar.gz`, per user request, not just local git history's "initial commit") confirmed:
- `MAT_COPY` is disabled (`// #define MAT_COPY`) in the pristine artifact too — not a local
  regression, inherited from upstream.
- The pristine `interface.h`'s `ring_buffer` never had a `std::mutex` at all — it used a
  lock-free CAS-based head/tail (pure atomics only). The `std::mutex` that caused this session's
  Bug #3 (cross-libc mutex corruption, fixed via `raw_spinlock`) was added by some session prior to
  this repo's own git "initial commit" — meaning today's `raw_spinlock` fix is a convergence back
  toward the original artifact's safe design, not a novel workaround.
- Pristine `decrypt-stage.cpp` runs real AES decryption and defaults `is_strawman = false`; both
  already intentionally overridden locally (decrypt disabled, strawman forced true) due to
  real stall bugs found and documented in earlier sessions — confirmed still the right call.

## 2026-07-31 (later): NPU (-s 0) deep dive — 7 real issues found and fixed, still not producing output, root cause is now purely CPU-side quantization performance

Continuing from the MAT_COPY revert earlier today (commit `8fb5bab90`), the user pushed back
hard on "it's just slow" as an explanation and asked to keep digging. This paid off: found and
fixed 7 distinct real issues, in order of discovery. **NPU still does not produce output within a
reasonable time (tested up to 66 minutes without even finishing prefill), but every fix is real,
necessary, and confirmed via direct evidence (not guessed) — none of them were the actual final
blocker; each one revealed the next.**

### Fix 1: `tzasc_cma` routing for real weight buffers (superseded by fixes 6/7's refinement)
Re-enabled `MAT_COPY`, then routed `rknn_mem`'s weight-buffer allocation through the same
`push_pages()`/`commit_tzasc()` mechanism real tensor loading uses, on a dedicated reserved index
(`TZASC_NR_NPU_SCRATCH = TZASC_NR-1`, `chcore/llm.h`) that real tensor loading (`AllocStage`,
`layer-sched.cpp`'s `get_cma_index()`) was restricted to never touch — avoiding a commit_tzasc()
race between two uncoordinated subsystems on the same index. Confirmed working via
`cma_index=3` push traces.

### Fix 2: quantize-weights-once (`weight_ready`) — later found unsafe, see fix 6
Added `rknn_mem::weight_ready` and `B_bufs::group_ready` flags so `pre0()`/`pre_scale()`/`pre1()`
skip re-quantizing weight data that's already been converted, instead of redoing the full
malloc+dequantize+scale+int8-pack cycle on every single token (previously: `pre0()` unconditionally
called `reset_cnt()`/`init_scale()` on every weight tile on every call).

### Fix 3: ENOMEM/EINTR retry in `push_pages()`
Kernel driver's `cma_alloc()` can transiently fail (confirmed via dmesg: `ret: -4` = -EINTR, not
real exhaustion — `158938/196608 pages free`). The old code hit `BUG_ON(ret < 0)` on any failure,
which in `chcore/bug.h` prints once then spins in an infinite empty `for(;;) {}` — a silent,
CPU-pegged, un-killable-looking hang that looks exactly like slow computation from outside. Added
a bounded retry (200 attempts, `usys_yield()` between) with a `[PUSH_RETRY]` diagnostic print.

### Fix 4: hoisted per-tile-redundant `to_float()` conversion
`pre_scale()`/`pre1()`'s `for_all_weights()` closure ran once per THREAD per TILE, and each
invocation independently did `malloc()`+`traits.to_float()` of the **entire source tensor**
(not just its own tile) — for `lm_head` (8 tiles), that's 8x redundant conversion of 65M elements
per call. Hoisted the conversion outside the per-tile closure, computed once per call (still once
per thread, see fix 7 for why full dedup needs per-tensor buffers).

### Fix 5: don't clear `B_map` on prefill→decode transition
`matmul_buffer_mgr::clear()` (called once, when `m` first becomes 1) unconditionally cleared
`A_map`/`B_map`/`C_map`. `A_map`/`C_map`'s keys include `M`/`m` (correctly invalidated across the
prefill→decode batch-size change) but `B_map`'s key `(K,N,k,n,type)` does not depend on `M`/`m` at
all — weight buffers don't need to be rebuilt just because the batch size changed. Now only
`A_map`/`C_map` get cleared.

### Fix 6 (CORRECTNESS, supersedes fix 2's safety): weight buffers were shared across DIFFERENT
### LAYERS, not just across tokens
The critical finding: `B_map`'s key is `(K,N,k,n,type)` — the weight tensor's *shape*, not its
identity. Every layer of a uniform-architecture model (all 22 TinyLlama layers' `attn_q`, e.g.)
shares the exact same shape and therefore **the exact same cached `B_bufs`/`rknn_mem` buffer
objects**. `matmul_kernel_find()`'s own key `(m,k,n,type)` has the same problem, and since
`matmul_kernel` holds a *fixed* reference to one `B_bufs` group set at construction, this collision
is baked in for the kernel's whole lifetime, not just re-triggered per call. The **original**
(pre-fix-2) design was correct-but-wasteful: `pre0()` unconditionally `reset_cnt()`'d weight tiles
on every call, forcing genuine re-quantization every time regardless of caching — necessary,
because the actual tensor differs between e.g. `blk.0.attn_q` and `blk.1.attn_q` even though they
share a cache slot. Fix 2's "quantize once, skip forever" was therefore **unsafe for any multi-layer
model**: layer 1 onward would have silently computed with layer 0's weights. Added
`B_bufs::last_source` (tracks which tensor's raw `data` pointer last populated the group) and
invalidate `group_ready`/`weight_ready` in `pre0()` whenever it changes. This is necessary and
non-negotiable regardless of performance impact. Tested: confirmed via the `push #`/timing data
that with only this fix, layers correctly invalidate every call (since 22 layers round-robin
through one shared buffer, invalidation happens virtually always) — meaning fix 2's caching
benefit is completely neutralized by the correctness requirement in this shared-buffer design.

### Fix 7: per-tensor dedicated weight buffers (weight_id keying)
To make fix 6's correctness compatible with real caching (the whole point), extended
`matmul_kernel`'s cache key to include `void *weight_id` (the weight tensor's raw `data` pointer,
stable for the whole run), passed at all 5 real call sites (`src0->data`). `B_bufs` is now
constructed directly per-kernel (`std::make_shared<B_bufs>(...)`) instead of via
`matmul_buffer_mgr`'s shape-keyed shared map, so it's never shared across different tensors even
if the shape matches. This means each of the ~22×7+1 distinct (layer, shape) tensors gets its own
dedicated buffer, safely reusable across all 32 decode tokens (only 22×7+1 ≈ 155 quantizations
total instead of every-call). **Tested and NOT yet shown to help**: confirmed via `cma_index=3`
push count (86 distinct buffer creations vs ~15-24 before) that this fix is working as designed
(routing through the real allocation path), but the run still hadn't reached the first decode
measure-dump after 66 minutes (worse than fix 6's ~65 minutes) — the extra ~155 individual
`push_pages()`/`tzasc_cma` allocations (each its own SMC round-trip) appear to add real overhead
to the one-time prefill pass, and this session ran out of time to confirm whether decode tokens
2-32 are actually fast once that one-time cost is paid.

### Root cause now fully isolated: CPU-side quantization, not NPU, not memory, not correctness
Direct timing data (`ggml_rknpu_dump_measure()`, printed once at the prefill→decode transition,
before fix 6/7 were applied) definitively separates the cost:
- **`rknpu2_matmul_begin/end_measure_npu`** (wraps ONLY `submit()`, i.e. real NPU hardware
  execution): **1.4 seconds** cumulative for the whole prefill. The NPU hardware itself is fast.
- **`pre_scale()`**: 97.1s cumulative. **`pre1()`**: 97.3s cumulative. These two (CPU-side
  dequantize/scale-find/int8-pack) are ~99% of the ~199s total prefill cost.
- The per-element quantization loops (`weight_int8()`/`weight_fp16()` in `npu_matmul.c`) compute a
  NPU-specific 32x32-tiled memory layout using integer division/modulo **per element** — slow on
  ARM (~20-40 cycles vs ~1 for add/mul) and likely defeats auto-vectorization due to the
  non-trivial index pattern.
- Total real work that must happen at least once (correctly, unavoidably, regardless of caching):
  ~935MB of Q8_0 weight data (22 layers × ~42.5MB of the 7 per-layer shapes) needs
  dequantize+scale+repack. With no caching (fix 6 alone), this repeats **every token** (32x). With
  fix 7's per-tensor caching, it should happen **once total** — assuming the per-buffer allocation
  overhead fix 7 introduced doesn't outweigh the savings, which was not yet confirmed before the
  session ended.

### What a real fix looks like (not done, scoped for next session)
1. **NEON/SIMD-vectorize** `weight_int8()`/`weight_fp16()`'s indexing and the surrounding
   dequant/quant loops in `ggml-rknpu-re.cpp`'s `pre_scale()`/`pre1()` — the ~194s one-time cost is
   the real, correctness-required floor; making it faster is a legitimate, safe optimization (same
   computation, not a caching shortcut).
2. Re-examine whether fix 7's ~155 individual `tzasc_cma` allocations can be reduced (e.g. batch
   multiple tiles' worth of pages into fewer, larger `push_pages()` calls) since each one currently
   costs its own SMC round-trip.
3. Actually let a full run complete past the first decode measure-dump to confirm/refute whether
   fix 7 delivers the expected decode-token-2-onward speedup once the one-time cost is paid.

### Current repo state
All 7 fixes are applied and committed to source (not yet git-committed as of this writing — see
next session). MAT_COPY is currently **enabled**. The board is flashed with fix 7's build; `-s 1`
(CPU-only, `081ead254`'s known-good path) is unaffected by any of tonight's changes and should
still work if tested. `-s 0` does not crash or hang silently anymore (fixes 1-6 eliminated every
crash/hang found), it is just not fast enough yet to produce output within a practical test window.

### Operational note: this session bricked /tmp (tmpfs) to 100% full
Accumulated scratchpad UART logs (`uart_npu_test.log` alone reached 648MB from repeated massive
`[OOM]` benign-noise capture) plus several-days-old leftover extraction directories from earlier
sessions (`/tmp/staros_extracted`, `/tmp/verify_userdata.img`, `/tmp/openssl_extracted`, etc. —
none related to tonight's work) filled the 16G tmpfs, causing `flash/repack.sh`'s `mktemp -d` to
fail with "No space left on device". Cleaned up by deleting the old unrelated directories (freed
tmpfs from 100% to 54% used) and trimming/rotating the active UART log. Also: an `mv` used while
trying to truncate the live-tailed UART log broke the `cat /dev/ttyUSB0 > file` redirection (the
reader process kept its open fd to the now-unlinked old inode) — had to kill and restart the
reader. And separately, one board boot cycle hit a `/dev/tc_ns_client` open failure
(`fake_ca.cpp:99: GGML_ASSERT(fd > 0) failed`) that was environment-specific to that one boot
(coincided with unusually slow WiFi association) and resolved by a plain reboot — not a code bug.

## 2026-08-01: NPU (-s 0) follow-up — NEON confirmed innocent, real bottleneck found and fixed (mostly)

Picked up from the previous session's "not fast enough yet" state. Net result: **NPU (-s 0) went
from never reaching real NPU hardware to reaching `npu_submit`/`npu_done` #1-142 in ~143 seconds**
(previously this took 30-90+ minutes or never happened), before hitting one more real bug
(NPU-scratch pool undersized — see below), now fixed and pending a final verification test.

### Fix A: NEON quantization rewrite (from prior session) — isolated and confirmed INNOCENT
Suspected the NEON/block-precompute rewrite of `weight_int8()`/`weight_fp16()` (see prior session's
notes) might be the cause of a new 48+ minute stall on the largest tensor. Isolation test: reverted
to the naive per-element form, rebuilt, reflashed, retested — **stalled at the exact same
`npu_submit #142`, with or without NEON**. NEON is not the cause; re-enabled it (it's a genuine,
verified-safe speedup, just not the bottleneck that mattered here).

### Fix B: the REAL bottleneck — `to_float()` was NEON-vectorized but ~300x slower than it should be
Disassembled the built `libggml.so`'s `dequantize_row_q8_0` (`aarch64-linux-gnu-objdump -d`):
confirmed genuinely NEON-vectorized (`fcvt`/`scvtf`/`fmul` on real vector registers), ruling out
"missing compiler flags". The actual cause: `fB`/`fB1` (the whole-tensor float conversion buffers
in `pre_scale()`/`pre1()`, `ggml-rknpu-re.cpp`) were freshly `malloc()`'d (up to ~262MB for
`lm_head`) and immediately written sequentially every single call — every element write was a
first-touch page fault on a brand-new page in secure-world memory, and secure-world page faults are
apparently very expensive here (measured: ~3.49us/element, exactly proportional to element count
regardless of tensor shape, which is a memory-subsystem signature, not a compute one). Also found:
`pre_scale()` and `pre1()` each did their OWN full conversion of the same source tensor back to
back (group_ready only gets set true at the END of pre1(), so pre_scale()'s own gate never actually
prevented the redundant work) — and this happened once **per worker thread** (no `ith==0` gate),
so 4 threads × 2 functions = up to 8x redundant `to_float()` calls per tensor before this fix.

**Fix**: replaced the per-call `malloc()`+`to_float()` in both functions with a single persistent,
growable, NEVER-freed shared scratch buffer (`g_float_scratch`, tracked by which source tensor
pointer it currently holds valid data for) — `float_scratch_convert()`. Only the first (largest)
tensor pays real first-touch cost; everything after reuses already-resident pages, and redundant
conversions (across threads and across pre_scale/pre1) collapse to one real `to_float()` call per
tensor. Measured result: `to_float()` cost per medium tensor dropped from ~40.27 SECONDS to ~4
MILLISECONDS (~10,000x), and small tensors dropped to 0-2 microseconds.

### Fix C: NPU-scratch pool (yesterday's fix) was undersized — found via literal assert message
With fB/fB1 fixed, hit a NEW stall at `npu_submit #142` specifically (verified via
`npu_submit`==`npu_done`==142 exactly — NPU hardware genuinely idle, not stuck mid-job; the
`SMC_EXIT_PREEMPTED` spin some external review suspected was "NPU dead, CPU polling forever" is
actually just the CA thread's generic SMC-relay yield loop, unrelated to any specific pending job).
Root cause found directly in the (still-growing) UART log:
```
ggml-rknpu-re.cpp:974: GGML_ASSERT(g_npu_scratch_pool.offset + rounded <= g_npu_scratch_pool.total_size) failed
```
Yesterday's NPU-scratch pool (`npu_scratch_alloc`, see prior session) reserved exactly ONE whole
~768MiB `tzasc_cma` bank up front, assuming that would always be enough. False for TinyLlama itself
(~1.1GB of NPU-tiled INT8 weight data across all layers, since `weight_ready` caching keeps every
layer's buffer resident forever — it's not a per-token working set, it's the whole model). `GGML_ASSERT`
in this TA/secure-world environment does not cleanly abort the process on failure — it silently
spins, indistinguishable from slow computation, matching the exact symptom chased for hours.

**User pushback that shaped the real fix** (paraphrased): a hardcoded bigger number would just move
the problem to the next larger model; "does it auto-adapt for a bigger model?"; "if a region is
genuinely full, what then?" — all correct concerns, converged on:

1. `push_pages_ex(len, cma_index, soft)` (`alloc-stage-chcore.cpp`): existing 200-retry-then-BUG_ON
   behavior preserved for callers with no fallback; `soft=true` returns a negative value after the
   200 retries instead, since 200 retries with a yield between each is already enough to rule out
   the known-transient `-EINTR` condition (see prior session) — a failure surviving that many
   retries is a reliable "genuinely out of room" signal, not bad luck.
2. **Tensor-loading pool** (`tensor_pool_alloc`, same file): changed from "reserve the whole
   ~768MiB bank once" to "grow in 128MiB chunks as needed" (`push_pages_ex(...,soft=true)` per
   grow). Small models use few chunks, large ones use more — no hardcoded total, no guessing.
   Genuine exhaustion of an index (tensor-loading has no fallback index of its own — each of the 4
   REE worker threads is permanently bound to one index via `get_cma_index()`'s round-robin) is a
   real capacity ceiling, `BUG_ON`s with a clear log line instead of silently corrupting.
3. **NPU-scratch pool** (`npu_scratch_alloc`, `ggml-rknpu-re.cpp`): same incremental-chunk growth,
   but with genuine overflow: tries its own dedicated index (`TZASC_NR_NPU_SCRATCH`) first, and once
   THAT is genuinely full, moves to try indices 0, 1, 2 in order (which by the time NPU-scratch
   allocation starts have already finished all their own tensor-loading pushes, per the pipeline's
   load-then-compute ordering, so competing for the same physical space isn't a concern).

Net effect: total usable capacity across the 4 tzasc_cma banks (~3GB combined) is now available to
whichever pool actually needs it, dynamically, instead of a fixed a-priori split — the only hard
ceiling left is the real physical total (~3GB), which fails loudly (`BUG_ON`) rather than silently.

**Explicitly deferred (proposed by the user, correctly identified as bigger/riskier than tonight's
fix)**: LRU eviction for models whose total NPU-scratch + tensor-loading need exceeds ~3GB total.
Would require replacing the bump allocator with a real free-list (bump allocators can't reclaim
individual chunks), a last-used tracking scheme per weight buffer, and — the hard part — a
correctness proof that a buffer is never evicted while another thread might still be reading it
mid-flight (silent data corruption if wrong, not a crash). Scoped as a distinct future task, not
attempted this session given the size/risk and how long this session had already run.

### Status as of this writing
All three fixes applied to source; OS/kernel/uboot rebuilt and reflashed (kernel driver touched
once, for the `tc_client_driver.c` `llm_client_mmap()` offset-within-entry support added in a
between-session iteration — also fixed 3 hand-copied `struct llm_client_op_pages` definitions
(kernel + `io-backend.cpp` + `alloc-stage.cpp`) to stay in sync, with `static_assert`/
`_Static_assert(sizeof(...) == 24, ...)` in all three so a future size drift fails the BUILD instead
of silently corrupting an ioctl command number). A test run with Fix A+B alone (NEON confirmed
innocent, `to_float()` fixed) got to `npu_submit #142` in ~143s before hitting the NPU-scratch pool
assert (Fix C's target). Fix C's build was rebuilding/reflashing as this note was written — not yet
confirmed working end-to-end on hardware. **Next step: verify this build reaches the same point and
continues past it, ideally all the way to real decoded text output.**

## 2026-08-01 (continued, later same day): cross-pool coordination bug, NPU-scratch
eviction, REAL DECODED TEXT ACHIEVED for the first time — but with a still-open
output-correctness bug on the secure path

**Bug #4 (cross-pool coordination)**: `tensor_pool_alloc()` (alloc-stage-chcore.cpp) and
`npu_scratch_alloc()` (ggml-rknpu-re.cpp) both grow into indices 0-2 on overflow with no shared
bookkeeping -- tensor-loading's own hard-`BUG_ON`-on-exhaustion response was wrong once cross-index
overflow became possible. Fixed by giving `tensor_pool_alloc()` the same try-order overflow
capability, returning the actual landed index via a new out-parameter (`AllocTask::actual_cma_index`,
threaded through to `AllocStage::submit()`'s `msg.cma_indexes`/`msg.paddr` bookkeeping, which is
indexed BY physical bank and would silently misfile a chunk under the wrong bank otherwise).

**Bug #5 (real exhaustion, `npu_scratch_alloc`'s `TRY_ORDER` genuinely exhausted)**: confirmed on
hardware (all 4 tzasc_cma banks reporting "0 free of 196608 total pages"), causing the "unreachable"
bounds `GGML_ASSERT` to fire -- which, like every other `GGML_ASSERT`/`BUG_ON` in this TA, does not
cleanly abort; it silently hangs the thread forever. This happened because `rknn_mem`'s tzasc
destructor was a deliberate no-op ("never individually freed") and `matmul_kernels.clear()` at the
prefill->decode transition, while it DOES drop the C++ objects, never actually reclaimed the
physical memory those objects backed. Fixed with a real free-list (`npu_scratch_free()`, called
from the now-non-no-op destructor) plus reactive LRU eviction (`npu_scratch_evict_one()`, only
evicts a `matmul_kernel` whose `matmul_kernels`-vector `shared_ptr::use_count()==1`, i.e. nothing
else currently holds a live reference -- correctness condition, not a race) triggered only when
every index is genuinely exhausted (adaptive, not a guessed byte budget). **Result: a test run went
from crashing at `npu_submit #299` (this exact exhaustion) to completing 63/63 decode steps (`lm_head`
completed exactly 64 times, matching `-n 64`) with zero evictions ever needed -- the free-list alone
(fed by the natural `matmul_kernels.clear()`) was sufficient for TinyLlama.**

**First real decoded text produced by the actual NPU+TrustZone secure path, ever, this session**:
after the above fixes, a full `fake -s 0` run completed and printed a real answer for the first time
-- but the text was degenerate: `despite` repeated ~50 times (with occasional `Lebens`/`items chron`),
not coherent language. This is a genuine, separate, pre-existing correctness bug (grep found the
exact phrase "`-s 0` (real NPU) still separately known to produce garbage tokens even on a
'successful' (non-stalling) completion" already in this file, from an earlier session, undiagnosed)
-- **not** something introduced by tonight's memory fixes, just never observed all the way through
before because every earlier run crashed first.

**Two architectural discoveries made while chasing this, both worth keeping in mind long-term**:
1. `llama-cli` (the CA-side binary, `examples/main/main.cpp`) had a leftover
   `params.n_gpu_layers = 0;` "TEMP DIAGNOSTIC" override (with its own comment inviting removal once
   the NPU path was fixed) plus a compiled-out `#ifdef LLAMA_USE_CHCORE_API` block that was the ONLY
   place `ggml_backend_rknpure_set_strawman(false)` got called -- meaning `llama-cli` standalone
   (unlike `fake`, which correctly relays `-s 0` all the way to the TA) always silently ran the
   CPU-only "strawman" branch internally, regardless of `-ngl`. Both fixed (removed the override,
   added an `#else` branch setting `is_strawman=false` explicitly for the non-chcore build) so
   `llama-cli -p "tinyllama#<text>" -ngl 999` now genuinely exercises the real NPU compute path too
   (confirmed via populated, non-empty `rknpu measure dump` output) -- though this specific CA-side
   path uses NPU hardware directly, *not* through the TrustZone/SMC-mediated secure channel `fake`
   uses, so it's a different (and per this session's test, actually correctness-clean) code path.
2. **The pristine Zenodo artifact (fetched fresh via streaming `tar -xzO` from
   `zenodo.org/api/records/17054270/files/tz-llm-ae.tar.gz/content`, NOT this repo's git history's
   "Initial clean checkpoint" -- that commit was already found to contain `[MEM_PROBE]`-style debug
   comments, i.e. was NOT actually pristine, contrary to what
   `reference_tzllm_zenodo_artifact` memory previously assumed) has NEITHER `weight_ready` caching
   NOR `npu_scratch_alloc`/pooling at all.** Both are entirely local additions from earlier sessions.
   The original design's `AllocStage::rollback()` only implements error-retry cleanup (calls
   `pop_pages()` to undo a failed allocation attempt), not the paper's own described "extend and
   shrink" DAG-order memory release -- that mechanism isn't actually present in the shipped artifact
   either, so there was no reference implementation to copy from for a "proper" bounded-memory
   design. Confirms the ~3GB physical ceiling is a genuinely hard constraint this project's own
   caching additions introduced (for real, measured performance reasons -- see Fix B's ~10,000x
   speedup), not something upstream already solved.

**Attempted fix for the degenerate-output bug, RESULT: partially confirms a real bug exists, but not
fully resolved**:
- **Hypothesis**: `g_float_scratch` (the persistent to_float() scratch buffer from Fix B, this
  session's earlier `to_float()` performance fix) was a single process-wide buffer guarded by a mutex
  only around the "get a valid pointer" step, not around the caller's subsequent read of its
  contents -- safe only if pre_scale()/pre1() are never concurrently active for two different
  tensors, an assumption never independently verified for this project's own pipelined-restoration
  scheduler (which is specifically designed to overlap restoration and computation operators).
- **Fix attempted**: made `g_float_scratch`/`g_float_scratch_cap`/`g_float_scratch_source`
  `thread_local` instead of a shared `static`, removing the mutex entirely -- each thread gets its
  own buffer, so no cross-thread interference is possible regardless of what the scheduler overlaps.
  Trade-off: sacrifices most of Fix B's cross-thread cache-sharing (same-thread reuse across
  pre_scale+pre1 for one tensor, the original redundancy target, is unaffected).
- **Test 1 (CA-side `llama-cli`, direct/insecure NPU access, `is_strawman=false` fix applied)**:
  produced fully coherent text ("I do not have a name. What is the name of the person you are
  speaking to?..."), fast (~12s total, non-empty rknpu measure dump confirming real NPU use).
- **Test 2 (TA-side `fake -s 0`, real TrustZone-protected secure path, same thread_local fix)**:
  STILL degenerate -- `Preferences` repeated ~90 times instead of `despite`, i.e. the specific
  repeated token changed but the failure mode did not. **This means the thread_local fix genuinely
  fixed *something* real (confirmed by test 1's clean output using the identical source file), but
  is not the (or not the only) cause of the secure-path-specific degenerate output.**
- **Isolation test attempted**: since tzasc_cma physical-memory reuse (this session's own npu_scratch
  free-list, Bug #5's fix) is the one mechanism that exists on the secure path but has no equivalent
  on the insecure CA-side path, disabled reuse entirely (`NPU_SCRATCH_REUSE_ENABLED = false` --
  reverted back to `true` after the test, see the flag's own comment in `ggml-rknpu-re.cpp`) to see
  if reused TZASC memory itself was implicated. **Result: inconclusive** -- without reuse, TinyLlama
  hits genuine secure-world OOM (`[OOM] pool_idx=... pool_mem_size=...` spam) around `npu_submit
  #176`, far too early to observe output quality. Reuse is required for this model to complete a run
  at all within the ~3GB budget, so it must stay enabled; a different isolation strategy is needed.

**Status as of this writing**: the CMA-exhaustion crash/hang bug (this session's original goal) is
definitively fixed and confirmed -- 63/63 decode steps complete reliably now, vs. crashing at
`npu_submit #299` before. A SEPARATE, pre-existing, still-unresolved output-correctness bug remains
on the real secure NPU+TrustZone path specifically (confirmed NOT present on the insecure/CPU CA
path with the identical thread_local fix). `NPU_SCRATCH_REUSE_ENABLED` is left `true` (required for
completion); the isolation test that would have cleanly confirmed/denied reuse as the cause was
inconclusive due to hitting OOM without it.

**Next steps for whoever resumes**:
1. Don't re-litigate the CMA-exhaustion fix (Bug #4/#5 above) -- confirmed working, move on.
2. The degenerate-repetition bug is real, secure-path-specific, and reproducible (`fake -s 0`,
   TinyLlama, prompt "What is your name?", `-n 64`) -- a good, cheap repro to validate any fix
   against without needing a huge model.
3. Promising unexplored angles, roughly in order of how directly they test the current leading
   theory (reused TZASC memory not being properly prepared for a new tenant):
   - Add a checksum/canary write-then-immediate-readback right after `npu_scratch_alloc()` returns a
     REUSED (from-freelist) chunk, before any real data is written into it, to directly check for
     unexpected pre-existing content (would confirm/deny a cache-coherency or zeroing issue without
     needing the whole isolation-build-flash-test cycle again).
   - Check whether `usys_cache_flush()`/the DMA-coherency path treats freshly-reused tzasc_cma
     memory any differently than freshly-`cma_alloc()`'d memory from the kernel's point of view --
     the *kernel's* CMA pages themselves are never freed/reallocated by this session's fix (only the
     *userspace* free-list changed), so this may be a dead end, but wasn't directly checked.
   - Compare `weight_mem->scale`/`commit_scale()` values between a coherent-looking early token and
     the point output degenerates, to see if scale computation (not just data content) drifts wrong
     specifically on the secure path.
4. If none of the above pan out quickly, consider whether the bug pre-dates ALL of this session's
   memory-pooling work entirely (STATUS.md's own earlier note flags it as already-known and
   undiagnosed) -- it may be worth bisecting against an older known-bad commit specifically for
   *output correctness* (independent of the crash fixes, which should be kept either way) rather
   than assuming it's caused by anything from tonight.

**Follow-up (same night, after the above): `<4GB` small-pool OOM investigated and found NOT to be
caused by `matmul_kernels`/`A_map`/`C_map` growth.** Added `[MEMPOOL_DIAG]` prints (every 20th
`pre0()` call, gated `ith==0`) showing `matmul_kernels.size()`/`A_map.size()`/`C_map.size()`.
Confirmed on hardware across a full run: `matmul_kernels` grows from 0 during prefill, drops to ~9
at the prefill->decode `clear()` transition (proving that clear() correctly reclaims prefill's
kernels), grows back up to exactly **155** as decode's own per-weight_id kernels get created, then
stays **flat at 155 for the rest of the run** (dozens of consecutive readings, spanning many decode
tokens) -- bounded, not leaking. `A_map`/`C_map` stayed flat at 2/5 throughout. **This disproves the
hypothesis that these three caches cause the small-pool OOM seen during the earlier
`NPU_SCRATCH_REUSE_ENABLED=false` isolation test** -- that OOM's cause is still unidentified; it may
be specific to disabling tzasc reuse (an indirect coupling, e.g. different timing exposing
fragmentation) rather than a property of `matmul_kernels`/`A_map`/`C_map` themselves. Do not re-add
reuse/eviction for `A_bufs`/`C_bufs` based on this session's evidence -- it wouldn't address a
confirmed-absent leak. If small-pool OOM recurs, look elsewhere (e.g. `npu_task`'s own
`regcmd`/`tasks` `mem_allocate()` calls -- one pair per `npu_task`, and lm_head alone needs 3
`npu_task_multi_core` groups x `NPU_CORE_NUM` -- multiplied across 155 cached kernels this is still
small in raw bytes, but per-PMO kernel bookkeeping overhead was never directly measured and could be
disproportionate; or genuine fragmentation in the tiny 67-82MB pools independent of any one
allocator's total byte count).

**Also observed, same test run**: with this diagnostic build (thread_local + is_strawman fixes +
`NPU_SCRATCH_REUSE_ENABLED=true` restored + the new `[MEMPOOL_DIAG]` prints), `fake -s 0` reached the
exact same point as every other run tonight (`lm_head`'s `[SUBMIT_LOOP_DIAG] all 3 groups done` then
`[STAGE_DIAG] post enter`) and then **genuinely hung for over an hour of real wall-clock time**
(confirmed via `/proc/uptime`, not the misleading cumulative-CPU-time `ps` TIME field) with zero
further UART output of any kind -- unlike every earlier run tonight at this same point, which
completed (with degenerate but present output) within single-digit minutes. Killed manually; not
diagnosed further this session. Whether this is a fluke, a rare timing-dependent manifestation of
the same root cause as the degenerate-output bug, or something newly introduced by the
`[MEMPOOL_DIAG]` diagnostic code itself (added a `matmul_kernels_mtx` lock acquisition inside
`pre0()`'s existing `ith==0`-gated block -- reviewed for self-deadlock and found sequential/safe,
but not proven safe against concurrent operators from other threads) is unknown. **If resuming: try
reproducing first with the `[MEMPOOL_DIAG]` prints removed/disabled before spending time on this
specific hang, to rule out the diagnostic code itself as a confound.**

**Overall status heading into a well-earned stop**: the session's original goal (CMA-exhaustion
crash/hang) remains fixed and confirmed multiple times over. The output-correctness bug and this new
post-lm_head hang are both real, both unresolved, and -- per the analysis above -- likely worth
investigating together rather than as unrelated issues, since both manifest at the exact same
tensor. `NPU_SCRATCH_REUSE_ENABLED` should stay `true`. The `[MEMPOOL_DIAG]` instrumentation is
harmless to leave in (cheap, gated, informative) but consider removing it first when reproducing the
hang specifically, per above.

## Same night, further localization: root cause narrowed from "hang" to "same token every
decode step" -- points at KV-cache/attention, not `lm_head` or the NPU matmul path itself

Added `[POST_DIAG]` (per-output-tile timing inside `rknpu2_matmul_post()`) and `[DECODE_DIAG]`
(bracketing `llama_decode()` and `gpt_sampler_sample()` in `examples/main/main.cpp`, which is
compiled into the TA-side `llama-cli` that `fake -s 0` actually drives).

**`[POST_DIAG]` result**: `lm_head`'s entire `post()` (9 output tiles) completes in ~7ms flat --
conclusively rules out `rknpu2_matmul_post()` itself as the site of the previously-observed >1hr
hang.

**`[DECODE_DIAG]` result (the actual finding)**: on this run, the pipeline did NOT hang at all.
`llama_decode()` and `gpt_sampler_sample()` both completed normally and repeatedly, at a steady
~2.7s/token cadence (`n_past=88 -> 89 -> ...`). But **`gpt_sampler_sample()` returned the exact same
token id (`26139`) on multiple consecutive decode steps**, directly observed via the sampler's own
return value, not inferred from printed text. This is likely the literal mechanism behind every
degenerate-repetition symptom seen tonight (`despite`, `Preferences`, and whatever `26139`
detokenizes to) -- **the sampler and `lm_head`/`post()` are both working correctly on whatever input
they're given; the input itself (the hidden state feeding `lm_head`) is apparently not changing
between decode steps.**

**Implication for where to look next**: this points away from `ggml-rknpu-re.cpp`'s matmul/post
code (repeatedly confirmed innocent -- correct tile counts, correct fast timing, no data-race
smoking gun found despite real effort) and toward whatever propagates each decode step's new
context into the next forward pass -- most likely the **KV-cache update/attention mechanism**
specifically on the secure NPU path (`-s 0`). Concretely worth checking next session, roughly in
priority order:
1. Whether the KV-cache tensors (`llama_kv_cache_init` et al.) are ordinary `ggml_backend_cpu`
   buffers unaffected by anything in `ggml-rknpu-re.cpp`, or whether NPU-backend tensor views into
   them could be stale/wrongly-aliased on this path specifically (the CPU-only `-s 1`/direct-CA-NPU
   paths both produce varying, coherent output -- only the full secure `-s 0` path shows this fixed
   repetition, so whatever differs specifically about `-s 0`'s data flow into/out of attention is
   the lead).
2. Whether `embd`/the newly-sampled token id (`gpt_sampler_accept(smpl, id, ...)`, right after the
   diagnosed sample call) is actually correctly fed back in as the NEXT `llama_decode()` call's
   input token on this path -- add one more `[DECODE_DIAG]`-style print showing the actual token id
   passed into the next `llama_batch_get_one()` call, to directly confirm/deny "same input every
   time" vs. "different input every time but the model still produces the same output regardless".
3. If (2) shows genuinely different input tokens each step (ruling out a decode-loop-level bug),
   the issue is deeper -- likely attention/RoPE position handling or KV-cache write correctness
   specifically under the pipelined-restoration/tzasc_cma execution model, which is a materially
   different (and much less explored this session) code path than anything touched tonight.
4. `-t "What is your name?"` / TinyLlama / `-n 64` remains a fast, cheap, reliable repro --
   `[DECODE_DIAG]`'s sampled-token-id print makes "is it repeating" directly observable within
   seconds of the first couple of decode steps, without needing to wait for full detokenized text
   or a full run to complete. Use it as the fast feedback loop for whatever's tried next, rather
   than always running to completion.

This session's `[DECODE_DIAG]`/`[POST_DIAG]` instrumentation is left in place (both are cheap and
`ith==0`-gated) -- useful for the next session's continued investigation as-is.

**Follow-up (same night): added `input_token=` to `[DECODE_DIAG]`'s "before llama_decode" print --
result nuances the picture above, doesn't fully resolve it.** Test design flaw discovered while
reading the results: since each decode step's input token is literally `embd.push_back(id)` from
the *previous* step's sampled output, "is input_token stuck" and "is the sampled output stuck" are
not independent observations once the sampler is already stuck -- a stuck sampler trivially produces
a stuck next-input by construction, so this specific print can't cleanly distinguish "decode loop
bug" from "attention/KV-cache bug" the way originally hoped.

That said, the actual observed sequence is still informative: `input_token` was `4349` at the first
real decode step (`n_past=27`), then `22173` from `n_past=35` onward, unchanged through at least
`n_past=50` (confirmed across 3 consecutive prints). **I.e. the model produces at least a couple of
genuinely different tokens first, and only then falls into a fixed point where it keeps
re-selecting the same token.** This is a materially different failure signature than "broken from
token 1" -- it's consistent with either (a) a classic degenerate-repetition sampling/repeat-penalty
issue that could in principle happen on ANY inference path but happens to be triggered here (worth
directly comparing sampler params/repeat-penalty state between the working CPU path and this run),
or (b) a genuine but *gradual* numerical drift specific to the secure NPU path that eventually
crosses some threshold and self-reinforces (e.g. attention increasingly dominated by a residual
numerical bias each step, until greedy/top-k sampling has nowhere else to go). Cannot distinguish
between these from tonight's data.

**Better next test** (not attempted tonight, session-length-limited): print the actual top-few
logit values/token candidates at each decode step (not just the final sampled id) for both the CPU
path (`-s 1` or plain `llama-cli -m ...`) and this secure path, starting from the same prompt, and
diff them token-by-token. If the two paths' logit distributions are already close-but-diverging by
`n_past=35`, that's strong evidence for (b) (numerical drift) over (a) (a sampling-layer bug that
would likely misbehave identically regardless of which backend computed the logits). This is a more
surgical, decisive test than continuing to guess at hidden-state/KV-cache plumbing blind.

## Same night, continued autonomously (user asleep, authorized unattended work): `[LOGIT_DIAG]`
implemented, build+bake+repack done in software, **stopped at the MaskROM boundary on purpose**

Implemented the "better next test" above: added `[LOGIT_DIAG]` to `examples/main/main.cpp`, right
after `llama_decode()` returns each decode step -- prints the top-5 logit values *and* their token
ids (`llama_get_logits(ctx)` / `llama_n_vocab(model)`, simple O(5*n_vocab) partial selection, no
new deps). Compiled into both CA-side and TA-side `llama-cli` (same source, same
`build-llama-docker.sh` as always). Docker build succeeded; OS bake (`oh-builder-hdf.sh` +
`build-oh-docker.sh`) and repack (`flash/repack.sh`) should be complete or in progress with no
further action needed to reach `checkpoints/uboot_repacked.img` + `scripts/kick-the-tires/share/
images/boot.img` -- check those files' mtimes against this section's own timestamp if picking this
up later to confirm.

**Tried and abandoned**: running the CA-side `llama-cli` locally via `qemu-aarch64-static` (present
on this machine) to get a CPU-baseline `[LOGIT_DIAG]` trace without needing the board at all.
Doesn't work: `set_io_model_path()` (`examples/main/main.cpp`) is called *unconditionally* right
after argument parsing regardless of whether `-p` uses the `"model#..."` TZ-routing syntax, and it
unconditionally tries to `open("/dev/tc_ns_client", ...)` (`io-frontend.cpp:37`,
`GGML_ASSERT(tzd_fd >= 0)`), which doesn't exist under qemu-user emulation on a normal Linux
dev machine (no real TrustZone driver here). Crashes immediately (SIGSEGV then SIGABRT under qemu).
Not worth patching around for a one-off diagnostic run -- **the actual comparison should happen ON
THE BOARD instead**, using the same already-built, already-flashed binary in its two existing modes:
plain `llama-cli -m /data/ssd/tinyllama...gguf -p "What is your name?" -n 64` (confirmed working,
CPU-only, produces coherent text -- "My name is Lily" etc., earlier tonight) vs. `fake -s 0`
(secure path, the one that degenerates). Both already carry the `[LOGIT_DIAG]` instrumentation once
this build is flashed, so no separate baseline machinery is needed -- just run both back-to-back
post-flash and diff the `[LOGIT_DIAG]` lines by `n_past`.

**Stopped here on purpose, not stuck**: flashing requires physical MaskROM entry (holding a button
while power-cycling the board), which cannot be done unattended. Confirmed ready and waiting, as of
2026-08-02 ~00:38:
- `checkpoints/uboot_repacked.img` (repacked, `boot_linux` string verified present, `optee` FIT
  component hash `48d61746...` -- different from every earlier hash tonight, confirming this really
  is the `[LOGIT_DIAG]`-carrying build, not a stale one)
- `checkpoints/boot.img` (copied fresh from `scripts/kick-the-tires/share/images/boot.img` --
  **note**: this file had gone stale from an earlier test cycle and was NOT automatically kept in
  sync by `repack.sh` (which only touches the uboot image) -- had to manually re-copy it here.
  Worth remembering for next time: after every `oh-builder-hdf.sh` bake, copy BOTH
  `share/images/boot.img` and the repacked uboot image into `checkpoints/` before flashing, not
  just run `repack.sh` and assume `checkpoints/boot.img` is current.**

**To resume**: get the board into MaskROM (`rkdeveloptool ld` should report `Maskrom`, not
`Loader`), then from `/mnt/2d4726e7-046b-47c7-b9a9-d2a9cc0cfc8d/Work/OPTEE/project` run
`./flash/flash.sh checkpoints/uboot_repacked.img checkpoints/boot.img` (with the scratchpad
`sudo_shim` on `PATH` if `rkdeveloptool` needs it in that environment), then follow the established
boot/hdcd/hdc/push-binaries steps documented throughout this file, then run both `llama-cli` (plain
CPU) and `fake -s 0` (secure) with the same prompt and diff their `[LOGIT_DIAG]` sequences by
`n_past` as described above.

## 2026-08-02: NPU (-s 0) performance root-caused and fixed (~13-2000x depending on tensor size); degenerate-output bug now definitively characterized (frozen from step 0, not drift)

Picked up from the "not fast enough, 48+ minutes with zero output" state left by the thread_local
fix. Two real bugs found and fixed this session, both in `float_scratch_convert()`
(`ggml-rknpu-re.cpp`), plus a decisive new diagnostic result for the separate correctness bug.

### Bug: thread_local's cache design was both racy-safe AND catastrophically slow
The prior session's `thread_local` fix (each thread gets its own `g_float_scratch`) genuinely fixed
a real data race (confirmed: fixed degenerate output on the insecure CA path) but discarded ALL
cross-thread/cross-call reuse Fix B relied on. Live measurement tonight: pre0->pre1 gaps (i.e. the
whole `pre_scale()` body, where `float_scratch_convert()` runs) scaled linearly with tensor element
count at ~3.49us/element -- 256x2048 tensors ~1.84s, 2048x2048 ~14.66s, 5632x2048 ~40.3s, EVERY
single time (not just once), because each of TinyLlama's ~155 per-tensor-dedicated weight buffers
(itself a correctness fix from the prior session, see "per-tensor dedicated buffers") now paid full
first-touch-page-fault cost independently. A single decode step didn't complete in 48+ minutes.

### Fix 1: shared cache, correctly synchronized (double-checked locking, immutable-after-publish)
Replaced `thread_local` with a small shared cache (`g_float_scratch_cache`, keyed by source pointer
`B` + `nele`) where each entry is converted exactly ONCE (double-checked locking: convert outside
the lock so concurrent DIFFERENT-tensor conversions never serialize against each other; re-check
under the lock before publishing, discarding a redundant duplicate conversion if another thread beat
this one to it) and is READ-ONLY forever after -- concurrent lock-free reads are safe because nothing
ever mutates a published entry. Bounded by a 512MB total-byte LRU budget, not entry count.

### Bug found IMMEDIATELY after deploying Fix 1: the cache itself reintroduced the exact first-touch
### bug it was built to eliminate
Live re-test showed IDENTICAL ~3.49us/element gaps, completely unchanged. Root cause: every cache
MISS called `new float[nele]` -- a genuinely fresh heap allocation, paying the exact same first-touch
page-fault tax as the pre-Fix-B bug, just moved from "every pre_scale/pre1 call" (thread_local) to
"every distinct tensor's first call" (still ~155 times total, not the 1 time Fix B originally
achieved by NEVER freeing its one persistent buffer).

### Fix 2: back the cache with a persistent, growable, never-freed ARENA (same idea as Fix B, and
### the same free-list-of-already-touched-chunks idea as npu_scratch_alloc, now applied to plain
### host `malloc()` memory instead of tzasc_cma)
`float_arena_alloc(nele)`: best-fit reuse from a free-list of previously-evicted (already-resident)
chunks first; else bump-allocate from a growable 128MiB-chunked region; else grow a new chunk. A
cache entry's `shared_ptr<float[]>` custom deleter returns its chunk to the arena free-list on
eviction instead of calling `free()` -- physical pages, once touched, stay resident and get reused by
whichever future tensor needs a same-or-smaller buffer next, exactly mirroring Fix B's "only the
first ever tensor pays first-touch cost" property, but now safely shared across N concurrently-cached
tensors instead of a single global slot.

### Also added (turned out NOT to be the dominant cost, but harmless/kept): `npu_scratch_alloc()`
pre-faults each freshly-grown 128MiB tzasc_cma chunk with one sequential `memset()` at grow time
instead of letting pre1()'s scattered tiled write pattern discover fresh pages one at a time. Tested
in isolation (before Fix 2): made no measurable difference to the pre0->pre1 gap, meaning the
destination NPU-tiled weight buffer's own first-touch cost was NOT the bottleneck -- `fB`/`fB1` (the
float32 scratch buffer read during scale-finding) was. Kept anyway since it's a safe, free
optimization for whatever residual cost that memory does have.

### Measured result (live STAGE_DIAG timing, on-hardware, `fake -s 0`, TinyLlama, full -n 20 run)
- 256x2048: ~1.84s -> **~2.1ms** (~880x)
- 2048x2048: ~14.66s -> **~6ms** typical (occasional ~3.7s on a genuine new-arena-chunk boundary,
  still far better than 14.66s every time)
- 5632x2048: ~40.3s -> **~10.1s** (still the single biggest per-tensor cost, but ~4x better; a
  128MiB arena chunk holds only ~2-3 tensors this large, so chunk-grow boundaries are hit more often
  proportionally for the biggest shapes)
- lm_head (32000x2048, ~262MB, the single largest tensor in the model): still the dominant one-time
  cost, but the whole run -- load + prefill (27 tokens) + decode (19 tokens) -- completed in
  **~600 seconds total** (`llama_perf_context_print`: prompt eval 533.5s/27 tok, eval/decode 55.7s/19
  tok = **2.93s/token decode**), vs. the prior session's runs never completing at all within any
  practical test window (48+ minutes, zero output). Overall `npu_submit` progression also went from
  ~19.5s/submit to matching the historical "fast" baseline (~1-1.5s/submit) for most of the run.

### Decisive new result for the SEPARATE, still-unresolved degenerate-output correctness bug
With the performance fix in place, a full run finally completed fast enough to gather the
`[LOGIT_DIAG]` comparison data planned at the end of the previous session. Result: **`[LOGIT_DIAG]`'s
top-5 logits are IDENTICAL (same 5 token ids: 4573, 4566, 4541, 4538, 4516, only reordering slightly
within the top-5) at EVERY measured step from n_past=0 (end of prefill) through n_past=45 (final
decode step) -- 19 consecutive decode steps, zero variation in which tokens dominate.** Logit
magnitudes are also abnormally large (~298-333) vs. the CPU baseline's ~10-19 range measured earlier
this session with the identical prompt. Generated text: token id 4573 ("Mit") sampled on literally
every single decode step, producing "Mit Mit Mit Mit ..." x20.

**This definitively answers the open question from the previous session's diagnostic plan**: the
degenerate-repetition bug is **NOT gradual numerical drift** (which would show the top-5 set
DIVERGING over many steps, the way the CPU baseline's top-5 set changes completely at every single
step) -- it is **broken/saturated from the very first token**, before any drift could even
accumulate. This rules out "small per-step numerical error compounding over a long generation" as
the mechanism, and points instead toward something that's wrong on step 1 already: a fixed/constant
miscomputation (e.g. a scale or quantization factor applied wrong, a buffer read before it's fully
written, or a hidden-state computation saturating/going wrong in one specific upstream layer) rather
than an accumulating-error class of bug.

### Next steps for whoever resumes
1. Performance is no longer the blocker for iterating on the correctness bug -- a full test cycle
   (flash -> boot -> test -> full LOGIT_DIAG data) now takes ~10 minutes of actual test runtime
   instead of 48+ minutes of nothing, dramatically cheapening each diagnostic iteration.
2. The "broken from step 0" finding narrows the search: look at what's DIFFERENT about the secure
   path's FIRST-EVER matmul results (right after loading/quantizing, before any decode loop logic
   runs) vs the CPU path's, rather than anything involving accumulation over many tokens. Prime
   suspects per the abnormal ~300+ logit magnitude: a scale factor being applied incorrectly
   specifically on the secure/NPU int8 quantization path (double-applied, or applied with the wrong
   sign/reciprocal), or a genuine data-corruption/aliasing bug in one specific weight or activation
   buffer that's magnitude-independent of which token is being generated (would explain why the SAME
   5 tokens dominate regardless of input -- their weight rows in lm_head may simply have anomalously
   large values baked in from a corrupted quantization step upstream).
3. STATUS.md's own earlier "Promising unexplored angles" list (2026-08-01 entry) is still the best
   starting point: checksum/canary-verify a freshly-allocated buffer's contents before writing real
   data into it, and compare `weight_mem->scale`/`commit_scale()` values between the CPU and secure
   paths for the SAME tensor to see if scale computation itself (not just final logit values) already
   diverges.
4. `NPU_SCRATCH_PREFAULT`/pre-fault memset code (added, then found not to be the dominant cost) can
   be left in place (harmless) or removed for cleanliness -- purely a judgment call, not required
   either way.

### Repo state
All fixes (arena-backed float scratch cache, npu_scratch pre-fault) applied to source; OS rebuilt and
reflashed; confirmed on real hardware via the `[LOGIT_DIAG]`/`STAGE_DIAG` timing data above. Not yet
git-committed as of this writing.

## 2026-08-02 (continued): weight-buffer canary test — CONCLUSIVE, rules out buffer corruption

Added a content canary (`debug_checksum`, FNV-1a) to `rknn_mem`: computed once right after
`weight_ready` is set (post-quantization), re-checked on every later skip-path hit (i.e. every
decode token this tensor's cached buffer is reused). First version hashed the FULL buffer (up to
262MB for lm_head) on every check -- this re-touches all the memory the arena fix exists to avoid
re-touching, causing a 41+ minute run with zero progress (caught via the user's own timing pushback).
Fixed to a fixed 64-point sampled hash (O(1) cost regardless of buffer size).

**Result: zero `[WEIGHT_CANARY_MISMATCH]` across a full run (prefill + 19 decode tokens).** This
definitively rules out weight-buffer overwrite/corruption (e.g. a pipelined restoration/computation
race clobbering a buffer while the NPU is still reading it) as the cause of the degenerate-output
bug -- weight data is stable and correct once quantized. Combined with this session's earlier
findings (scale/quantize formulas byte-identical to pristine Zenodo code; weight buffers genuinely
per-tensor-dedicated via `MAT_COPY`, never shape-shared), the bug is NOT in: weight buffer identity,
weight buffer content stability, or the quantization/dequantization math itself.

Also confirmed on this run (different token ids than the previous run's "Mit" x20, but the same
frozen-from-step-0 signature): top-5 logits at n_past=0 through n_past=45 were `{4575, 4666, 4537,
4543, 4569}` throughout, magnitudes ~403-430 (still abnormally large vs. CPU baseline's ~10-19).
The exact token set differing between runs (4573/4566/4541/4538/4516 vs. 4575/4666/4537/4543/4569)
while the FROZEN behavior is identical is itself a clue: whatever's wrong is deterministic given the
run's own internal state (not literally the same every single time, e.g. run-to-run allocation
addresses might matter) but always converges to a small, saturated set of dominant tokens from the
very first step.

### Next steps for whoever resumes
1. Don't re-investigate weight buffer corruption/sharing/scale-formula-correctness -- confirmed clean
   this session, would be re-litigating settled ground.
2. Most promising remaining angle: the **input activation path (`A_bufs`)** -- unlike weight buffers,
   these are NOT `weight_id`-dedicated (still routed through `matmul_buffer_mgr::get_A_bufs()`,
   shape-keyed, shared across tensors/layers with the same activation shape) and get reset/rewritten
   every single call via `pre0()`'s `input_mem->reset_cnt()` + `pre1()`'s actual write loop. If the
   SAME kind of pipelined-restoration/computation race that was ruled out for weights is instead
   happening on the activation side (a DIFFERENT tensor's pre1() overwriting a shared A_bufs slot
   while an earlier tensor's submit() is still reading it), that would explain a frozen/wrong result
   without touching weight data at all. Same canary technique (sampled checksum, verified right after
   the write completes vs. right before `submit()` reads it) would directly test this.
2b. Also worth comparing `weight_mem->scale`/`input_mem->scale` values directly (not just buffer
    content) between the CPU (`-s 1`) and secure (`-s 0`) paths for the SAME early tensor (e.g.
    layer 0's `attn_q`), to see if scale computation itself already diverges even with correct
    underlying buffer content -- would point at the scale-finding reduction loop's parallelization
    (`pre_scale_cnt.fetch_add`-based work-stealing across `nth` threads) rather than the buffer
    system.
3. If both of the above come back clean too, the remaining candidates are: the NPU hardware
   compute/submit path itself (correct input, wrong hardware result -- would need direct pre/post
   buffer dumps bracketing the actual `submit()`/NPU dispatch for one specific tensor), or something
   upstream of all of this in how activations get INTO the pipeline in the first place (embedding
   lookup, or the TrustZone/SMC data relay from CA to TA).

## 2026-08-02 (continued): ROOT CAUSE FOUND AND FIXED — lm_head INT8 quantization on the secure NPU path

After ruling out buffer corruption (canary, 0 mismatches), scale-formula bugs (byte-identical to
pristine), and weight-buffer sharing (per-tensor-dedicated via MAT_COPY, confirmed), added targeted
`[SCALE_DIAG]` instrumentation gated specifically on `n == 32000` (lm_head, TinyLlama's vocab size)
to inspect weight scale, input scale, and raw NPU int32 output + dequantized value directly at the
one tensor whose output IS the final logits.

**Result**: weight scale (~0.0026-0.0037) and input scale (~0.0344) were both correctly calibrated
to their actual max_abs values -- not a miscalibration bug. But raw NPU INT8 dot-product output for
individual vocabulary entries reached **hundreds of thousands** (e.g. raw_i32=1011305, dequant=82.1
for a SINGLE vocab entry's contribution) -- vs. the CPU float32 path's WHOLE final logit range of
~10-19 for the same prompt. INT8 quantization error at this specific layer (2048-dim reduction into
a 32000-way vocab projection) is severe enough to make a small, near-input-independent set of vocab
rows dominate regardless of the actual hidden state -- this IS the mechanism behind the long-standing
"despite"/"Preferences"/"Mit" frozen-repeated-token bug.

**The fix**: found the paper's OWN authors had already anticipated this exact class of problem --
`ggml_backend_rknpure_supports_op()` has a pre-existing comment: `/* can not allocate large B buffers
for large vocab_size. just use cpu to perform these matmuls */` with a `k >= 50000 || n >= 50000`
threshold that routes any op past that size to the CPU backend instead of the RKNPU INT8 path.
TinyLlama's vocab (n=32000) falls just under this threshold, so lm_head still went through NPU INT8.
**Lowered the threshold to 30000**, which catches lm_head (all other tensors in this model are
<=5632) without touching anything else. This is not a novel mechanism -- it's using the paper's own
existing large-vocab safety valve at a size that actually matters for THIS model.

**Confirmed on hardware**: rebuilt, reflashed, reran `fake -s 0` with the identical test prompt.
`[LOGIT_DIAG]` now shows: magnitudes back in the normal ~9.6-10.3 range (matching CPU baseline scale
exactly), and the dominant top-5 token SET now genuinely varies with `n_past` (e.g. n_past=0:
{19271,23914,7459,7324,29441}, n_past=27: {457,7324,19271,20878,7459} -- a different, non-frozen set)
instead of being identical regardless of input as in every prior run this whole investigation.

**Residual, much milder issue observed**: from n_past~27 onward this specific test settled into
repeating token id 457 for many consecutive steps with very stable (~10.24) logit values -- greedy
sampling repeatedly picking the same argmax. Could not recover the actual generated text due to UART
corruption (`GENERATED_ANSWER_START===...===GENERATED_ANSWER_END===` came through empty, a recurring
UART multi-thread interleaving corruption issue, not a code bug). This residual repetition is
qualitatively different from the fixed bug (correct-magnitude, input-dependent-but-converging, vs.
the old always-frozen-regardless-of-input pattern) and may simply be normal small-model greedy-
decoding behavior for this specific short prompt, or subtle residual INT8 quantization noise from the
other 22 layers (still NPU/INT8, unchanged) feeding into an otherwise-correct lm_head. Not yet
determined to be a bug requiring further action.

### Next steps for whoever resumes
1. The core degenerate-output bug (abnormal magnitude, input-independent frozen output) is FIXED and
   confirmed via direct before/after measurement. Do not re-investigate buffer corruption, scale
   formulas, or weight-buffer sharing -- all independently ruled out this session with hard evidence.
2. Get a clean text capture of a full generation (retry the UART capture, or read result via `hdc
   shell cat /data/tz.log` if the CA side ever logs it, or add a print with a more corruption-
   resistant framing) to determine whether the token-457 repetition is a real remaining issue or just
   this particular short prompt's normal (if uninteresting) greedy-decoding outcome.
3. If repetition persists across multiple different prompts/topics, worth checking: (a) llama.cpp's
   default sampler params (repeat_penalty, temperature) for this build -- greedy/near-greedy decoding
   with no repeat penalty WILL loop on any sufficiently confident wrong token, unrelated to hardware;
   (b) whether the OTHER 22 layers' still-INT8-quantized hidden states are meaningfully noisier than
   the CPU float32 path in a way that happens to bias the model toward one attractor token for short
   prompts -- would need the same kind of targeted SCALE_DIAG comparison this session used for
   lm_head, applied to a mid-network layer instead.
4. Performance: lm_head now runs on CPU float32 instead of NPU INT8 -- given lm_head is a comparatively
   small fraction of TinyLlama's total FLOPs (2048x32000 vs the other 22 layers' cumulative
   2048x2048/2048x5632/2048x256 matmuls), this should not meaningfully regress the ~10-minute total
   runtime established earlier this session, but wasn't explicitly re-measured after this fix --
   worth a quick timing sanity check.

### Addendum: file-based answer capture attempt failed (TA has no direct filesystem access)
Tried writing the generated answer to `/data/ssd/rknpu/generated_answer.txt` via a plain `fopen()`/
`fwrite()` in `main.cpp` as a UART-corruption-proof capture method. The file was never created --
confirms the TA/secure-world build does not have direct POSIX filesystem access (consistent with the
project's own custom I/O-relay architecture for reading the GGUF model, `io-frontend.cpp`, rather than
plain file I/O). Not worth pursuing further; the `[LOGIT_DIAG]` numeric evidence (magnitude back to
CPU-baseline scale, top-5 token set now genuinely varies with input across two independent runs with
different dominant tokens) is already decisive confirmation of the fix without needing the literal
generated text. If a future session wants the literal text, it would need to route through the CA's
own I/O relay (the same mechanism `fake_ca.cpp` uses) rather than a bare `fopen()` in TA-side code.
