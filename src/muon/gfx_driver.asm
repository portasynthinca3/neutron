;Neutron project
;Graphics driver

gfx_go_best:						;switches the video mode to the best available one
									;  Saves video buffer linear address into ECX,
									;  resolution into EDX, bpp in AH
	call gfx_get_mode_list			;get mode list pointer in ES:DI
	gfx_go_best_loop:				;
		mov dx, word [es:di]		;load a mode number
		cmp dx, 0xffff				;0xffff = the end of the list
		je gfx_go_best_ret			;
		or dh, 1 << 6				;set the linear framebuffer flag
		push dx						;save DX
		call gfx_get_mode			;get mode parameters
		cmp ah, 32					;only try 32bpp modes
		jne gfx_go_best_skip		;
		test al, 1 << 0				;check if mode is supported
		jz gfx_go_best_skip			;
		test al, 1 << 4				;check if mode is a graphical one
		jz gfx_go_best_skip			;
		cmp edx, [ds:gfx_best_res]	;compare the resolution with the best currently discovered one
		jbe gfx_go_best_skip		;
		mov [ds:gfx_best_res], edx	;if we hadn't skipped yet, store resolution and number
		pop word [ds:gfx_best_mod]	;
		push word [ds:gfx_best_mod]	;
		gfx_go_best_skip:			;
		pop dx						;restore DX
		add di, 4					;move on to the next mode
		jmp gfx_go_best_loop		;
	gfx_go_best_ret:				;
	mov dx, [ds:gfx_best_mod]		;load the best mode
	call gfx_go_mode				;go to it
	ret								;return from subroutine

gfx_go_mode:						;switches the video mode to one which number is stored in DX
									;  Saves video buffer linear address into ECX,
									;  resolution into EDX, bpp in AH
	push bx							;save the registers
	push es							;
	mov ax, 4f02h					;set BIOS function: set VBE mode
	mov bx, dx						;VBE mode
	or bh, 01000000b				;raise the flag indicating that we want a flat video buffer
	int 10h							;call BIOS routine
	call gfx_get_mode				;get mode parameters
	pop es							;restore the registers
	pop bx							;
	ret								;return from subroutine

gfx_get_mode:						;returns information about a VBE mode
									;input: DX = mode number
									;output: ECX = linear framebuffer address
									;        EDX = resolution
									;        AH = bits per pixel
									;        AL = lower bits of mode attributes
	push di							;save DI
	push es							;save ES
	mov ax, 4f01h					;set BIOS function: get VBE mode properties
	mov cx, dx						;VBE mode
	push 0xf50						;load info into 0xf50:0x0000
	pop es							;
	xor di, di						;
	int 10h							;call BIOS routine
	mov dword ecx, [es:0x28]		;move the buffer linear address into ECX
	movzx edx, word [es:0x12]		;load width
	shl edx, 16						;
	mov dx, word [es:0x14]			;load height
	mov ah, byte [es:0x19]			;load bits per pixel
	mov al, byte [es:0x00]			;load mode attributes
	pop es							;restore ES
	pop di							;restore DI
	ret								;return from subroutine

gfx_get_mode_list:					;reads VBE mode list
									;output: list pointer in ES:DI
	push 0x1050						;load controller info into 0x1050:0x0000
	pop es							;
	xor di, di						;
	mov ax, 0x4f00					;
	int 10h							;
	mov eax, dword [es:di+0xe]		;load the list pointer into EAX
	mov di, ax						;AX[15:0] = DI
	shr eax, 16						;
	mov es, ax						;AX[31:16] = ES
	ret								;return from subroutine

gfx_best_res: dd 0					;the best resolution currently found by gfx_go_best
gfx_best_mod: dw 0					;the number of the best resolution currently found by gfx_go_best