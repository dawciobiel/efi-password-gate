#!/usr/bin/env python3
"""
Generates the EXPECTED_HASH C array for uefi-bootloader-password.c

Usage:
    python3 generate_hash.py "your-password-here"
"""
import sys
import hashlib

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <password>")
        sys.exit(1)

    password = sys.argv[1]
    # Match CHAR16 in-memory layout: UTF-16LE, no BOM, no null terminator
    encoded = password.encode("utf-16-le")
    digest = hashlib.sha256(encoded).digest()

    print("static const UINT8 EXPECTED_HASH[32] = {")
    for i in range(0, 32, 8):
        chunk = digest[i:i+8]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        print(f"    {line},")
    print("};")

if __name__ == "__main__":
    main()
