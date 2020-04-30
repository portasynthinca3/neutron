.intel_syntax noprefix
.globl   exc_0, exc_1, exc_2, exc_3, exc_4, exc_5, exc_6, exc_7, exc_8, exc_9, exc_10, exc_11, exc_12, exc_13, exc_14, exc_16, exc_17, exc_18, exc_19, exc_20, exc_30, apic_timer_isr_wrap, apic_error_isr_wrap
.align   8

;//Specific handlers for each exception
exc_0:
    push 0
    jmp exc_wrapper
exc_1:
    push 1
    jmp exc_wrapper
exc_2:
    push 2
    jmp exc_wrapper
exc_3:
    push 3
    jmp exc_wrapper
exc_4:
    push 4
    jmp exc_wrapper
exc_5:
    push 5
    jmp exc_wrapper
exc_6:
    push 6
    jmp exc_wrapper
exc_7:
    push 7
    jmp exc_wrapper
exc_8:
    push 8
    jmp exc_wrapper_code
exc_9:
    push 9
    jmp exc_wrapper
exc_10:
    push 10
    jmp exc_wrapper_code
exc_11:
    push 11
    jmp exc_wrapper_code
exc_12:
    push 12
    jmp exc_wrapper_code
exc_13:
    push 13
    jmp exc_wrapper_code
exc_14:
    push 14
    jmp exc_wrapper_code
exc_16:
    push 16
    jmp exc_wrapper
exc_17:
    push 17
    jmp exc_wrapper_code
exc_18:
    push 18
    jmp exc_wrapper
exc_19:
    push 19
    jmp exc_wrapper
exc_20:
    push 20
    jmp exc_wrapper
exc_30:
    push 30
    jmp exc_wrapper_code

exc_wrapper:
    ;//Disable interrupts
    cli
    ;//Save the state of the currently running task
    ;//For ease of debugging
    add rsp, 8
    ;//call mtask_save_state
    sub rsp, 8
    ;//Get exception number
    pop rcx
    ;//Get exception address
    pop rdx
    ;//Clear direction flag
    cld
    ;//Call the exception handler
    call krnl_exc
    iretq
exc_wrapper_code:
    ;//Disable interrupts
    cli
    ;//Save the state of the currently running task
    ;//For ease of debugging
    add rsp, 16
    ;//call mtask_save_state
    sub rsp, 16
    ;//Get exception number
    pop rcx
    ;//Get extra exception data
    pop rbx
    ;//Get exception address
    pop rdx
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
    mov r15, 0xFEE000B0
    mov dword ptr [r15], 0
    pop r15
    sti
    iretq
    apic_timer_isr_wrap_cont:
    call mtask_save_state
    call mtask_schedule
    jmp mtask_restore_state
