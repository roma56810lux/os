; boot/boot.asm - Основной загрузчик
bits 16
org 0x7C00

%define KERNEL_OFFSET 0x1000    ; Адрес загрузки ядра
%define STACK_OFFSET  0x9000    ; Начало стека

; Точка входа
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, STACK_OFFSET
    sti

    ; Сохраняем номер диска
    mov [BOOT_DRIVE], dl

    ; Приветственное сообщение
    mov si, MSG_LOADING
    call print_string

    ; Загружаем ядро с диска
    mov bx, KERNEL_OFFSET
    mov dh, 32                  ; Количество секторов (32*512 = 16KB)
    mov dl, [BOOT_DRIVE]
    call disk_load

    ; Получаем информацию о VBE
    call vbe_get_info

    ; Переходим в защищенный режим
    call switch_to_pm

    ; Сюда мы не должны вернуться
    jmp $

; ------------------------------
; Подпрограммы реального режима
; ------------------------------

print_string:
    pusha
    mov ah, 0x0E
.repeat:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .repeat
.done:
    popa
    ret

disk_load:
    pusha
    push dx
    
    mov ah, 0x02    ; Функция чтения диска
    mov al, dh      ; Количество секторов
    mov ch, 0x00    ; Цилиндр 0
    mov dh, 0x00    ; Головка 0
    mov cl, 0x02    ; Начинаем со 2-го сектора
    
    int 0x13
    jc disk_error   ; Если ошибка
    
    pop dx
    cmp dh, al      ; Проверяем сколько секторов прочитано
    jne disk_error
    
    popa
    ret

disk_error:
    mov si, MSG_DISK_ERROR
    call print_string
    jmp $

; ------------------------------
; VBE функции
; ------------------------------

vbe_get_info:
    mov ax, 0x4F00          ; Функция получения информации VBE
    mov di, VBE_INFO        ; Буфер для информации
    int 0x10
    cmp ax, 0x004F          ; Проверяем успешность
    jne vbe_error
    
    ; Ищем нужный режим (1024x768x32)
    mov si, [VBE_INFO.video_modes]
    mov ds, si
    
.find_mode:
    lodsw
    cmp ax, 0xFFFF          ; Конец списка
    je mode_not_found
    
    mov cx, ax
    mov ax, 0x4F01          ; Получить информацию о режиме
    mov di, MODE_INFO
    int 0x10
    cmp ax, 0x004F
    jne .next_mode
    
    ; Проверяем параметры
    mov ax, [MODE_INFO.width]
    cmp ax, 1024
    jne .next_mode
    
    mov ax, [MODE_INFO.height]
    cmp ax, 768
    jne .next_mode
    
    mov al, [MODE_INFO.bpp]
    cmp al, 32
    jne .next_mode
    
    ; Нашли подходящий режим
    mov [SELECTED_MODE], cx
    mov si, MSG_VBE_OK
    call print_string
    ret
    
.next_mode:
    jmp .find_mode

vbe_error:
    mov si, MSG_VBE_ERROR
    call print_string
    jmp $

mode_not_found:
    mov si, MSG_MODE_NOT_FOUND
    call print_string
    jmp $

; ------------------------------
; Переход в защищенный режим
; ------------------------------

switch_to_pm:
    cli                     ; Отключаем прерывания
    lgdt [gdt_descriptor]   ; Загружаем GDT
    
    ; Включаем A20 линию
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Устанавливаем бит PE
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Прыжок в 32-битный код
    jmp CODE_SEG:init_pm

; ------------------------------
; 32-битный код
; ------------------------------

bits 32

init_pm:
    ; Инициализируем сегменты
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Инициализируем стек
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Устанавливаем графический режим
    call set_graphics_mode
    
    ; Переход к ядру
    call KERNEL_OFFSET
    
    ; Зависаем если ядро вернулось
    jmp $

set_graphics_mode:
    ; Устанавливаем VBE режим
    mov ax, 0x4F02
    mov bx, [SELECTED_MODE]
    or bx, 0x4000          ; Включаем линейный буфер
    int 0x10
    cmp ax, 0x004F
    jne .error
    
    ; Сохраняем информацию о framebuffer для ядра
    mov eax, [MODE_INFO.framebuffer]
    mov [FRAMEBUFFER_ADDR], eax
    mov ax, [MODE_INFO.width]
    mov [SCREEN_WIDTH], ax
    mov ax, [MODE_INFO.height]
    mov [SCREEN_HEIGHT], ax
    mov al, [MODE_INFO.bpp]
    mov [SCREEN_BPP], al
    
    ret
.error:
    mov ebx, MSG_MODE_SET_FAILED
    call print_string_pm
    jmp $

print_string_pm:
    ; Простая печать в текстовом режиме (до инициализации графики)
    pusha
    mov edx, 0xB8000
.loop:
    mov al, [ebx]
    mov ah, 0x0F
    cmp al, 0
    je .done
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop
.done:
    popa
    ret

; ------------------------------
; Данные
; ------------------------------

bits 16

; Сообщения
MSG_LOADING           db "Loading OS...", 0x0D, 0x0A, 0
MSG_DISK_ERROR        db "Disk read error!", 0x0D, 0x0A, 0
MSG_VBE_OK            db "VBE OK", 0x0D, 0x0A, 0
MSG_VBE_ERROR         db "VBE error!", 0x0D, 0x0A, 0
MSG_MODE_NOT_FOUND    db "Graphic mode not found!", 0x0D, 0x0A, 0
MSG_MODE_SET_FAILED   db "Failed to set mode!", 0

; Переменные
BOOT_DRIVE            db 0
SELECTED_MODE         dw 0
FRAMEBUFFER_ADDR      dd 0
SCREEN_WIDTH          dw 0
SCREEN_HEIGHT         dw 0
SCREEN_BPP            db 0

; Структуры VBE
VBE_INFO:
    .signature        db "VBE2"
    .version          dw 0
    .oem              dd 0
    .capabilities     dd 0
    .video_modes      dd 0
    .total_memory     dw 0
    times 512-($-VBE_INFO) db 0

MODE_INFO:
    .attributes       dw 0
    .window_a         db 0
    .window_b         db 0
    .granularity      dw 0
    .window_size      dw 0
    .segment_a        dw 0
    .segment_b        dw 0
    .win_func_ptr     dd 0
    .pitch            dw 0
    .width            dw 0
    .height           dw 0
    .w_char           db 0
    .y_char           db 0
    .planes           db 0
    .bpp              db 0
    .banks            db 0
    .memory_model     db 0
    .bank_size        db 0
    .image_pages      db 0
    .reserved0        db 0
    .red_mask         db 0
    .red_position     db 0
    .green_mask       db 0
    .green_position   db 0
    .blue_mask        db 0
    .blue_position    db 0
    .reserved_mask    db 0
    .reserved_position db 0
    .direct_color_attributes db 0
    .framebuffer      dd 0
    times 256-($-MODE_INFO) db 0

; GDT
gdt_start:
    ; Нулевой дескриптор
    dq 0
    
    ; Дескриптор кода (0-4GB)
    dw 0xFFFF          ; Limit
    dw 0x0000          ; Base (low)
    db 0x00            ; Base (middle)
    db 10011010b       ; Access (present, ring 0, code, readable)
    db 11001111b       ; Flags + Limit (high)
    db 0x00            ; Base (high)
    
    ; Дескриптор данных (0-4GB)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b       ; Access (present, ring 0, data, writable)
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_start + 8
DATA_SEG equ gdt_start + 16

; Заполнение до 512 байт и сигнатура загрузчика
times 510-($-$$) db 0
dw 0xAA55