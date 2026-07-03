#!/bin/bash

PROJECT_SOURCE_FILE=BOOTX64
BOOTLOADER_EFI_FILE=BOOTX64.EFI

EFIINC=/usr/include/efi
EFIINCS="-I$EFIINC -I$EFIINC/x86_64 -I$EFIINC/protocol"
LIB=/usr/lib64
EFILIB=/usr/lib

gcc -fno-stack-protector -fpic -fshort-wchar -mno-red-zone \
  -maccumulate-outgoing-args -DEFI_FUNCTION_WRAPPER \
  $EFIINCS -c "${PROJECT_SOURCE_FILE}.c" -o "${PROJECT_SOURCE_FILE}.o"

ld -nostdlib -znocombreloc -T $EFILIB/elf_x86_64_efi.lds -shared \
  -Bsymbolic -L $EFILIB -L $LIB \
  $EFILIB/crt0-efi-x86_64.o \
  "${PROJECT_SOURCE_FILE}.o" \
  -o "${PROJECT_SOURCE_FILE}.so" \
  -lefi -lgnuefi

objcopy -j .text -j .sdata -j .data -j .dynamic \
  -j .dynsym -j .rel -j .rela -j .reloc -j .rodata \
  --target=efi-app-x86_64 "${PROJECT_SOURCE_FILE}.so" "${BOOTLOADER_EFI_FILE}"
