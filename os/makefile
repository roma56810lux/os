# Makefile для учебной графической ОС

# Настройки компилятора
CC = i686-elf-gcc
ASM = nasm
LD = i686-elf-ld
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I./include

# Исходные файлы
KERNEL_SOURCES = kernel/kernel.c \
                 kernel/gdt.c \
                 kernel/idt.c \
                 kernel/isr.c \
                 kernel/irq.c \
                 kernel/timer.c \
                 kernel/keyboard.c \
                 kernel/mouse.c \
                 kernel/vbe.c \
                 kernel/gui.c \
                 kernel/terminal.c \
                 kernel/commands.c \
                 kernel/framebuffer.c \
                 kernel/memory.c

ASM_SOURCES = boot/boot.asm \
              kernel/gdt_asm.asm \
              kernel/idt_asm.asm

# Объектные файлы
OBJS = $(ASM_SOURCES:.asm=.o) $(KERNEL_SOURCES:.c=.o)

# Имя ядра
KERNEL = myos.bin
ISO = myos.iso

# Цели по умолчанию
all: $(ISO)

# Сборка загрузочного ISO
$(ISO): $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $(ISO) isodir

# Компиляция ядра
$(KERNEL): $(OBJS)
	$(LD) -T linker.ld -o $@ $(OBJS) -lgcc

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция ассемблерных файлов
%.o: %.asm
	$(ASM) -f elf32 $< -o $@

# Очистка
clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -rf isodir

# Запуск в QEMU
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -m 256 -monitor stdio

# Отладка
debug: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -m 256 -s -S &
	i686-elf-gdb -ex "target remote localhost:1234" -ex "symbol-file $(KERNEL)"

.PHONY: all clean run debug
