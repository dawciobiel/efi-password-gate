PROJECT_SOURCE_FILE = uefi-bootloader-password

ARCH      = x86_64
EFIINC    = /usr/include/efi
EFIINCS   = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIB       = /usr/lib64
EFILIB    = /usr/lib

CFLAGS    = $(EFIINCS) -fno-stack-protector -fpic -fshort-wchar \
            -mno-red-zone -maccumulate-outgoing-args -DEFI_FUNCTION_WRAPPER
LDFLAGS   = -nostdlib -znocombreloc -shared -Bsymbolic \
            -L $(EFILIB) -L $(LIB) \
            -T $(EFILIB)/elf_$(ARCH)_efi.lds

all: $(PROJECT_SOURCE_FILE).efi

$(PROJECT_SOURCE_FILE).o: $(PROJECT_SOURCE_FILE).c
	gcc $(CFLAGS) -c $(PROJECT_SOURCE_FILE).c -o $(PROJECT_SOURCE_FILE).o

$(PROJECT_SOURCE_FILE).so: $(PROJECT_SOURCE_FILE).o
	ld $(LDFLAGS) $(EFILIB)/crt0-efi-$(ARCH).o $(PROJECT_SOURCE_FILE).o -o $(PROJECT_SOURCE_FILE).so -lefi -lgnuefi

$(PROJECT_SOURCE_FILE).efi: $(PROJECT_SOURCE_FILE).so
	objcopy -j .text -j .sdata -j .data -j .dynamic \
	    -j .dynsym -j .rel -j .rela -j .reloc \
	    --target=efi-app-$(ARCH) $(PROJECT_SOURCE_FILE).so $(PROJECT_SOURCE_FILE).efi

clean:
	rm -f *.o *.so *.efi
