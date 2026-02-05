global kernel_entry
extern kernel_main

section .text
kernel_entry:
    mov esp, 0x90000
    call kernel_main
    cli
    hlt