/**
 * kernel/gdt.c - Реализация глобальной таблицы дескрипторов (GDT)
 */

#include "gdt.h"

// Глобальные переменные GDT
struct gdt_entry gdt_entries[6];
struct gdt_ptr gdt_pointer;
struct tss_entry tss;

/**
 * Установка дескриптора в GDT
 * @param index Индекс дескриптора
 * @param base  Базовый адрес сегмента
 * @param limit Лимит сегмента
 * @param access Флаги доступа
 * @param flags Дополнительные флаги
 */
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt_entries[index].base_low = base & 0xFFFF;
    gdt_entries[index].base_middle = (base >> 16) & 0xFF;
    gdt_entries[index].base_high = (base >> 24) & 0xFF;
    
    gdt_entries[index].limit_low = limit & 0xFFFF;
    gdt_entries[index].granularity = (limit >> 16) & 0x0F;
    
    gdt_entries[index].granularity |= flags & 0xF0;
    gdt_entries[index].access = access;
}

/**
 * Инициализация TSS (Task State Segment)
 * @param idx  Индекс TSS в GDT
 * @param ss0  Селектор стека уровня 0
 * @param esp0 Указатель стека уровня 0
 */
void tss_init(uint32_t idx, uint16_t ss0, uint32_t esp0) {
    // Вычисляем базовый адрес и лимит TSS
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = base + sizeof(struct tss_entry);
    
    // Устанавливаем дескриптор TSS в GDT
    gdt_set_entry(idx, base, limit, 
                  0xE9,  // Present, DPL=3, 32-bit TSS
                  0x00);
    
    // Обнуляем TSS
    memset(&tss, 0, sizeof(struct tss_entry));
    
    // Устанавливаем стек уровня 0
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    
    // Устанавливаем сегменты
    tss.cs = 0x0B;      // Код сегмент уровня 3
    tss.ss = 0x13;      // Стек сегмент уровня 3
    tss.ds = 0x13;
    tss.es = 0x13;
    tss.fs = 0x13;
    tss.gs = 0x13;
    
    // I/O карта
    tss.iomap_base = sizeof(struct tss_entry);
}

/**
 * Установка стека уровня 0 в TSS
 * @param esp0 Указатель стека уровня 0
 */
void tss_set_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}

/**
 * Инициализация GDT
 */
void gdt_init(void) {
    // Устанавливаем указатель на GDT
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_pointer.base = (uint32_t)&gdt_entries;
    
    // Нулевой дескриптор (обязателен)
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // Код сегмент уровня 0 (ядро)
    gdt_set_entry(1, 0, 0xFFFFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | 
                  GDT_ACCESS_CODE | GDT_ACCESS_READABLE,
                  GDT_FLAG_32BIT | GDT_FLAG_4K_GRANULARITY);
    
    // Данные сегмент уровня 0 (ядро)
    gdt_set_entry(2, 0, 0xFFFFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | 
                  GDT_ACCESS_WRITABLE,
                  GDT_FLAG_32BIT | GDT_FLAG_4K_GRANULARITY);
    
    // Код сегмент уровня 3 (пользователь)
    gdt_set_entry(3, 0, 0xFFFFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SEGMENT | 
                  GDT_ACCESS_CODE | GDT_ACCESS_READABLE,
                  GDT_FLAG_32BIT | GDT_FLAG_4K_GRANULARITY);
    
    // Данные сегмент уровня 3 (пользователь)
    gdt_set_entry(4, 0, 0xFFFFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SEGMENT | 
                  GDT_ACCESS_WRITABLE,
                  GDT_FLAG_32BIT | GDT_FLAG_4K_GRANULARITY);
    
    // Инициализация TSS
    tss_init(5, GDT_KERNEL_DATA_SEG, 0x90000);
    
    // Загружаем GDT
    gdt_flush((uint32_t)&gdt_pointer);
    
    // Загружаем TSS
    tss_flush();
}