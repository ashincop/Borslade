CC := x86_64-elf-gcc
AR := x86_64-elf-ar

CFLAGS := -ffreestanding -I./include -mno-red-zone -fno-stack-protector -pie -fPIC -c

SRC_DIR := source
OBJ_DIR := build

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

LIB := libc.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(LIB)

.PHONY: all clean
