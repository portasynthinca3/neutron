;Neutron project
;Panic logic

%define gqv_ptr_pan_idt 6									;GQV-pointer: IDT selector
%define gqv_siz_pan_idt 4									;GQV-size:    >>

db "PANIC"

panic_init:													;set the IDT up
															;input: none
															;output: none
															;
	push fs													;save the registers
	push edi												;
	push edx												;
	push gs													;
	push es													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
															;
	mov edx, (256 * 16) + 6									;allocate 256 IDT entries 16 bytes each plus 6 bytes for the descriptor
	call malloc												;
	mov dx, gs												;move GS to ES
	mov es, dx												;
	call gdt_laddr											;get the linear address of the allocated buffer
	mov edi, eax											;set EDI to the liner address of the buffer
	add eax, 6												;add 6 to the address, as the IDT starts this many bytes from the start of the buffer
	mov word [ds:edi], 256 * 16								;write IDT limit to the IDT descriptor
	mov dword [ds:edi+2], eax								;write IDT linear offset to the IDT descriptor
	lidt [edi]												;load the freshly created IDT descriptor
	add edi, 6												;add 6 to the address, as the IDT starts this many bytes from the start of the buffer
	xor bx, bx												;boring stuff: set the exception handlers
	mov eax, panic_exc_de									;
	call panic_idt_entry_write								;
	mov bx, 1*8											;
	mov eax, panic_exc_db									;
	call panic_idt_entry_write								;
	mov bx, 3*8											;
	mov eax, panic_exc_bp									;
	call panic_idt_entry_write								;
	mov bx, 4*8											;
	mov eax, panic_exc_of									;
	call panic_idt_entry_write								;
	mov bx, 5*8											;
	mov eax, panic_exc_br									;
	call panic_idt_entry_write								;
	mov bx, 6*8											;
	mov eax, panic_exc_ud									;
	call panic_idt_entry_write								;
	mov bx, 7*8											;
	mov eax, panic_exc_nm									;
	call panic_idt_entry_write								;
	mov bx, 8*8											;
	mov eax, panic_exc_df									;
	call panic_idt_entry_write								;
	mov bx, 10*8											;
	mov eax, panic_exc_ts									;
	call panic_idt_entry_write								;
	mov bx, 11*8											;
	mov eax, panic_exc_np									;
	call panic_idt_entry_write								;
	mov bx, 12*8											;
	mov eax, panic_exc_ss									;
	call panic_idt_entry_write								;
	mov bx, 13*8											;
	mov eax, panic_exc_gp									;
	call panic_idt_entry_write								;
	mov bx, 14*8											;
	mov eax, panic_exc_pf									;
	call panic_idt_entry_write								;
	mov bx, 30*8											;
	mov eax, panic_exc_sx									;
	call panic_idt_entry_write								;
															;
	pop es													;restore the registers
	pop gs													;
	pop edx													;
	pop edi													;
	pop fs													;
															;
	ret														;return from subroutine

panic_idt_entry_write:										;sets an IDT entry for an interrupt
															;input: EAX = function pointer
															;       EDI = IDT pointer
															;       BX = IDT selector
															;output: none
															;CAUTION: absolutely destroys EAX and upper bits of EBX
															;https://bit.ly/2pSQQfV
															;
	and ebx, 0x0000FFFF										;only let the lower bits of EBX to make it through
															;
	mov word [ds:edi+ebx], ax								;load fn ptr[0:15]
	mov ax, cs												;load CS as the GDT selector
	mov word [ds:edi+ebx+2], ax								;
	mov word [ds:edi+ebx+4], 1000111000000000b				;set options: level 0, interrupt gate, don't switch tasks
	shr eax, 16												;expose fn ptr[16:31] in AX
	mov word [ds:edi+ebx+6], ax								;load fn ptr[16:31]
	;mov dword [ds:edi+ebx+8], 0								;fn ptr[32:63] = 0
	;mov dword [ds:edi+ebx+12], 0							;reserved = 0
															;
	ret														;return from subroutine

panic_exc_de:												;called on the #DE exception (divide by zero)
	push eax												;save the original EAX value
	mov eax, 0												;set EAX indicating it's a #DE exception
	jmp panic_general										;jump to the general panic handler
panic_exc_db:												;called on the #BE exception (debug)
	push eax												;save the original EAX value
	mov eax, 1												;set EAX indicating it's a #DB exception
	jmp panic_general										;jump to the general panic handler
panic_exc_bp:												;called on the #BP exception (breakpoint)
	push eax												;save the original EAX value
	mov eax, 3												;set EAX indicating it's a #BP exception
	jmp panic_general										;jump to the general panic handler
panic_exc_of:												;called on the #OF exception (overflow)
	push eax												;save the original EAX value
	mov eax, 4												;set EAX indicating it's a #OF exception
	jmp panic_general										;jump to the general panic handler
panic_exc_br:												;called on the #BR exception (bound range exceeded)
	push eax												;save the original EAX value
	mov eax, 5												;set EAX indicating it's a #BR exception
	jmp panic_general										;jump to the general panic handler
panic_exc_ud:												;called on the #UD exception (invalid opcode)
	push eax												;save the original EAX value
	mov eax, 6												;set EAX indicating it's a #UD exception
	jmp panic_general										;jump to the general panic handler
panic_exc_nm:												;called on the #NM exception (device not available)
	push eax												;save the original EAX value
	mov eax, 7												;set EAX indicating it's a #NM exception
	jmp panic_general										;jump to the general panic handler
panic_exc_df:												;called on the #DF exception (double fault)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 8												;set EAX indicating it's a #DF exception
	jmp panic_general										;jump to the general panic handler
