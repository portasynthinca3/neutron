.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, irq0_wrap, irq1_wrap, irq2_wrap, irq3_wrap, irq4_wrap, irq5_wrap, irq6_wrap, irq7_wrap, irq8_wrap, irq9_wrap, irq10_wrap, irq11_wrap, irq12_wrap, irq13_wrap, irq14_wrap, irq15_wrap
.align   4

exc_wrapper:
    pop edx
    cld
    call quark_exc
    iret
exc_wrapper_code:
    pop ecx
    pop edx
    cld
    call quark_exc
    iret

irq0_wrap:
    pushad
    push 0
    call quark_irq
    add esp, 4
    popad
    iret
irq1_wrap:
    pushad
    push 1
    call quark_irq
    add esp, 4
    popad
    iret
irq2_wrap:
    pushad
    push 2
    call quark_irq
    add esp, 4
    popad
    iret
irq3_wrap:
    pushad
    push 3
    call quark_irq
    add esp, 4
    popad
    iret
irq4_wrap:
    pushad
    push 4
    call quark_irq
    add esp, 4
    popad
    iret
irq5_wrap:
    pushad
    push 5
    call quark_irq
    add esp, 4
    popad
    iret
irq6_wrap:
    pushad
    push 6
    call quark_irq
    add esp, 4
    popad
    iret
irq7_wrap:
    pushad
    push 7
    call quark_irq
    add esp, 4
    popad
    iret
irq8_wrap:
    pushad
    push 8
    call quark_irq
    add esp, 4
    popad
    iret
irq9_wrap:
    pushad
    push 9
    call quark_irq
    add esp, 4
    popad
    iret
irq10_wrap:
    pushad
    push 10
    call quark_irq
    add esp, 4
    popad
    iret
irq11_wrap:
    pushad
    push 11
    call quark_irq
    add esp, 4
    popad
    iret
irq12_wrap:
    pushad
    push 12
    call quark_irq
    add esp, 4
    popad
    iret
irq13_wrap:
    pushad
    push 13
    call quark_irq
    add esp, 4
    popad
    iret
irq14_wrap:
    pushad
    push 14
    call quark_irq
    add esp, 4
    popad
    iret
irq15_wrap:
    pushad
    push 14
    call quark_irq
    add esp, 4
    popad
    iret
