; kernel/idt_asm.asm - Ассемблерные обработчики прерываний

section .text

; Внешние функции из C
extern isr_handler
extern irq_handler

; Макрос для создания обработчика исключения без кода ошибки
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push byte 0        ; Заглушка для кода ошибки
        push byte %1       ; Номер прерывания
        jmp isr_common
%endmacro

; Макрос для создания обработчика исключения с кодом ошибки
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        push byte %1       ; Номер прерывания
        jmp isr_common
%endmacro

; Макрос для создания обработчика IRQ
%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push byte 0
        push byte %2
        jmp irq_common
%endmacro

; Создаем обработчики исключений (0-31)
ISR_NOERRCODE 0   ; Деление на ноль
ISR_NOERRCODE 1   ; Отладка
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Точка останова
ISR_NOERRCODE 4   ; Переполнение
ISR_NOERRCODE 5   ; Выход за границы
ISR_NOERRCODE 6   ; Неверный код операции
ISR_NOERRCODE 7   ; Устройство недоступно
ISR_ERRCODE   8   ; Двойное исключение
ISR_NOERRCODE 9   ; Зарезервировано
ISR_ERRCODE   10  ; Неверный TSS
ISR_ERRCODE   11  ; Сегмент отсутствует
ISR_ERRCODE   12  ; Ошибка сегмента стека
ISR_ERRCODE   13  ; Общая ошибка защиты
ISR_ERRCODE   14  ; Ошибка страницы
ISR_NOERRCODE 15  ; Зарезервировано
ISR_NOERRCODE 16  ; Ошибка сопроцессора
ISR_NOERRCODE 17  ; Проверка выравнивания
ISR_NOERRCODE 18  ; Проверка машины
ISR_NOERRCODE 19  ; Ошибка SIMD
ISR_NOERRCODE 20  ; Виртуализация
ISR_NOERRCODE 21  ; Зарезервировано
ISR_NOERRCODE 22  ; Зарезервировано
ISR_NOERRCODE 23  ; Зарезервировано
ISR_NOERRCODE 24  ; Зарезервировано
ISR_NOERRCODE 25  ; Зарезервировано
ISR_NOERRCODE 26  ; Зарезервировано
ISR_NOERRCODE 27  ; Зарезервировано
ISR_NOERRCODE 28  ; Зарезервировано
ISR_NOERRCODE 29  ; Зарезервировано
ISR_NOERRCODE 30  ; Исключение безопасности
ISR_NOERRCODE 31  ; Зарезервировано

; Создаем обработчики IRQ (32-47)
IRQ 0,  32   ; Таймер
IRQ 1,  33   ; Клавиатура
IRQ 2,  34   ; Каскадирование
IRQ 3,  35   ; COM2
IRQ 4,  36   ; COM1
IRQ 5,  37   ; LPT2
IRQ 6,  38   ; Дисковод
IRQ 7,  39   ; LPT1
IRQ 8,  40   ; CMOS таймер
IRQ 9,  41   ; Свободно
IRQ 10, 42   ; Свободно
IRQ 11, 43   ; Свободно
IRQ 12, 44   ; PS/2 мышь
IRQ 13, 45   ; FPU
IRQ 14, 46   ; Первичный ATA
IRQ 15, 47   ; Вторичный ATA

; Общий обработчик для исключений
isr_common:
    ; Сохраняем все регистры
    pusha
    
    ; Сохраняем сегментные регистры
    push ds
    push es
    push fs
    push gs
    
    ; Загружаем сегмент данных ядра
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Передаем указатель на структуру registers в C-обработчик
    push esp
    call isr_handler
    add esp, 4
    
    ; Восстанавливаем сегментные регистры
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Восстанавливаем регистры общего назначения
    popa
    
    ; Очищаем код ошибки и номер прерывания из стека
    add esp, 8
    
    ; Восстанавливаем состояние процессора и возвращаемся
    sti
    iret

; Общий обработчик для IRQ
irq_common:
    ; Сохраняем все регистры
    pusha
    
    ; Сохраняем сегментные регистры
    push ds
    push es
    push fs
    push gs
    
    ; Загружаем сегмент данных ядра
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Передаем указатель на структуру registers в C-обработчик
    push esp
    call irq_handler
    add esp, 4
    
    ; Восстанавливаем сегментные регистры
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Восстанавливаем регистры общего назначения
    popa
    
    ; Очищаем код ошибки и номер прерывания из стека
    add esp, 8
    
    ; Восстанавливаем состояние процессора и возвращаемся
    sti
    iret

; Функция загрузки IDT
global idt_load
idt_load:
    mov eax, [esp + 4]    ; Получаем указатель на idt_ptr из стека
    lidt [eax]            ; Загружаем IDT
    ret