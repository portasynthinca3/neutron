.intel_syntax noprefix
.globl   mtask_save_state, mtask_restore_state
.align   8

mtask_save_state:
    ;//Save RAX
    push rax
    ;//Load the current task pointer into RAX
    call mtask_get_cur_task
    ;//Store RAX
    pop [rax+  0]
    ;//Store the rest of GPRs
    mov [rax+  8], rbx
    mov [rax+ 16], rcx
    mov [rax+ 24], rdx
    mov [rax+ 32], rsi
    mov [rax+ 40], rdi
    mov [rax+ 48], rbp
    mov [rax+ 64], r8
    mov [rax+ 72], r9
    mov [rax+ 80], r10
    mov [rax+ 88], r11
    mov [rax+ 96], r12
    mov [rax+104], r13
    mov [rax+112], r14
    mov [rax+120], r15
    ;//Load the last task switch TSC into RCX
    mov rcx, [rax+160]
    ;//Read TSC into RDX
    push rax
    rdtsc ;//Actually reads into EDX:EAX, need to shift
    shl rdx, 32
    or  rdx, rax
    pop rax
    ;//Store RDX as the new "last switch TSC"
    mov [rax+160], rdx
    ;//Subtract RCX from RDX to get the difference
    sub rdx, rcx
    ;//Store it as the amount of cycles the task took
    mov [rax+152], rdx
    ;//Load non-GPRs into GPRs
    mov r8,  cr3
    mov r9,  [rsp+ 8] ;//RIP
    mov r10, [rsp+24] ;//RFLAGS
    mov r11, [rsp+32] ;//RSP
    ;//Store them
    mov [rax+128], r8
    mov [rax+136], r9
    mov [rax+144], r10
    mov [rax+ 56], r11
    ;//Increment the switch counter
    inc qword ptr [rax+168]
    ret

mtask_restore_state:
    ;//Load the current task pointer into RAX
    call mtask_get_cur_task
    ;//Load RSP
    mov rsp, [rax+ 56]
    ;//Load non-GPRs
    mov rbx, [rax+128]
    mov rcx, [rax+136]
    mov rdx, [rax+144]
    mov cr3, rbx
    push     rcx
    push     rdx
    ;//Load GPRs
    mov rbx, [rax+  8]
    mov rcx, [rax+ 16]
    mov rdx, [rax+ 24]
    mov rsi, [rax+ 32]
    mov rdi, [rax+ 40]
    mov rbp, [rax+ 48]
    mov r8,  [rax+ 64]
    mov r9,  [rax+ 72]
    mov r10, [rax+ 80]
    mov r11, [rax+ 88]
    mov r12, [rax+ 96]
    mov r13, [rax+104]
    mov r14, [rax+112]
    ;//Send EOI
    mov r15, 0xFEE000B0
    mov dword ptr [r15], 0
    ;//Load R15
    mov r15, [rax+120]
    ;//Load RAX
    mov rax, [rax+  0]
    ;//Load RFALGS
    popfq
    ;//Enable interrupts
    sti
    ;//Load RIP
    ret
