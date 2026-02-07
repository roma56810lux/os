/**
 * kernel/idt.c - Реализация таблицы дескрипторов прерываний (IDT)
 */

#include "idt.h"
#include "isr.h"
#include "irq.h"
#include <string.h>

// Глобальные переменные IDT
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;
isr_t interrupt_handlers[IDT_ENTRIES];

/**
 * Установка дескриптора в IDT
 * @param num     Номер прерывания
 * @param base    Адрес обработчика прерывания
 * @param selector Селектор сегмента кода
 * @param flags   Флаги дескриптора
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

/**
 * Установка обработчика для прерывания
 * @param isr     Номер прерывания
 * @param handler Функция-обработчик
 */
void isr_install_handler(int isr, isr_t handler) {
    if (isr >= 0 && isr < IDT_ENTRIES) {
        interrupt_handlers[isr] = handler;
    }
}

/**
 * Удаление обработчика прерывания
 * @param isr Номер прерывания
 */
void isr_uninstall_handler(int isr) {
    if (isr >= 0 && isr < IDT_ENTRIES) {
        interrupt_handlers[isr] = 0;
    }
}

/**
 * Обработчик прерывания по умолчанию
 * @param r Регистры на момент прерывания
 */
static void default_handler(struct registers* r) {
    // В реальной ОС здесь будет обработка критических ошибок
    // Пока просто игнорируем
    (void)r;
}

/**
 * Инициализация IDT
 */
void idt_init(void) {
    // Устанавливаем указатель на IDT
    idtp.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idtp.base = (uint32_t)&idt;
    
    // Очищаем IDT
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);
    
    // Очищаем массив обработчиков
    memset(&interrupt_handlers, 0, sizeof(isr_t) * IDT_ENTRIES);
    
    // Устанавливаем обработчики исключений (0-31)
    idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    
    // Устанавливаем обработчики IRQ (32-47)
    idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT_INT);
    
    // Устанавливаем обработчик по умолчанию для всех прерываний
    for (int i = 0; i < IDT_ENTRIES; i++) {
        isr_install_handler(i, default_handler);
    }
    
    // Загружаем IDT
    idt_load((uint32_t)&idtp);
}