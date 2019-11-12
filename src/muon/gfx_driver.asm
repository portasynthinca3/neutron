;Neutron project
;Graphics driver

gfx_go_800x600x256c:				;switches the video mode to 800x600x256c graphical
	push bx							;save the registers
	push es							;
	push ax							;
	mov ax, 4f02h					;set BIOS function: set VESA mode
	mov bx, 101h					;VESA mode 101h
	or bh, 01000000b				;set some kind of bit (idk what this does, but without this line it doesn't work XD)
	int 10h							;call BIOS routine
	mov ax, 4f01h					;set BIOS function: get VESA mode properties
	mov cx, 101h					;VESA mode 101h
	push 0x1050						;load info into 0x1050:0x0000
	pop es							;
	xor di, di						;
	int 10h							;call BIOS function
	mov dword ecx, [es:0x28]		;move the buffer linear address into ECX
	pop ax							;restore the registers
	pop es							;
	pop bx							;
	ret								;return from subroutine

gfx_go_80x25x16t:					;switches the video mode to 80x25x16c text
	push ax							;save AX
	xor ah, ah						;set BIOS func.: change video mode
	mov al, 3h						;set video mode: 80x25x16c text
	int 10h							;call BIOS routine
	pop ax							;restore AX
	ret								;return from subroutine