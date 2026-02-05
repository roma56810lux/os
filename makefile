CC = gcc
ASM = nasm
LD = ld
CFLAGS = -m32 -nostdlib -ffreestanding -O2
ASFLAGS = -f elf32

all: os_image.bin

os_image.bin: boot.bin kernel.bin
    cat $^ > $@

boot.bin: boot.asm
    $(ASM) -f bin $< -o $@

kernel.bin: kernel.o krnl_asm.o sys.o sys_asm.o calc.o calc_asm.o paint.o paint_asm.o
    $(LD) -m elf_i386 -Ttext 0x7E00 -o $@ $^

%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
    $(ASM) $(ASFLAGS) $< -o $@

clean:
    rm -f *.o *.bin

run: os_image.bin
    qemu-system-i386 -drive format=raw,file=os_image.bin