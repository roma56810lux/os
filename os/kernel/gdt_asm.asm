; kernel/gdt_asm.asm - Ассемблерные функции для GDT

section .text
    global gdt_flush     ; Объявляем gdt_flush глобальной
    global tss_flush     ; Объявляем tss_flush глобальной

; Функция для загрузки GDT
; Сигнатура: void gdt_flush(uint32_t gdt_ptr)
gdt_flush:
    mov eax, [esp + 4]   ; Получаем указатель на структуру gdt_ptr из стека
    lgdt [eax]           ; Загружаем GDT

    ; Обновляем сегментные регистры
    mov ax, 0x10         ; Селектор сегмента данных ядра (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Делаем дальний прыжок для обновления cs
    jmp 0x08:.flush      ; 0x08 - селектор кода ядра
.flush:
    ret

; Функция для загрузки TSS
; Сигнатура: void tss_flush()
tss_flush:
    mov ax, 0x28         ; Селектор TSS (0x28)
    ltr ax               ; Загружаем TSS
    ret