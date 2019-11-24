;Neutron project
;Graphics driver

;standard VGA/VESA 256-color palette: https://upload.wikimedia.org/wikipedia/commons/6/66/VGA_palette_with_black_borders.svg

%include "fonts/neutral.nfnt.asm"							;include the standard "Neutral" font

%define gqv_ptr_gfx_dbuf 0									;GQV-pointer: double buffer selector
%define gqv_siz_gfx_dbuf 2									;GQV-size:    >>
%define gqv_ptr_gfx_mbla 2									;GQV-pointer: main buffer linear address
%define gqv_siz_gfx_mbla 4									;GQV-size:    >>
%define gqv_ptr_gfx_sizx 10									;GQV-pointer: screen width
%define fqv_sic_gfx_sizx 2									;GQV-size:    >>
%define gqv_ptr_gfx_sizy 12									;GQV-pointer: screen height
%define fqv_sic_gfx_sizy 2									;GQV-size:    >>

db "GFX"

gfx_init:													;preforms some initialization stuff
															;input: none
															;output: none
															;
	push fs													;save the registers
	push gs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
															;
	mov edx, scrn_w * scrn_h								;allocate the double buffer
	call malloc												;
	mov word [fs:gqv_ptr_gfx_dbuf], gs						;save the dbuf selector
	mov dword [fs:gqv_ptr_gfx_mbla], ecx					;save the MBLA
															;
	pop gs													;restore the registers
	pop fs													;
															;
	ret														;return from subroutine
	
gfx_flip:													;move the double buffer to the main video RAM
															;input: none
															;output: none
															;
	push esi												;save all registers
	push edi												;
	push ds													;
	push es													;
	push fs													;
	push eax												;
	push ebx												;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
	mov dword ebx, [fs:gqv_ptr_gfx_mbla]					;load main buf linar address into EBX
															;
	xor esi, esi											;clear ESI (source)
	mov edi, ebx											;set EDI (destination) to main VRAM
	mov ax, es												;swap DS and ES
	mov bx, ds												;
	mov ds, ax												;
	mov es, bx												;
	mov ecx, scrn_w * scrn_h/4								;repeat (screen_size) / 4 times (move this many doublewords)
	rep movsd												;actually, move the doublewords
															;
	pop ebx													;restore all registers
	pop eax													;
	pop fs													;
	pop es													;
	pop ds													;
	pop edi													;
	pop esi													;
	ret														;return from subroutine

gfx_fill:													;fills an entire screen with one color
															;input: AL = color
															;output: none
															;
	push edi												;save the registers
	push es													;
	push fs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
															;
	xor edi, edi											;clear EDI
	gfx_fill_cycle:											;
		mov [es:edi], al									;set one pixel
		inc edi												;increment the pointer
		cmp edi, scrn_w * scrn_h							;check if all of the pixels were set
		jb gfx_fill_cycle									;continue if not
															;
	pop fs													;restore the registers
	pop es													;
	pop edi													;
	ret														;return from subroutine

gfx_xy_to_ebx:												;converts X/Y coordinates into EBX value
															;input: BX = X
															;       DX = Y
															;output: EBX = the required format
															;
	push eax												;save the registers
	push dx													;
	push ecx												;
															;
	mov ax, dx												;load Y into the accumulator
	mov cx, scrn_w											;load the screen width into CX
	mul cx													;multiply AX by CX and store the result in DX:AX
	push bx													;save BX
	mov bx, dx												;load the higher 16 bits of the result into BX
	shl ebx, 16												;shift them to the right
	mov bx, ax												;load the lower 16 bits of the result into BX, EBX now contains DX:AX
	xor ecx, ecx											;clear ECX
	pop cx													;load the saved BX value into CX
	add ebx, ecx											;add the X coordinate
															;
	pop ecx													;restore the registers
	pop dx													;
	pop eax													;
															;
	ret														;return from subroutine

gfx_draw_pixel:												;draws a pixel
															;input: EBX = pixel coordinate
															;       AL = color
															;
	push es													;save the registers
	push fs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
															;
	mov [es:ebx], al										;set the pixel
															;
	pop fs													;restore the registers
	pop es													;
															;
	ret														;return from subroutine

