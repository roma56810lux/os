#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
uint8_t vga_color = 0x0F;

void vga_clear() {
    for(int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (vga_color << 8) | ' ';
    }
}

void vga_puts(const char* str) {
    static int index = 0;
    while(*str) {
        if(*str == '\n') index += VGA_WIDTH - (index % VGA_WIDTH);
        else vga_buffer[index++] = (vga_color << 8) | *str;
        str++;
    }
}

void kernel_main() {
    vga_clear();
    vga_puts("Graphical OS v0.1\n");
    vga_puts("Initializing...\n");
    
    // Инициализация графики
    init_graphics();
    
    // Запуск приложений
    // paint_app();
    // calc_app();
    
    while(1);
}