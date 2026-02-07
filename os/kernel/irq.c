/**
 * kernel/irq.c - Обработчики аппаратных прерываний (IRQ)
 */

#include "irq.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "terminal.h"
#include <stddef.h>

// Массив обработчиков IRQ
irq_handler_t irq_handlers[16];

// Порт контроллера прерываний для отправки EOI
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

// Команда End Of Interrupt
#define PIC_EOI   0x20

/**
 * Обработчик IRQ (вызывается из ассемблера)
 * @param regs Регистры на момент прерывания
 */
void irq_handler(struct registers* regs) {
    // Проверяем, есть ли зарегистрированный обработчик для этого IRQ
    if (irq_handlers[regs->int_no - 32] != NULL) {
        irq_handlers[regs->int_no - 32](regs);
    } else {
        // Если обработчика нет, выводим предупреждение (только для отладки)
        // В реальной системе лучше игнорировать
        #ifdef DEBUG
        terminal_printf("Unhandled IRQ: %d\n", regs->int_no - 32);
        #endif
    }
    
    // Отправляем End Of Interrupt контроллеру прерываний
    if (regs->int_no >= 40) {
        // Если прерывание от ведомого контроллера
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

/**
 * Настройка контроллера прерываний (PIC)
 */
static void pic_remap(void) {
    // Сохраняем маски прерываний
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    
    // Инициализация ведущего PIC
    outb(PIC1_CMD, 0x11);  // ICW1: начинаем инициализацию
    outb(PIC1_DATA, 0x20); // ICW2: вектор прерываний для ведущего (начинается с 32)
    outb(PIC1_DATA, 0x04); // ICW3: ведущий подключен к IRQ2
    outb(PIC1_DATA, 0x01); // ICW4: режим 8086
    
    // Инициализация ведомого PIC
    outb(PIC2_CMD, 0x11);  // ICW1: начинаем инициализацию
    outb(PIC2_DATA, 0x28); // ICW2: вектор прерываний для ведомого (начинается с 40)
    outb(PIC2_DATA, 0x02); // ICW3: ведомый подключен к IRQ2 ведущего
    outb(PIC2_DATA, 0x01); // ICW4: режим 8086
    
    // Восстанавливаем маски прерываний
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

/**
 * Регистрация обработчика IRQ
 * @param irq Номер IRQ (0-15)
 * @param handler Указатель на функцию-обработчик
 */
void irq_register_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
        
        // Снимаем маску с этого прерывания
        if (irq < 8) {
            uint8_t mask = inb(PIC1_DATA);
            mask &= ~(1 << irq);
            outb(PIC1_DATA, mask);
        } else {
            uint8_t mask = inb(PIC2_DATA);
            mask &= ~(1 << (irq - 8));
            outb(PIC2_DATA, mask);
        }
    }
}

/**
 * Удаление обработчика IRQ
 * @param irq Номер IRQ (0-15)
 */
void irq_unregister_handler(int irq) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = NULL;
        
        // Устанавливаем маску на это прерывание
        if (irq < 8) {
            uint8_t mask = inb(PIC1_DATA);
            mask |= (1 << irq);
            outb(PIC1_DATA, mask);
        } else {
            uint8_t mask = inb(PIC2_DATA);
            mask |= (1 << (irq - 8));
            outb(PIC2_DATA, mask);
        }
    }
}

/**
 * Инициализация обработчиков IRQ
 */
void irq_init(void) {
    // Очищаем массив обработчиков
    for (int i = 0; i < 16; i++) {
        irq_handlers[i] = NULL;
    }
    
    // Перенастраиваем контроллер прерываний
    pic_remap();
    
    // Регистрируем стандартные обработчики
    irq_register_handler(0, timer_handler);    // Таймер
    irq_register_handler(1, keyboard_handler); // Клавиатура
    irq_register_handler(12, mouse_handler);   // Мышь
    
    // Устанавливаем маски на все прерывания по умолчанию
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    // Снимаем маски с нужных прерываний
    irq_register_handler(0, timer_handler);    // Размаскируем таймер
    irq_register_handler(1, keyboard_handler); // Размаскируем клавиатуру
    irq_register_handler(12, mouse_handler);   // Размаскируем мышь
}

/**
 * Вспомогательные функции для работы с портами ввода-вывода
 */

// Чтение байта из порта
uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Запись байта в порт
void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Чтение слова из порта
uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Запись слова в порт
void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}