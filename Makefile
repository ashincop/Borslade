# --- Compiler & Linker ---
CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld
GRUB_MKRESCUE = grub-mkrescue

# --- Flags ---
CFLAGS = -Wall -Wextra -ffreestanding -O2 -w -mno-red-zone -m64 -fno-pic -mcmodel=large -Isrc/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld

# --- Paths ---
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
SRC_DIR = src

# --- Find all sources ---
C_SRCS = $(shell find $(SRC_DIR) -name '*.c')
ASM_SRCS = $(shell find $(SRC_DIR) -name '*.asm')

# --- Convert sources to object files ---
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS)) \
      $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

# --- Targets ---
all: $(BUILD_DIR)/boot.iso
# 1. Check the Windows-native variable (Works in CMD/PowerShell/MSYS)
ifeq ($(OS),Windows_NT)
    $(error CRASH: Strawberry Kernel cannot be built on Windows!)
endif

# 2. Fallback check for environments that might hide $(OS) but have uname
UNAME_S := $(shell uname -s 2>/dev/null || echo "Unknown")
ifneq (,$(filter %MINGW% %MSYS% %CYGWIN% %Windows%,$(UNAME_S)))
    $(error CRASH: Detected Windows-based shell ($(UNAME_S)). Build aborted.)
endif

# Detect the Operating System
OS := $(shell uname -s)

# Default value
ACC = kvm

# Platform-specific overrides
ifeq ($(OS), Linux)
    ACC = kvm
endif

ifeq ($(OS), Darwin)
    ACC = hvf
endif
# Link the Kernel
$(BUILD_DIR)/kernel.bin: $(OBJ) genconfig
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Build the ISO
$(BUILD_DIR)/boot.iso: $(BUILD_DIR)/kernel.bin
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(ISO_DIR)/boot/kernel.bin
	cp ./grub.cfg $(ISO_DIR)/boot/grub
	find initrd | cpio -o -H newc > $(ISO_DIR)/boot/initrd.img
	@$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)

# --- Compilation Rules ---
# Compile C
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "AS      $<"
	@$(AS) $(ASFLAGS) $< -o $@
menuconfig:
	kconfig-mconf Kconfig
genconfig:
	mkdir -p include/generated include/config
	KCONFIG_AUTOHEADER=src/include/config.h kconfig-conf --silentoldconfig Kconfig
alldefconfig:
	mkdir -p include/generated include/config
	kconfig-conf --alldefconfig Kconfig
defconfig:
	mkdir -p include/generated include/config
	KBUILD_DEFCONFIG=arch/x86_64/configs/borslade_defconfig kconfig-conf --defconfig Kconfig
savedefconfig:
	kconfig-conf --savedefconfig=defconfig Kconfig

# Launch QEMU
run: $(BUILD_DIR)/boot.iso
	cp immu/OVMF_VARS.fd ./
	unset TMPDIR; qemu-system-x86_64 \
		-machine q35,accel=$(ACC) \
		-cpu host \
		-m 8G \
		-drive if=pflash,format=raw,unit=0,file=immu/OVMF_CODE.fd,readonly=on \
		-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
		-cdrom $(BUILD_DIR)/boot.iso \
		-vga std \
		-serial stdio

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run
