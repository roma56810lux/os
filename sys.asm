global syscall_handler
global outb, inb

section .text
syscall_handler:
    pusha
    ; Обработка системных вызовов
    cmp eax, 1
    je .draw_pixel
    cmp eax, 2
    je .get_input
    popa
    iret

.draw_pixel:
    ; ebx = x, ecx = y, edx = color
    call draw_pixel
    popa
    iret

.get_input:
    call get_keyboard_input
    popa
    iret

outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

inb:
    mov dx, [esp + 4]
    in al, dx
    ret