gfx_draw_cur:												;draws a curser
															;input: EBX = top-left coordinate
															;       AL = color
															;
	push es													;save the registers
	push fs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
															;
	mov [es:ebx], al										;set the pixels
	mov [es:ebx+1], al										;
	mov [es:ebx+2], al										;
	mov [es:ebx+(1*scrn_w)], al								;
	mov [es:ebx+(2*scrn_w)], al								;
	mov [es:ebx+(1*scrn_w)+1], al							;
	mov [es:ebx+(2*scrn_w)+2], al							;
	mov [es:ebx+(3*scrn_w)+3], al							;
															;
	pop fs													;restore the registers
	pop es													;
															;
	ret														;return from subroutine
	
gfx_draw_line_hor:											;draws a horizontal line on the screen
															;input: AL = color
															;       EBX = coordinate
															;       CX = width
															;
	push es													;save the registers
	push fs													;
	push ecx												;
	push ebx												;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
															;
	and ecx, 0x0000FFFF										;mask the higher bits of ECX out
															;
	gfx_dl_loop:											;
		mov byte [es:ebx], al								;set the pixel
		inc ebx												;increment the counter
		loop gfx_dl_loop									;loop CX (width) times
															;
	pop ebx													;restore the registers
	pop ecx													;
	pop fs													;
	pop es													;
	ret														;return from subroutine
	
gfx_fill_rect:												;fills a rectangle on the screen
															;input: AL = color
															;       EBX = top-left corner coordinate
															;       ECX = (height SHL 16) OR width
															;
	push ebx												;save the registers
	push dx													;
															;
	xor dx, dx												;clear DX
	gfx_dr_loop:											;
		call gfx_draw_line_hor								;draw a line
		add ebx, scrn_w										;go to the next line
		inc dx												;increment DX
		push ecx											;save ECX
		shr ecx, 16											;expose the upper 16 bits of ECX in CX
		cmp dx, cx											;compare DX with the upper 16 bits of ECX (the current Y with the target Y)
		pop ecx												;restore ECX
		jb gfx_dr_loop										;loop when needed
															;
	pop dx													;restore the registers
	pop ebx													;
	ret														;return from subroutine
	
gfx_draw_str:												;graphically prints a string using the beforehand loaded font
															;input: AL = color
															;       ESI = the string to print
															;       EBX = coordinate
															;
	push cx													;save the registers
	push esi												;
															;
	gfx_ps_loop:											;
		mov cl, [ds:esi]									;load the next char
		cmp cl, 0											;check if the end has been reached
		je gfx_ps_ret										;exit if so
		call gfx_print_char									;else, print the char
		inc esi												;increment the pointer
		jmp gfx_ps_loop										;looping
	gfx_ps_ret:												;
															;
	pop esi													;restore the registers
	pop cx													;
															;
	ret														;return from subroutine
	
gfx_print_char:												;graphically prints a char
															;input: AL = color
															;       CL = char
															;       EBX = coordinate
															;
	push dx													;save the registers
	push esi												;
	push cx													;
	push ax													;
															;
	mov al, cl												;load the char into AL
	dec al													;decrement the char, beacuse the font is offset by a little
	xor ah, ah												;clear AH
	mov dl, font_neutral_width								;set DL to the font width
	mul dl													;multiply AX by DL (char no. by font width)
	xor esi, esi											;move AX to the pointer
	mov si, ax												;
	xor dl, dl												;clear DL
	pop ax													;restore AX
	xor ch, ch												;clear CH
	gfx_pc_loop:											;
		mov cl, [ds:esi+font_neutral]						;load the next char column
		call gfx_put_char_col								;print it
		inc esi												;increment the pointer
		inc ebx												;increment the coordinate
		inc ch												;increment the counter
		cmp ch, font_neutral_width							;check if the end has been reached
		jb gfx_pc_loop										;continue if not
	pop cx													;restore the registers (except for AX, it was already restored before)
	pop esi													;
	pop dx													;
	ret														;return from subroutine

