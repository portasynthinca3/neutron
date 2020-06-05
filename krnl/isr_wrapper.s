.intel_syntax noprefix
.globl   exc_0, exc_1, exc_2, exc_3, exc_4, exc_5, exc_6, exc_7, exc_8, exc_9, exc_10, exc_11, exc_12, exc_13, exc_14, exc_16, exc_17, exc_18, exc_19, exc_20, exc_30, apic_timer_isr_wrap, apic_error_isr_wrap, ps21_isr_wrap, ps22_isr_wrap
.align   8

;//Specific handlers for each exception
exc_0:
    movb [rsp-128], 0
    jmp exc_wrapper
exc_1:
    movb [rsp-128], 1
    jmp exc_wrapper
exc_2:
    movb [rsp-128], 2
    jmp exc_wrapper
exc_3:
    movb [rsp-128], 3
    jmp exc_wrapper
exc_4:
    movb [rsp-128], 4
    jmp exc_wrapper
exc_5:
    movb [rsp-128], 5
    jmp exc_wrapper
exc_6:
    movb [rsp-128], 6
    jmp exc_wrapper
exc_7:
    movb [rsp-128], 7
    jmp exc_wrapper
exc_8:
    add rsp, 8
    movb [rsp-128], 8
    jmp exc_wrapper
exc_9:
    movb [rsp-128], 9
    jmp exc_wrapper
exc_10:
    add rsp, 8
    movb [rsp-128], 10
    jmp exc_wrapper
exc_11:
    add rsp, 8
    movb [rsp-128], 11
    jmp exc_wrapper
exc_12:
    add rsp, 8
    movb [rsp-128], 12
    jmp exc_wrapper
exc_13:
    add rsp, 8
    movb [rsp-128], 13
    jmp exc_wrapper
exc_14:
    add rsp, 8
    movb [rsp-128], 14
    jmp exc_wrapper
exc_16:
    movb [rsp-128], 16
    jmp exc_wrapper
exc_17:
    add rsp, 8
    movb [rsp-128], 17
    jmp exc_wrapper
exc_18:
    movb [rsp-128], 18
    jmp exc_wrapper
exc_19:
    movb [rsp-128], 19
    jmp exc_wrapper
exc_20:
    movb [rsp-128], 20
    jmp exc_wrapper
exc_30:
    add rsp, 8
    movb [rsp-128], 30
    jmp exc_wrapper

exc_wrapper:
    ;//Disable interrupts
    cli
    ;//Save task state
    call mtask_save_state
    ;//Clear direction flag
    cld
    ;//Call the exception handler
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
    mov r15, 0xFFFFFFFFFFFFE0B0
    mov dword ptr [r15], 0
    pop r15
    sti
    iretq
    apic_timer_isr_wrap_cont:
    call mtask_save_state
    call mtask_schedule
    jmp mtask_restore_state

ps21_isr_wrap:
    ;//Disable interrupts
    cli
    ;//Save task state
    call mtask_save_state
    ;//Call the handler
    call ps21_intr
    ;//Restore task state (return)
    jmp mtask_restore_state

ps22_isr_wrap:
    ;//Disable interrupts
    cli
    ;//Save task state
    call mtask_save_state
    ;//Call the handler
    call ps22_intr
    ;//Restore task state (return)
    jmp mtask_restore_state
