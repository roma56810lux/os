/**
 * kernel/kernel.c - Главный файл ядра
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// Заголовочные файлы
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "vbe.h"
#include "framebuffer.h"
#include "gui.h"
#include "terminal.h"
#include "commands.h"

// Размер стека ядра
#define KERNEL_STACK_SIZE 8192

// Глобальные переменные ядра
uint32_t kernel_stack[KERNEL_STACK_SIZE];

// Внешние переменные из загрузчика
extern uint32_t framebuffer_addr;
extern uint16_t screen_width;
extern uint16_t screen_height;
extern uint8_t screen_bpp;

// Прототипы функций
void kernel_main(void);
void kernel_panic(const char* message);
void init_system(void);
void main_loop(void);

/**
 * Точка входа ядра (вызывается из загрузчика)
 */
void _start(void) {
    // Инициализируем стек
    asm volatile(
        "mov %0, %%esp\n"
        "mov %%esp, %%ebp"
        : 
        : "r" (kernel_stack + KERNEL_STACK_SIZE)
        : "esp", "ebp"
    );
    
    // Вызываем основную функцию ядра
    kernel_main();
    
    // Если ядро вернулось - паника
    kernel_panic("Kernel returned!");
}

/**
 * Основная функция ядра
 */
void kernel_main(void) {
    // Инициализация системы
    init_system();
    
    // Основной цикл
    main_loop();
    
    // Мы не должны сюда попасть
    kernel_panic("Main loop exited!");
}

/**
 * Инициализация всех подсистем
 */
void init_system(void) {
    // Инициализация GDT
    gdt_init();
    
    // Инициализация IDT и прерываний
    idt_init();
    isr_init();
    irq_init();
    
    // Инициализация таймера
    timer_init(100); // 100 Гц
    
    // Инициализация клавиатуры
    keyboard_init();
    
    // Инициализация мыши
    mouse_init();
    
    // Инициализация графики
    vbe_init(framebuffer_addr, screen_width, screen_height, screen_bpp);
    framebuffer_init();
    
    // Инициализация GUI
    gui_init();
    
    // Инициализация терминала
    terminal_init();
    
    // Инициализация команд
    commands_init();
    
    // Включаем прерывания
    asm volatile("sti");
    
    // Отображаем стартовый экран
    gui_draw_desktop();
    terminal_print_banner();
    terminal_print_prompt();
}

/**
 * Главный цикл обработки событий
 */
void main_loop(void) {
    static uint32_t last_time = 0;
    static uint32_t frames = 0;
    uint32_t current_time;
    
    // Бесконечный цикл
    while (1) {
        // Получаем текущее время
        current_time = timer_get_ticks();
        
        // Обработка ввода с клавиатуры (уже в прерываниях)
        // Обработка мыши (уже в прерываниях)
        
        // Обновление курсора мыши
        if (current_time - last_time > 50) { // 20 FPS для курсора
            gui_update_cursor();
            last_time = current_time;
            frames++;
        }
        
        // Обработка команд терминала
        terminal_process_input();
        
        // Небольшая задержка для экономии процессорного времени
        // (в реальной ОС здесь будет переключение задач)
        asm volatile("hlt");
    }
}

/**
 * Паническая остановка системы
 */
void kernel_panic(const char* message) {
    // Отключаем прерывания
    asm volatile("cli");
    
    // Выводим сообщение об ошибке
    framebuffer_draw_string(10, 10, "KERNEL PANIC:", 0xFF0000);
    framebuffer_draw_string(10, 30, message, 0xFF0000);
    
    // Бесконечный цикл
    while (1) {
        asm volatile("hlt");
    }
}

/**
 * Обработчик системных вызовов
 */
void syscall_handler(registers_t* regs) {
    // Здесь будут обрабатываться системные вызовы
    // Пока просто заглушка
    (void)regs;
}