.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, apic_timer_isr_wrap, apic_error_isr_wrap
.align   8

exc_wrapper:
    ;//Disable interrupts
    cli
    ;//Save the state of the currently running task
    ;//For ease of debugging
    call mtask_save_state
    ;//Get exception address
    pop rdx
    cld
    call krnl_exc
    iretq
exc_wrapper_code:
    ;//Disable interrupts
    cli
    ;//Move the stack pointer 8 bytes up
    add rsp, 8
    ;//Save the state of the currently running task
    ;//For ease of debugging
    call mtask_save_state
    ;//Get exception address
    pop rdx
    cld
    call krnl_exc
    iretq

apic_timer_isr_wrap:
    ;//Disable interrupts
    cli
    ;//Save RAX
    push rax
    ;//Check if multitasking is enabled
    call mtask_is_enabled
    cmp rax, 1
    ;//Restore RAX
    pop rax
    ;//Return if not
    je apic_timer_isr_wrap_cont
    ;//Re-enable interrupts; send EOI; return
    push r15
    mov r15, 0xFEE000B0
    mov dword ptr [r15], 0
    pop r15
    sti
    iretq
    apic_timer_isr_wrap_cont:
    call mtask_save_state
    call mtask_schedule
    jmp mtask_restore_state
