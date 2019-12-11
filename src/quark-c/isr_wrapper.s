.globl   isr_wrapper, isr_wrapper_code
.align   4
 
isr_wrapper:
    pop %edx
    cld
    call quark_isr
    iret

isr_wrapper_code:
    pop %ecx
    pop %edx
    cld
    call quark_isr
    iret
