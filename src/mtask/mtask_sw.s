.intel_syntax noprefix
.globl   mtask_save_state, mtask_restore_state
.align   8

mtask_save_state:
    ;//Save RAX and RBX
    push rax
    push rbx
    ;//Load the target state pointer address into RAX
    mov rax, [mtask_cur_task]
    ;//Save the task state
    pop [rax+  8]      ;//RBX was pushed onto stack
    pop [rax+  0]      ;//as well as RAX
    mov [rax+ 16], rcx ;//save the rest of the registers directly
    mov [rax+ 24], rdx
    mov [rax+ 32], rsi
    mov [rax+ 40], rdi
    mov [rax+ 48], rbp
    lea r15, [rsp+48] ;//task's RSP was 48 bytes up
    mov [rax+ 56], r15
    mov [rax+ 64], r8
    mov [rax+ 72], r9
    mov [rax+ 80], r10
    mov [rax+ 88], r11
    mov [rax+ 96], r12
    mov [rax+104], r13
    mov [rax+112], r14
    mov [rax+120], r15
    mov r15, cr3
    mov [rax+128], r15
    mov r15, [rsp+  8] ;//task's RIP was at RSP+8
    mov [rax+136], r15
    mov r15, [rsp+ 24] ;//RFLAGS at RSP+24
    mov [rax+144], r15
    ret

mtask_restore_state:
    ;//Load the task state pointer into RBX
    mov rbx, [mtask_cur_task]
    ;//Restore CR3, R8-R15, RSP, RBP, RDI, RSI, RDX, RCX
    mov r8,  [rbx+128]
    mov cr3, r8
    mov r8,  [rbx+ 64]
    mov r9,  [rbx+ 72]
    mov r10, [rbx+ 80]
    mov r11, [rbx+ 88]
    mov r12, [rbx+ 96]
    mov r13, [rbx+104]
    mov r14, [rbx+112]
    mov r15, [rbx+120]
    mov rsp, [rbx+ 56]
    mov rbp, [rbx+ 48]
    mov rdi, [rbx+ 40]
    mov rsi, [rbx+ 32]
    mov rdx, [rbx+ 24]
    mov rcx, [rbx+ 16]
    mov rax, [rbx+  0]
    push [rbx+144] ;//RFLAGS
    popfq
    push [rbx+136] ;//RIP
    mov rbx, [rbx+  8]
    ret
