DEVKITPPC ?= /opt/devkitpro/devkitPPC
CROSS_COMPILE ?= $(DEVKITPPC)/bin/powerpc-eabi-

CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
PYTHON  := python3

CFLAGS  := -g -Wall -msdata=none -G 0 -ffunction-sections -fdata-sections
LDFLAGS := -T script.ld

SRC     := ppc.c   gba_emulation.c screen_mod.c  entrypoint.c
OBJ     := $(SRC:.c=.o)
ELF     := gba_thread.elf
PATCH   := fsa2.S
MAIN_DOL := main.dol
ORIG_DOL := orig.dol
ENTRYPOINT := 0x801347a4

.PHONY: all clean

all: $(OBJ) $(ELF) $(PATCH)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ) script.ld
	$(CC) $(CFLAGS) -nostartfiles -Wl,-T,script.ld $(OBJ) -o $@

$(PATCH): $(ELF) generate_patch.py
	$(PYTHON) generate_patch.py $(ELF) $(PATCH)

$(MAIN_DOL): $(ELF) $(ORIG_DOL) patch_dol.py
	$(PYTHON) patch_dol.py $(ELF) $(ENTRYPOINT) $(ORIG_DOL) $@

clean:
	rm -f $(OBJ) $(ELF) $(PATCH) $(MAIN_DOL)

