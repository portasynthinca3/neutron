.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, irq0_wrap, irq1_wrap, irq2_wrap, irq3_wrap, irq4_wrap, irq5_wrap, irq6_wrap, irq7_wrap, irq8_wrap, irq9_wrap, irq10_wrap, irq11_wrap, irq12_wrap, irq13_wrap, irq14_wrap, irq15_wrap
.align   4

exc_wrapper:
    pop rdx
    cld
    call krnl_exc
    iret
exc_wrapper_code:
    pop rcx
    pop rdx
    cld
    call krnl_exc
    iret

irq0_wrap:
    ;//pushaq
    push 0
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq1_wrap:
    ;//pushaq
    push 1
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq2_wrap:
    ;//pushaq
    push 2
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq3_wrap:
    ;//pushaq
    push 3
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq4_wrap:
    ;//pushaq
    push 4
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq5_wrap:
    ;//pushaq
    push 5
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq6_wrap:
    ;//pushaq
    push 6
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq7_wrap:
    ;//pushaq
    push 7
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq8_wrap:
    ;//pushaq
    push 8
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq9_wrap:
    ;//pushaq
    push 9
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq10_wrap:
    ;//pushaq
    push 10
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq11_wrap:
    ;//pushaq
    push 11
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq12_wrap:
    ;//pushaq
    push 12
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq13_wrap:
    ;//pushaq
    push 13
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq14_wrap:
    ;//pushaq
    push 14
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
irq15_wrap:
    ;//pushaq
    push 14
    call krnl_irq
    add esp, 4
    ;//popaq
    iret
