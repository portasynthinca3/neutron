.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, apic_timer_isr_wrap, apic_error_isr_wrap
.align   8

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
    ;//Disable interrupts
    cli
    cmp byte ptr [mtask_enabled], 1
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
