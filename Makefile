# Uses the local extracted toolchain when present, otherwise system PATH.
LOCAL := $(CURDIR)/.tools/root/usr/bin/arm-none-eabi-
CROSS ?= $(if $(wildcard $(LOCAL)gcc),$(LOCAL),arm-none-eabi-)
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
CFLAGS := -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -std=c99 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Isrc
OBJECTS := build/start.o build/game.o build/main.o
.PHONY: all clean test check
all: dist/ilha_do_abrigo.gba
build dist:
	mkdir -p $@
build/start.o: src/start.s | build
	$(CC) -mcpu=arm7tdmi -c $< -o $@
build/%.o: src/%.c src/game.h | build
	$(CC) $(CFLAGS) -c $< -o $@
build/ilha_do_abrigo.elf: $(OBJECTS) gba.ld
	$(CC) -mcpu=arm7tdmi -mthumb -nostdlib -T gba.ld -Wl,--gc-sections,-Map,build/ilha_do_abrigo.map $(OBJECTS) -lgcc -o $@
dist/ilha_do_abrigo.gba: build/ilha_do_abrigo.elf tools/rom.py | dist
	$(OBJCOPY) -O binary $< $@
	python3 tools/rom.py $@
check: all
	python3 tools/rom.py --check dist/ilha_do_abrigo.gba
test: | build
	cc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -g -Isrc tests/test_game.c src/game.c -o build/test_game
	./build/test_game
clean:
	rm -rf build dist/ilha_do_abrigo.gba
