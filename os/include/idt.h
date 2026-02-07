/**
 * include/idt.h - Таблица дескрипторов прерываний (IDT)
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Максимальное количество дескрипторов прерываний
#define IDT_ENTRIES 256

// Структура дескриптора IDT
struct idt_entry {
    uint16_t base_low;      // Младшие 16 бит адреса обработчика
    uint16_t selector;      // Селектор сегмента кода
    uint8_t  zero;          // Всегда 0
    uint8_t  flags;         // Флаги типа, привилегий и присутствия
    uint16_t base_high;     // Старшие 16 бит адреса обработчика
} __attribute__((packed));

// Структура указателя на IDT (для инструкции LIDT)
struct idt_ptr {
    uint16_t limit;         // Размер IDT минус 1
    uint32_t base;          // Адрес IDT
} __attribute__((packed));

// Структура для сохранения регистров при прерывании
struct registers {
    uint32_t ds;            // Сегмент данных
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Регистры общего назначения
    uint32_t int_no, err_code; // Номер прерывания и код ошибки
    uint32_t eip, cs, eflags, useresp, ss; // Автоматически сохраняемые процессором
} __attribute__((packed));

// Тип указателя на обработчик прерываний
typedef void (*isr_t)(struct registers*);

// Внешние ассемблерные функции
extern void idt_load(uint32_t idt_ptr);
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

// Константы для флагов IDT
#define IDT_FLAG_PRESENT    0x80
#define IDT_FLAG_RING0      0x00
#define IDT_FLAG_RING3      0x60
#define IDT_FLAG_32BIT_INT  0x0E  // 32-битные шлюзы прерываний
#define IDT_FLAG_32BIT_TRAP 0x0F  // 32-битные шлюзы ловушек

// Номера IRQ (аппаратных прерываний)
#define IRQ0  32  // Таймер
#define IRQ1  33  // Клавиатура
#define IRQ2  34  // Каскадирование
#define IRQ3  35  // COM2
#define IRQ4  36  // COM1
#define IRQ5  37  // LPT2
#define IRQ6  38  // Дисковод
#define IRQ7  39  // LPT1
#define IRQ8  40  // CMOS таймер
#define IRQ9  41  // Свободно
#define IRQ10 42  // Свободно
#define IRQ11 43  // Свободно
#define IRQ12 44  // PS/2 мышь
#define IRQ13 45  // FPU
#define IRQ14 46  // Первичный ATA
#define IRQ15 47  // Вторичный ATA

// Исключения процессора (0-31)
enum {
    EXCEPTION_DIVIDE_ERROR = 0,
    EXCEPTION_DEBUG,
    EXCEPTION_NMI,
    EXCEPTION_BREAKPOINT,
    EXCEPTION_OVERFLOW,
    EXCEPTION_BOUND_RANGE,
    EXCEPTION_INVALID_OPCODE,
    EXCEPTION_DEVICE_NOT_AVAILABLE,
    EXCEPTION_DOUBLE_FAULT,
    EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN,
    EXCEPTION_INVALID_TSS,
    EXCEPTION_SEGMENT_NOT_PRESENT,
    EXCEPTION_STACK_SEGMENT_FAULT,
    EXCEPTION_GENERAL_PROTECTION,
    EXCEPTION_PAGE_FAULT,
    EXCEPTION_RESERVED_15,
    EXCEPTION_X87_FPU_ERROR,
    EXCEPTION_ALIGNMENT_CHECK,
    EXCEPTION_MACHINE_CHECK,
    EXCEPTION_SIMD_FPU_ERROR,
    EXCEPTION_VIRTUALIZATION,
    EXCEPTION_RESERVED_21,
    EXCEPTION_RESERVED_22,
    EXCEPTION_RESERVED_23,
    EXCEPTION_RESERVED_24,
    EXCEPTION_RESERVED_25,
    EXCEPTION_RESERVED_26,
    EXCEPTION_RESERVED_27,
    EXCEPTION_RESERVED_28,
    EXCEPTION_RESERVED_29,
    EXCEPTION_SECURITY_EXCEPTION,
    EXCEPTION_RESERVED_31
};

// Функции
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void isr_install_handler(int isr, isr_t handler);
void isr_uninstall_handler(int isr);

// Глобальные переменные
extern struct idt_entry idt[IDT_ENTRIES];
extern struct idt_ptr idtp;
extern isr_t interrupt_handlers[IDT_ENTRIES];

#endif // IDT_H