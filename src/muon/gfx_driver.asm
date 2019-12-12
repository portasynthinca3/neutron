;Neutron project
;Graphics driver

gfx_go_best:						;switches the video mode to the best available one
									;  Saves video buffer linear address into ECX,
									;  resolution into EDX, bpp in AH
	mov si, gfx_res_db				;point SI to the video database
	push bx							;save the position on screen
	gfx_go_best_loop:				;
	mov ax, word [cs:si]			;load the resolution
	shl eax, 16						;
	mov ax, word [cs:si + 2]		;
	cmp eax, 0						;EAX=0 indicates the end of the list
	je gfx_go_best_ret				;return in this case
	mov cl, 24						;try 24bpp
	call gfx_try_mode				;
	cmp al, 0						;check the success
	jne gfx_go_best_ret				;
	pop bx							;print the | character
	push cx							;
	mov ch, 0x0A					;
	mov cl, '|'						;
	call print_char					;
	pop cx							;
	push bx							;
	mov ax, word [cs:si]			;load the resolution again
	shl eax, 16						;
	mov ax, word [cs:si + 2]		;
	mov cl, 15						;try 15bpp
	call gfx_try_mode				;
	cmp al, 0						;check the success
	jne gfx_go_best_ret				;
	pop bx							;print the | character
	push cx							;
	mov ch, 0x0A					;
	mov cl, '|'						;
	call print_char					;
	pop cx							;
	push bx							;
	add si, 4						;point SI to the next entry
	jmp gfx_go_best_loop			;loop
	gfx_go_best_ret:				;
	pop bx							;restore position on screen
	ret								;return from subrountine

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

gfx_try_mode:						;try to find a VBE mode with resolution stored in EAX and bpp in CL
									;returns AL != 0 on success
	mov dx, 100h					;start with mode 100h
	gfx_try_mode_loop:				;
	push dx							;save DX
	push ax							;     AX
	push cx							;     CX
	call gfx_get_mode				;get mode information
	pop cx							;restore CX
	cmp ah, cl						;compare mode bpp with the desired one
	pop ax							;restore AX
	je gfx_try_mode_found_1			;make a resolution check if they are equal
	gfx_try_mode_false_alarm:		;
	pop dx							;restore DX
	inc dx							;try another mode number
	cmp dx, 301h					;but don't exceed mode 301h
	ja gfx_try_mode_not_found		;
	jmp gfx_try_mode_loop			;loop
	gfx_try_mode_found_1:			;jump occurs when bpp's are equal
	cmp eax, edx					;compare the mode resolutions
	je gfx_try_mode_found			;we found the one we've benn looking for if they are equal
	jmp gfx_try_mode_false_alarm	;if they aren't, continue searching
	gfx_try_mode_found:				;
	pop dx							;restore DX
	call gfx_go_mode				;go to that mode
	mov al, 1						;indicate the success
	ret								;return
	gfx_try_mode_not_found:			;
	xor al, al						;indicate an error
	ret								;return

gfx_get_mode:						;returns information about a VBE mode
									;input: DX = mode number
									;output: ECX = linear framebuffer address
									;        EDX = resolution
									;        AH = bits per pixel
	mov ax, 4f01h					;set BIOS function: get VBE mode properties
	mov cx, dx						;VBE mode
	push 0x1050						;load info into 0x1050:0x0000
	pop es							;
	xor di, di						;
	int 10h							;call BIOS routine
	mov dword ecx, [es:0x28]		;move the buffer linear address into ECX
	movzx edx, word [es:0x12]		;load width
	shl edx, 16						;
	mov dx, word [es:0x14]			;load height
	mov ah, byte [es:0x19]			;load bits per pixel
	ret								;return from subroutine

gfx_go_80x25x16t:					;switches the video mode to 80x25x16c text
	push ax							;save AX
	xor ah, ah						;set BIOS func.: change video mode
	mov al, 3h						;set video mode: 80x25x16c text
	int 10h							;call BIOS routine
	pop ax							;restore AX
	ret								;return from subroutine

gfx_res_db:							;Graphics Resolutions Database
									;Each database entry is 4 bytes long and has the following structure:
									;  word res_x
									;  word res_y
	dw 1600, 900
	dw 1600, 200
	dw 1280, 720
	dw 1024, 768
	dw 1280, 720
	dw 800, 600
	dw 640, 480
	dw 0, 0							;end