/**
 * include/gdt.h - Глобальная таблица дескрипторов (GDT)
 */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Структура дескриптора GDT
struct gdt_entry {
    uint16_t limit_low;     // Младшие 16 бит лимита
    uint16_t base_low;      // Младшие 16 бит базы
    uint8_t base_middle;    // Следующие 8 бит базы
    uint8_t access;         // Флаги доступа
    uint8_t granularity;    // Гранулярность и старшие биты лимита
    uint8_t base_high;      // Старшие 8 бит базы
} __attribute__((packed));

// Структура указателя на GDT (для инструкции LGDT)
struct gdt_ptr {
    uint16_t limit;         // Размер GDT минус 1
    uint32_t base;          // Адрес GDT
} __attribute__((packed));

// Структура TSS (Task State Segment)
struct tss_entry {
    uint32_t prev_tss;      // Указатель на предыдущий TSS
    uint32_t esp0;          // Указатель стека привилегированного уровня 0
    uint32_t ss0;           // Селектор стека привилегированного уровня 0
    uint32_t esp1;          // Указатель стека уровня 1
    uint32_t ss1;           // Селектор стека уровня 1
    uint32_t esp2;          // Указатель стека уровня 2
    uint32_t ss2;           // Селектор стека уровня 2
    uint32_t cr3;           // Указатель на таблицу страниц
    uint32_t eip;           // Счетчик команд
    uint32_t eflags;        // Флаги
    uint32_t eax;           // Регистры
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;            // Селекторы сегментов
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;           // Селектор LDT
    uint16_t trap;          // Флаг трассировки
    uint16_t iomap_base;    // Смещение I/O карты
} __attribute__((packed));

// Внешние объявления
extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush();

// Константы для селекторов GDT
#define GDT_KERNEL_CODE_SEG 0x08
#define GDT_KERNEL_DATA_SEG 0x10
#define GDT_USER_CODE_SEG   0x18
#define GDT_USER_DATA_SEG   0x20
#define GDT_TSS_SEG         0x28

// Константы для флагов доступа
#define GDT_ACCESS_PRESENT      (1 << 7)
#define GDT_ACCESS_RING0        (0 << 5)
#define GDT_ACCESS_RING3        (3 << 5)
#define GDT_ACCESS_SEGMENT      (1 << 4)
#define GDT_ACCESS_CODE         (1 << 3)
#define GDT_ACCESS_CONFORMING   (1 << 2)
#define GDT_ACCESS_READABLE     (1 << 1)
#define GDT_ACCESS_WRITABLE     (1 << 1)
#define GDT_ACCESS_ACCESSED     (1 << 0)

// Константы для флагов гранулярности
#define GDT_FLAG_32BIT          (1 << 6)
#define GDT_FLAG_4K_GRANULARITY (1 << 7)

// Функции
void gdt_init(void);
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
void tss_init(uint32_t idx, uint16_t ss0, uint32_t esp0);
void tss_set_stack(uint32_t esp0);

// Глобальные переменные
extern struct gdt_entry gdt_entries[6];
extern struct gdt_ptr gdt_pointer;
extern struct tss_entry tss;

#endif // GDT_H