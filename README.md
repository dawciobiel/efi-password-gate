# EFI Password Gate
 
A minimal UEFI application written in C that prompts for a password
before chainloading into a second-stage UEFI bootloader. Built with
[gnu-efi](https://sourceforge.net/projects/gnu-efi/).
 
> ⚠️ **Disclaimer:** This is an educational / experimental project for
> learning UEFI development. It is **not** a hardened security solution.
> See [Security Notes](#security-notes) below before relying on it for
> anything beyond testing.
 
## Features
 
- Password prompt with masked input (`*`) and backspace support
- Password is **never stored in plaintext** in the compiled binary —
  only a SHA-256 hash of the expected password is embedded
- On success: chainloads a second EFI binary (`bootloader2.efi`)
- On failure: displays an error and waits for a keypress before exiting
## Requirements
 
- Linux (tested on Nobara / Fedora-based distributions)
- `gcc`, `binutils`, `make`
- `gnu-efi` and `gnu-efi-devel` packages
- (Optional, for testing) `qemu` and `edk2-ovmf`
Install dependencies on Fedora/Nobara:
 
```bash
sudo dnf install gnu-efi gnu-efi-devel gcc binutils make
```
 
## Project Structure
 
| File | Description |
|---|---|
| `BOOTX64.c` | Main UEFI application source code |
| `BOOTX64.EFI` | compilated target file |
| `generate_hash.py` | Helper script to generate the SHA-256 password hash |
| `Makefile` | Build automation via `make` |
| `build.sh` | Standalone build script (alternative to `make`) |
| `LICENSE` | MIT License |
 
 
## Setting the Password
 
The password is **not** stored as plaintext — only its SHA-256 hash is
embedded in the source code. To set your own password:
 
1. Run the helper script:
```bash
   python3 generate_hash.py "your-password-here"
```
 
2. Copy the generated `EXPECTED_HASH` array into
   `BOOTX64.c`, replacing the existing placeholder.
3. Rebuild the project (see below).
## Building
 
```bash
make
```
 
This produces `BOOTX64.efi` in the project directory.
A `clean` target is also available:
 
```bash
make clean
```
 
### Manual build (without `make`)
 
A standalone `build.sh` script is also included, performing the same
steps (`gcc` → `ld` → `objcopy`) without relying on `make`.
 
```bash
./build.sh
```
 
> Note: paths to `gnu-efi` library/header files vary between distributions.
> On Nobara/Fedora they are typically located in `/usr/lib` and
> `/usr/include/efi`. Adjust the `EFILIB` / `EFIINC` variables in the
> `Makefile` / `build.sh` if your system differs.
 
## Testing with QEMU
 
It is **strongly recommended** to test this in a virtual machine before
deploying it on real hardware.
 
```bash
sudo dnf install qemu edk2-ovmf
 
mkdir -p test_disk/EFI/BOOT
cp BOOTX64.efi test_disk/EFI/BOOT/BOOTX64.EFI
cp /usr/share/edk2/ovmf/OVMF_VARS.fd test_disk_vars.fd
 
qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=test_disk_vars.fd \
  -drive format=raw,file=fat:rw:test_disk \
  -net none
```
 
Adjust the OVMF firmware paths if they differ on your system
(`rpm -ql edk2-ovmf` will show the actual locations).
 
## Deploying on Real Hardware
 
⚠️ **Back up your existing EFI bootloader first.** Replacing
`BOOTX64.EFI` (or whichever file is referenced by your boot entry) with
a broken or misconfigured binary can leave your system unable to boot.
 
This project does not perform any installation steps automatically —
it only compiles the `.efi` binary. Copying it into `/boot/efi/EFI/BOOT/`
(or wherever your ESP is mounted) and configuring boot entries is a
separate, manual step that you take at your own risk.
 
## Security Notes
 
- The embedded value is a **plain SHA-256 hash**, with **no salt** and
  **no key-derivation function** (e.g. PBKDF2, Argon2). This makes it
  vulnerable to offline brute-force/dictionary attacks if an attacker
  extracts the binary.
- There is no protection against an attacker booting an alternative
  medium and reading/modifying the ESP directly, unless combined with
  full disk encryption and/or Secure Boot.
- This project is intended as a **learning exercise** in UEFI
  application development, not as a production-grade access control
  mechanism.
## License
 
This project is licensed under the [MIT License](LICENSE).
 
