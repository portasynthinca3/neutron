.intel_syntax noprefix
.globl   exc_wrapper, exc_wrapper_code, apic_timer_isr_wrap
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
    cmp byte ptr [mtask_ready], 0
    jne apic_timer_isr_wrap_continue
    iretq
    apic_timer_isr_wrap_continue:
    ;//Clear the "ready" flag
    mov byte ptr [mtask_ready], 0
    call mtask_save_state
    ;//call timr_tick
    call mtask_schedule
    call apic_eoi
    jmp mtask_restore_state
