.globl   exc_wrapper, exc_wrapper_code, irq0_wrap, irq1_wrap, irq2_wrap, irq3_wrap, irq4_wrap, irq5_wrap, irq6_wrap, irq7_wrap, irq8_wrap, irq9_wrap, irq10_wrap, irq11_wrap, irq12_wrap, irq13_wrap, irq14_wrap, irq15_wrap
.align   4
 
exc_wrapper:
    pop %edx
    cld
    call quark_exc
    iret
exc_wrapper_code:
    pop %ecx
    pop %edx
    cld
    call quark_exc
    iret

irq0_wrap:
    pushal
    push $0
    call quark_irq
    add $4, %esp
    popal
    iret
irq1_wrap:
    pushal
    push $1
    call quark_irq
    add $4, %esp
    popal
    iret
irq2_wrap:
    pushal
    push $2
    call quark_irq
    add $4, %esp
    popal
    iret
irq3_wrap:
    pushal
    push $3
    call quark_irq
    add $4, %esp
    popal
    iret
irq4_wrap:
    pushal
    push $4
    call quark_irq
    add $4, %esp
    popal
    iret
irq5_wrap:
    pushal
    push $5
    call quark_irq
    add $4, %esp
    popal
    iret
irq6_wrap:
    pushal
    push $6
    call quark_irq
    add $4, %esp
    popal
    iret
irq7_wrap:
    pushal
    push $7
    call quark_irq
    add $4, %esp
    popal
    iret
irq8_wrap:
    pushal
    push $8
    call quark_irq
    add $4, %esp
    popal
    iret
irq9_wrap:
    pushal
    push $9
    call quark_irq
    add $4, %esp
    popal
    iret
irq10_wrap:
    pushal
    push $10
    call quark_irq
    add $4, %esp
    popal
    iret
irq11_wrap:
    pushal
    push $11
    call quark_irq
    add $4, %esp
    popal
    iret
irq12_wrap:
    pushal
    push $12
    call quark_irq
    add $4, %esp
    popal
    iret
irq13_wrap:
    pushal
    push $13
    call quark_irq
    add $4, %esp
    popal
    iret
irq14_wrap:
    pushal
    push $14
    call quark_irq
    add $4, %esp
    popal
    iret
irq15_wrap:
    pushal
    push $14
    call quark_irq
    add $4, %esp
    popal
    iret
