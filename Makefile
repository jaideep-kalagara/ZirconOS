# --- Toolchain ---
CROSS_PREFIX ?=
CC       := $(CROSS_PREFIX)gcc
LD       := $(CROSS_PREFIX)ld
AR       := $(CROSS_PREFIX)ar
RANLIB   := $(CROSS_PREFIX)ranlib
OBJCOPY  := $(CROSS_PREFIX)objcopy
OBJDUMP  := $(CROSS_PREFIX)objdump
NASM     := nasm

# --- Paths ---
ARCH      := i686
BUILD     := build/$(ARCH)
OBJDIR    := $(BUILD)/obj
LIBDIR    := $(BUILD)/lib
ISOROOT   := $(BUILD)/iso
BOOTDIR   := $(ISOROOT)/boot
GRUBDIR   := $(BOOTDIR)/grub

# --- Includes & Flags ---
INCLUDES  := -Iinclude -Iinclude/arch/$(ARCH) -Iinclude/kernel -Iinclude/libk
CFLAGS    := -m32 -ffreestanding -fno-builtin -fno-stack-protector -O2 -Wall -Wextra -Wno-unused-parameter $(INCLUDES)
ASFLAGS   := --32
LDSCRIPT  := link/$(ARCH).ld
LDFLAGS   := -T $(LDSCRIPT) -nostdlib
NASMFLAGS := -f elf32

# libgcc from the *current* compiler (cross)
LIBGCC := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# --- libk detection (supports libk/ OR src/libk/) ---
LIBK_DIR := $(firstword $(wildcard libk) $(wildcard src/libk))
ifeq ($(LIBK_DIR),)
  $(error Could not find libk sources (expected libk/ or src/libk/))
endif

# libk sources -> libk.a
SRC_LIBK := $(shell find $(LIBK_DIR) -maxdepth 1 -name '*.c' 2>/dev/null)
OBJ_LIBK := $(patsubst %.c,$(OBJDIR)/%.o,$(SRC_LIBK))
LIBK_A   := $(LIBDIR)/libk.a

# kernel/arch sources (exclude libk sources so we don't link them twice)
SRC_ALL_C := $(shell find src -name '*.c' 2>/dev/null)
SRC_C := $(filter-out $(SRC_LIBK),$(SRC_ALL_C))

SRC_S   := $(shell find src -name '*.S' -o -name '*.s' 2>/dev/null)
SRC_ASM := $(shell find src -name '*.asm' 2>/dev/null)

OBJ_C   := $(patsubst %.c,$(OBJDIR)/%.o,$(SRC_C))
OBJ_S   := $(patsubst %.S,$(OBJDIR)/%.o,$(SRC_S))
OBJ_s   := $(patsubst %.s,$(OBJDIR)/%.o,$(SRC_S))
OBJ_ASM := $(patsubst %.asm,$(OBJDIR)/%.o,$(SRC_ASM))

OBJ := $(OBJ_C) $(OBJ_S) $(OBJ_s) $(OBJ_ASM)

# --- Targets ---
.PHONY: all clean iso run disasm tree

all: $(BUILD)/kernel.elf iso

# Link the kernel against libk.a
$(BUILD)/kernel.elf: $(OBJ) $(LIBK_A) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBK_A) $(LDFLAGS) $(LIBGCC)
	@echo "----> $@ done"

# ----- Build libk.a -----
$(LIBK_A): $(OBJ_LIBK)
	@mkdir -p $(LIBDIR)
	@$(AR) rcs $@ $(OBJ_LIBK) $(LIBGCC)
	@$(RANLIB) $@
	@echo "----> $@ done"
	@members="$$($(AR) t $@)"; \
	if [ -z "$$members" ]; then \
	  echo "WARNING: $(LIBK_A) is empty (no members)"; \
	fi

# ----- Object build rules -----

# C -> o (both libk and src share this rule)
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "----> $@ done"

# GAS (.S/.s) -> o
$(OBJDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "----> $@ done"

$(OBJDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "----> $@ done"

# NASM (.asm) -> o
$(OBJDIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	@$(NASM) $(NASMFLAGS) $< -o $@
	@echo "----> $@ done"

# ----- ISO -----
iso: $(BUILD)/kernel.elf boot/grub/grub.cfg
	@mkdir -p $(GRUBDIR)
	@cp -v boot/grub/grub.cfg $(GRUBDIR)/
	@cp -v $(BUILD)/kernel.elf $(BOOTDIR)/
	@grub-mkrescue -o $(BUILD)/os.iso $(ISOROOT) > /dev/null 2>&1
	@echo "----> $(BUILD)/os.iso done"

# ----- Utilities -----
run: all
	@qemu-system-i386 -cdrom $(BUILD)/os.iso
	@echo "----> running $(BUILD)/os.iso done"

disasm: $(BUILD)/kernel.elf
	$(OBJDUMP) -D -Mintel -S $(BUILD)/kernel.elf | less

tree:
	@command -v tree >/dev/null && tree -a -I '.git|build' . || true

clean:
	@rm -rf build
	@echo "----> clean done"