gfx_put_char_col:											;puts a column from a character onto the screen
															;input: AL = color
															;       CL = data
															;       EBX = coordinate
															;
	push dx													;save the registers
	push cx													;
	push es													;
	push ebx												;
	push fs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	push word [fs:gqv_ptr_gfx_dbuf]							;load dbuf selector into ES
	pop word es												;
															;
	xor dx, dx												;clear DX
	gfx_pcc_loop:											;
		test cl, 1											;check if the lowest bit is set
		jz gfx_pcc_loop_cont								;skip the pixel writing if it isn't
		mov [es:ebx], al									;write the pixel
		gfx_pcc_loop_cont:									;
		shr cl, 1											;shift it one bit to the right
		add ebx, scrn_w										;add one line to the video buffer offset
		inc dx												;increment the counter
		cmp dx, 8											;check if the end has been reached
		jb gfx_pcc_loop										;continue looping if not
															;
	pop fs													;restore the registers
	pop ebx													;
	pop es													;
	pop cx													;
	pop dx													;
															;
	ret														;return from subroutine

gfx_put_32bit_hex:											;prints a 32-bit hexadecimal value
															;input: AL = color
															;       EDX = value
															;       EBX = coordinate
															;
	push edx												;save the registers
	push ax													;
	push cx													;
															;
	bswap edx												;convert EDX to little endian for it to be displayed as big endian
	xor ah, ah												;clear AX
	gfx_p32bh_cycle:										;
		xchg dx, bx											;exchange BX and DX as only the first one can be used as an indexing register
		push bx												;save BX again
		and bx, 0x00FF										;only get the lower byte
		and bl, 0xF0										;only get the higher 4 bits of BL
		shr bl, 4											;
		mov byte cl, [ds:gfx_num_hex_const+bx]				;fetch the character
		pop bx												;restore BX
		xchg dx, bx											;swap BX and DX, returning them to their original state
		call gfx_print_char									;print the character
		xchg dx, bx											;repeat this process once again
		push bx												;
		and bx, 0x00FF										;
		and bl, 0x0F										;but this time, getting the lower 4 bits of DL
		mov byte cl, [ds:gfx_num_hex_const+bx]				;
		pop bx												;
		xchg dx, bx											;
		call gfx_print_char									;print the character
		shr edx, 8											;fetch the next byte
		inc ah												;increment the counter
		cmp ah, 4											;check if all four bytes were printed
		jne gfx_p32bh_cycle									;continue if not
															;
	pop cx													;restore the registers
	pop ax													;
	pop edx													;
															;
	ret														;return from subroutine

gfx_put_16bit_hex:											;prints a 16-bit hexadecimal value
															;input: AL = color
															;       DX = value
															;       EBX = coordinate
															;
	push dx													;save the registers
	push ax													;
	push cx													;
															;
	xchg dl, dh												;convert DX to little endian for it to be displayed as big endian
	xor ah, ah												;clear AX
	gfx_p16bh_cycle:										;
		xchg dx, bx											;exchange BX and DX as only the first one can be used as an indexing register
		push bx												;save BX again
		and bx, 0x00FF										;only get the lower byte
		and bl, 0xF0										;only get the higher 4 bits of BL
		shr bl, 4											;
		mov byte cl, [ds:gfx_num_hex_const+bx]				;fetch the character
		pop bx												;restore BX
		xchg dx, bx											;swap BX and DX, returning them to their original state
		call gfx_print_char									;print the character
		xchg dx, bx											;repeat this process once again
		push bx												;
		and bx, 0x00FF										;
		and bl, 0x0F										;but this time, getting the lower 4 bits of DL
		mov byte cl, [ds:gfx_num_hex_const+bx]				;
		pop bx												;
		xchg dx, bx											;
		call gfx_print_char									;print the character
		shr dx, 8											;fetch the next byte
		inc ah												;increment the counter
		cmp ah, 2											;check if all two bytes were printed
		jne gfx_p16bh_cycle									;continue if not
															;
	pop cx													;restore the registers
	pop ax													;
	pop dx													;
															;
	ret														;return from subroutine

gfx_num_hex_const: db "0123456789ABCDEF"
