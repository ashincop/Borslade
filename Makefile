# --- Compiler & Linker ---
CC = x86_64-elf-gcc
AS = nasm
LD = x86_64-elf-ld

# --- Flags ---
# -ffreestanding: No standard library
# -mcmodel=kernel: Optimized for high-half addresses (optional but good)
# -mno-red-zone: Required for x86_64 kernels to prevent stack corruption
CFLAGS = -Wall -Wextra -ffreestanding -O2 -mno-red-zone -m64 -Isrc/include
ASFLAGS = -f elf64
LDFLAGS = -n -T linker.ld

# --- Paths ---
BUILD_DIR = build
SRC_DIR = src
OBJ = $(BUILD_DIR)/boot.o $(BUILD_DIR)/gdt_asm.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/kstart.o

# --- Targets ---
all: $(BUILD_DIR)/kernel.bin

# Compile Boot Assembly
$(BUILD_DIR)/boot.o: src/boot/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile GDT Assembly
$(BUILD_DIR)/gdt_asm.o: src/include/arch/x86_64/gdt_asm.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Compile GDT C (Assuming your C code is here)
$(BUILD_DIR)/gdt.o: src/include/arch/x86_64/gdt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile start kernel
$(BUILD_DIR)/kstart.o: src/kernel/kstart.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the Kernel
$(BUILD_DIR)/kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean