/**
 * kernel/isr.c - Обработчики исключений процессора
 */

#include "isr.h"
#include "idt.h"
#include "terminal.h"
#include "framebuffer.h"
#include <stddef.h>

// Массив обработчиков исключений
isr_t exception_handlers[32];

// Сообщения об исключениях
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/**
 * Обработчик исключений (вызывается из ассемблера)
 * @param regs Регистры на момент исключения
 */
void isr_handler(struct registers* regs) {
    // Проверяем, есть ли зарегистрированный обработчик
    if (exception_handlers[regs->int_no] != NULL) {
        exception_handlers[regs->int_no](regs);
    } else {
        // Если обработчика нет, выводим информацию об исключении
        handle_exception(regs);
    }
}

/**
 * Обработка исключения по умолчанию
 * @param regs Регистры на момент исключения
 */
void handle_exception(struct registers* regs) {
    // Отключаем прерывания
    asm volatile("cli");
    
    // Сохраняем текущее состояние экрана
    static uint32_t saved_background[80*25];
    static int saved = 0;
    
    if (!saved) {
        // Переключаемся в текстовый режим для вывода ошибки
        // (в реальной ОС нужно сохранить графический буфер)
        saved = 1;
    }
    
    // Выводим информацию об исключении
    framebuffer_draw_rect(0, 0, 1024, 768, 0x000033);
    framebuffer_draw_string(100, 100, "EXCEPTION OCCURRED:", 0xFF0000);
    
    char buffer[64];
    
    // Номер исключения
    framebuffer_draw_string(100, 130, "Exception: ", 0xFFFFFF);
    itoa(regs->int_no, buffer, 10);
    framebuffer_draw_string(220, 130, buffer, 0xFFFF00);
    framebuffer_draw_string(260, 130, exception_messages[regs->int_no], 0xFFFF00);
    
    // Код ошибки
    framebuffer_draw_string(100, 150, "Error Code: ", 0xFFFFFF);
    itoa(regs->err_code, buffer, 10);
    framebuffer_draw_string(220, 150, buffer, 0xFFFF00);
    
    // Регистры
    framebuffer_draw_string(100, 180, "Registers:", 0xFFFFFF);
    
    framebuffer_draw_string(100, 200, "EAX: ", 0xCCCCCC);
    itoa(regs->eax, buffer, 16);
    framebuffer_draw_string(180, 200, buffer, 0x00FF00);
    
    framebuffer_draw_string(100, 220, "EBX: ", 0xCCCCCC);
    itoa(regs->ebx, buffer, 16);
    framebuffer_draw_string(180, 220, buffer, 0x00FF00);
    
    framebuffer_draw_string(100, 240, "ECX: ", 0xCCCCCC);
    itoa(regs->ecx, buffer, 16);
    framebuffer_draw_string(180, 240, buffer, 0x00FF00);
    
    framebuffer_draw_string(100, 260, "EDX: ", 0xCCCCCC);
    itoa(regs->edx, buffer, 16);
    framebuffer_draw_string(180, 260, buffer, 0x00FF00);
    
    framebuffer_draw_string(100, 280, "ESI: ", 0xCCCCCC);
    itoa(regs->esi, buffer, 16);
    framebuffer_draw_string(180, 280, buffer, 0x00FF00);
    
    framebuffer_draw_string(100, 300, "EDI: ", 0xCCCCCC);
    itoa(regs->edi, buffer, 16);
    framebuffer_draw_string(180, 300, buffer, 0x00FF00);
    
    // EIP
    framebuffer_draw_string(100, 330, "EIP: ", 0xFFFFFF);
    itoa(regs->eip, buffer, 16);
    framebuffer_draw_string(180, 330, buffer, 0xFF00FF);
    
    // CS
    framebuffer_draw_string(100, 350, "CS: ", 0xFFFFFF);
    itoa(regs->cs, buffer, 16);
    framebuffer_draw_string(180, 350, buffer, 0xFF00FF);
    
    // EFLAGS
    framebuffer_draw_string(100, 370, "EFLAGS: ", 0xFFFFFF);
    itoa(regs->eflags, buffer, 16);
    framebuffer_draw_string(180, 370, buffer, 0xFF00FF);
    
    // Инструкция на экране
    framebuffer_draw_string(100, 400, "System halted. Please restart.", 0xFF0000);
    
    // Останавливаем систему
    while(1) {
        asm volatile("hlt");
    }
}

/**
 * Регистрация обработчика исключения
 * @param num Номер исключения (0-31)
 * @param handler Указатель на функцию-обработчик
 */
void register_exception_handler(uint8_t num, isr_t handler) {
    if (num < 32) {
        exception_handlers[num] = handler;
    }
}

/**
 * Инициализация обработчиков исключений
 */
void isr_init(void) {
    // Очищаем массив обработчиков
    for (int i = 0; i < 32; i++) {
        exception_handlers[i] = NULL;
    }
    
    // Устанавливаем обработчик страничных ошибок (для отладки)
    register_exception_handler(EXCEPTION_PAGE_FAULT, page_fault_handler);
    
    // Устанавливаем обработчик общей защиты
    register_exception_handler(EXCEPTION_GENERAL_PROTECTION, gp_handler);
}

/**
 * Обработчик ошибки страницы (#PF)
 */
void page_fault_handler(struct registers* regs) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    framebuffer_draw_string(10, 450, "Page fault at address: ", 0xFF0000);
    
    char buffer[32];
    itoa(faulting_address, buffer, 16);
    framebuffer_draw_string(250, 450, buffer, 0xFFFFFF);
    
    // Продолжаем выполнение (в реальной ОС здесь будет обработка)
    (void)regs;
}

/**
 * Обработчик общей ошибки защиты (#GP)
 */
void gp_handler(struct registers* regs) {
    framebuffer_draw_string(10, 470, "General Protection Fault", 0xFF0000);
    
    char buffer[32];
    itoa(regs->err_code, buffer, 16);
    framebuffer_draw_string(250, 470, buffer, 0xFFFFFF);
    
    // Продолжаем выполнение (в реальной ОС здесь будет обработка)
    (void)regs;
}

/**
 * Преобразование числа в строку (вспомогательная функция)
 */
void itoa(int value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    // Проверяем основание
    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }
    
    rc = ptr = str;
    
    // Для отрицательных чисел
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }
    
    low = ptr;
    
    do {
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while (value);
    
    *ptr-- = '\0';
    
    // Переворачиваем строку
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
}