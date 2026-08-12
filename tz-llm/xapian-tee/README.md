# Xapian TEE Integration (WIP)

This directory contains scripts and source code for cross-compiling the Xapian search engine and deploying it as a Trusted Application (TA) inside the OP-TEE / ChCore environment on the Rockchip RK3588 (Orange Pi 5 Max).

## Structure

- `build-deps.sh`: Downloads Zlib and Xapian-core source code into `src/`.
- `build-xapian.sh`: Cross-compiles Zlib and Xapian-core for the AArch64 TEE environment and installs them to `install/`.
- `xapian-ta/`: Source code for the Trusted Application. It instantiates an *in-memory* Xapian database to avoid TEE syscall restrictions (like `socketpair` / disk I/O).
- `xapian-ca/`: Source code for the Client Application running on Linux (Normal World) to communicate with the TA.
- `build-ca-ta.sh`: Builds both the TA and the CA.

## Build Process

All builds must be done through the custom Docker container provided in this repository to ensure the correct cross-compilation toolchain and ChCore environment are present.

### 1. Build Xapian Core
```bash
# Run the builder script from the repository root
./scripts/kick-the-tires/xapian-builder.sh bash -x build-deps.sh
./scripts/kick-the-tires/xapian-builder.sh bash -x build-xapian.sh
```

### 2. Build TA & CA
```bash
./scripts/kick-the-tires/xapian-builder.sh bash -x build-ca-ta.sh
```
This generates `xapian-ca/xapian-ca` and the `xapian-ta` binary in `xapian-ta/build/xapian-ta`.

### 3. Pack the TA into TEE OS
To embed the TA into the firmware, copy it to the `images` directory and rebuild the OS:
```bash
cp tz-llm/xapian-tee/xapian-ta/build/xapian-ta scripts/kick-the-tires/xapian-ta
cd scripts/kick-the-tires
./oh-builder.sh . bash -x /home/vectorxj/share/build-oh-docker.sh
```

### 4. Repack U-Boot
Because the stock OpenHarmony U-boot looks for a `boot` partition but the SD card layout uses `boot_linux`, you must repack the newly built TEE core into a patched U-Boot:
```bash
cd ../../  # Back to repository root
./flash/repack.sh scripts/kick-the-tires/images/uboot.img
```
This outputs `checkpoints/uboot_repacked.img`.

### 5. Flash to Device
```bash
sudo ./flash/flash.sh checkpoints/uboot_repacked.img scripts/kick-the-tires/images/boot.img
```

## Known Issues & Notes

- **Syscall 199 (socketpair)**: Xapian's on-disk database locking mechanism requires unsupported syscalls in the TEE. The TA currently uses `Xapian::InMemory::open()` to bypass disk I/O entirely.
- **TA Handshake**: The TA must write the string `"msg from tee\n"` to the shared memory buffer immediately after mapping it, and yield back to Linux. If this handshake fails, the Linux kernel module (`tzdriver`) will loop for 15 seconds (`attempt 86 got no TA marker`) and fail to initialize.
