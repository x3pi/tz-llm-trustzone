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
