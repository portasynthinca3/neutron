.intel_syntax noprefix
.globl   syscall_wrapper

syscall_wrapper:
    ;//Save all necessary registers
    push rbx
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbp
    ;//Handle syscall (obvious huh)
    call handle_syscall
    ;//Restore saved registers
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop rbx
    ;//Interrupt return
    iretq
