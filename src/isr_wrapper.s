.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, apic_timer_isr_wrap
.align   8

_isr_push_64:
    push rbp
    mov rbp, rsp
    sub rsp, 128
    mov [rsp+  0], rax
    mov [rsp+  8], rbx
    mov [rsp+ 16], rcx
    mov [rsp+ 24], rdx
    mov [rsp+ 32], rsi
    mov [rsp+ 40], rdi
    mov [rsp+ 48], rbp ;//RSP actually
    mov rbp, [rbp]
    mov [rsp+ 56], rbp ;//actual RBP
    mov [rsp+ 64], r8
    mov [rsp+ 72], r9
    mov [rsp+ 80], r10
    mov [rsp+ 88], r11
    mov [rsp+ 96], r12
    mov [rsp+104], r13
    mov [rsp+112], r14
    mov [rsp+120], r15
    ret

_isr_rest_64:
    lea rbp,  [rsp+128]
    mov r15,  [rsp+120]
    mov r14,  [rsp+112]
    mov r13,  [rsp+104]
    mov r12,  [rsp+ 96]
    mov r11,  [rsp+ 88]
    mov r10,  [rsp+ 80]
    mov r9,   [rsp+ 72]
    mov r8,   [rsp+ 64]
    mov rdi,  [rsp+ 40]
    mov rsi,  [rsp+ 32]
    mov rdx,  [rsp+ 24]
    mov rcx,  [rsp+ 16]
    mov rbx,  [rsp+  8]
    mov rax,  [rsp+  0]
    mov rsp,  [rsp+ 48]
    mov rbp,  [rbp]
    ret

exc_wrapper:
    pop rdx
    cld
    call krnl_exc
    iretq
exc_wrapper_code:
    pop rcx
    pop rdx
    cld
    call krnl_exc
    iretq

apic_timer_isr_wrap:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    call timr_tick
    call apic_eoi

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
