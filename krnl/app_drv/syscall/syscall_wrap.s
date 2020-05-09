.intel_syntax noprefix
.globl   syscall_wrapper

syscall_wrapper:
    ;//Save all necessary registers
    mov rbx, rsp
    ;//This instruction will be self-modified: movq rsp, <something>
    .byte 0x48, 0xbc
    .byte 0, 0, 0, 0, 0, 0, 0, 0
    push rbx
    push rcx
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rbp
    ;//Handle syscall (obvious huh)
    call syscall_handle
    ;//Restore saved registers
    pop rbp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop rcx
    pop rsp
    ;//Enable interrupts
    or r11, 1 << 9
    ;//System call return
    sysretq