panic_exc_ts:												;called on the #TS exception (invalid TSS)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 10												;set EAX indicating it's a #TS exception
	jmp panic_general										;jump to the general panic handler
panic_exc_np:												;called on the #NP exception (segment not present)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 11												;set EAX indicating it's a #NP exception
	jmp panic_general										;jump to the general panic handler
panic_exc_ss:												;called on the #SS exception (stack-segment fault)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 12												;set EAX indicating it's a #SS exception
	jmp panic_general										;jump to the general panic handler
panic_exc_gp:												;called on the #GP exception (general protection fault)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 13												;set EAX indicating it's a #GP exception
	jmp panic_general										;jump to the general panic handler
panic_exc_pf:												;called on the #PF exception (page fault)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 14												;set EAX indicating it's a #PF exception
	jmp panic_general										;jump to the general panic handler
panic_exc_sx:												;called on the #SX exception (security exception)
	add esp, 4												;add 4 to ESP to discard the zero pushed by the exception
	push eax												;save the original EAX value
	mov eax, 30												;set EAX indicating it's a #SX exception
	jmp panic_general										;jump to the general panic handler

panic_general:												;panic handler
															;input: EAX = fault number, last pushed value = EAX before
															;output: none
															;comment: is supposed to be run with a jump, not a call
															;         also, never returns
															;
	push eax												;save EAX
															;
	mov al, color_black										;clear the screen with black color
	call gfx_fill											;
															;
	push ebx												;save EBX
	push esi												;save ESI
	mov al, 0x28											;print the panic message in bright red
	xor ebx, ebx											;
	mov esi, panic_message									;
	call gfx_draw_str										;
	pop esi													;restore ESI
	pop ebx													;restore EBX
															;
	pop eax													;restore EAX
	push edx												;save EDX
	push ebx												;save EBX
	push ax													;save AX
	mov edx, eax											;print error ID in hex
	mov ebx, scrn_w * 8										;
	mov al, 0x28											;
	call gfx_put_32bit_hex									;
	pop ax													;restore AX
	pop ebx													;restore EBX
	pop edx													;restore EDX
															;
	pop eax													;restore previous EAX
	push edx												;save EDX
	push ebx												;save EBX
	push ax													;save AX
	mov edx, eax											;print EAX in hex
	mov ebx, scrn_w * 8 * 3									;
	mov al, 0x2A											;
	call gfx_put_32bit_hex									;
	pop ax													;restore AX
	pop ebx													;restore EBX
	pop edx													;restore EDX
															;
	push edx												;save EDX
	push ebx												;save EBX
	mov edx, ebx											;print EBX in hex
	mov ebx, scrn_w * 8 * 4									;
	mov al, 0x2A											;
	call gfx_put_32bit_hex									;
	pop ebx													;restore EBX
	pop edx													;restore EDX
															;
	push edx												;save EDX
	mov edx, ecx											;print ECX in hex
	mov ebx, scrn_w * 8 * 5									;
	mov al, 0x2A											;
	call gfx_put_32bit_hex									;
	pop edx													;restore EDX
															;
	mov ebx, scrn_w * 8 * 6									;print EDX in hex
	mov al, 0x2A											;
	call gfx_put_32bit_hex									;
															;
	mov edx, esi											;print ESI in hex
	mov ebx, scrn_w * 8 * 8									;
	mov al, 0x2C											;
	call gfx_put_32bit_hex									;

	mov edx, edi											;print EDI in hex
	mov ebx, scrn_w * 8 * 9									;
	mov al, 0x2C											;
	call gfx_put_32bit_hex									;

	mov dx, es												;print ES in hex
	and edx, 0x0000FFFF										;
	mov ebx, scrn_w * 8 * 11								;
	mov al, 0x2F											;
	call gfx_put_32bit_hex									;

	mov dx, fs												;print FS in hex
	mov ebx, scrn_w * 8 * 12								;
	mov al, 0x2F											;
	call gfx_put_32bit_hex									;

	mov dx, gs												;print GS in hex
	and edx, 0x0000FFFF										;
	mov ebx, scrn_w * 8 * 13								;
	mov al, 0x2F											;
	call gfx_put_32bit_hex									;

	pop edx													;load exception EIP
	mov ebx, scrn_w * 8 * 15								;print it
	mov al, 0x37											;
	call gfx_put_32bit_hex									;

	pop dx													;load exception CS
	and edx, 0x0000FFFF										;
	mov ebx, scrn_w * 8 * 16								;print it
	mov al, 0x37											;
	call gfx_put_32bit_hex									;

	mov edx, esp											;print ESP
	mov ebx, scrn_w * 8 * 18								;
	mov al, 0x23											;
	call gfx_put_32bit_hex									;
	
	mov ebx, scrn_w * 8 * 19								;
	xor ah, ah												;clear AH
	panic_strace:											;print the stack trace
		pop word dx											;load one word
		mov al, 0x23										;
		call gfx_put_16bit_hex								;print the word
		add ebx, 6											;move one cheracter to the right
		inc ah												;indicate it
		cmp ah, 8											;check if 8 words were printed
		jne panic_strace_skip								;
		add ebx, (scrn_w * 8) - (6 * 5 * 8)					;go to the next line
		panic_strace_skip:									;
		cmp esp, stack_top									;check if the end of the stack has been reached
		jb panic_strace										;continue stack printout if not

	call gfx_flip											;transfer dbuf->vram
															;
	jmp $													;hang

panic_message: db "Quark has panicked and couldn't continue. ID; EAX - EDX; ESI, EDI; ES - GS; EIP, CS; ESP, trace:", 0