# --- Compiler & Linker ---
CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld
GRUB_MKRESCUE = x86_64-elf-grub-mkrescue

# --- Flags ---
CFLAGS = -Wall -Wextra -ffreestanding -O2 -w -mno-red-zone -m64 -fno-pic -mcmodel=large -Isrc/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld

# --- Paths ---
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
SRC_DIR = src
OBJ = $(BUILD_DIR)/boot.o $(BUILD_DIR)/gdt_asm.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/kstart.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/idt_asm.o $(BUILD_DIR)/fb.o $(BUILD_DIR)/keyboard.o

# --- Targets ---
all: $(BUILD_DIR)/boot.iso

# Link the Kernel
$(BUILD_DIR)/kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Build the ISO
$(BUILD_DIR)/boot.iso: $(BUILD_DIR)/kernel.bin
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD_DIR)/kernel.bin $(ISO_DIR)/boot/kernel.bin
	@# Generate grub.cfg to fix the "no suitable video mode" error
	@echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'insmod all_video' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'if loadfont unicode; then' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '  set gfxmode=auto' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '  terminal_output gfxterm' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'fi' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set gfxpayload=keep' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "BorsladeOS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '  multiboot2 /boot/kernel.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '  boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	unset TMPDIR; $(GRUB_MKRESCUE) -o $(BUILD_DIR)/boot.iso $(ISO_DIR)

# --- Compilation Rules ---
$(BUILD_DIR)/%.o: src/boot/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/gdt_asm.o: src/include/arch/x86_64/gdt_asm.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/idt_asm.o: src/include/arch/x86_64/idt_asm.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/gdt.o: src/include/arch/x86_64/gdt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: src/include/arch/x86_64/idt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kstart.o: src/kernel/kstart.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot.o: src/boot/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/fb.o: src/include/drivers/screen/fb.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: src/include/drivers/keyboard/keyboard.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
# Launch QEMU (The macOS-safe version)
run: $(BUILD_DIR)/boot.iso
	@# Force clear the environment for this command to bypass the crash
	unset TMPDIR; qemu-system-x86_64 \
		-machine q35,accel=hvf \
		-cpu host \
		-m 1G \
		-drive if=pflash,format=raw,unit=0,file=/usr/local/share/qemu/edk2-x86_64-code.fd,readonly=on \
		-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
		-cdrom $(BUILD_DIR)/boot.iso \
		-vga std \
		-display cocoa,show-cursor=on \
		-monitor none

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run