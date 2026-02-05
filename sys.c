#include <stdint.h>

#define VBE_MODE 0x118  // 1024x768x16

void init_graphics() {
    // Инициализация VESA/VBE
    // Установка графического режима
}

void draw_pixel(int x, int y, uint32_t color) {
    // Реализация отрисовки пикселя
    uint8_t* fb = (uint8_t*)0xFD000000; // Адрес framebuffer
    // Расчет положения пикселя
}

void syscall(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    asm volatile(
        "int $0x80"
        : : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
    );
}