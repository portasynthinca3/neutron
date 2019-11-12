;Neutron Project
;GUI library
;GFX library required

%define gqv_ptr_gui_cuc 6									;GQV-pointer: cursor coordinate
%define gqv_siz_gui_cuc 4									;GQV-size:    >>

db "GUI"

gui_full_redraw:											;completely redraws the entire screen
															;input: none
															;output: none
															;
	push ebx												;save the registers
	push ecx												;
	push eax												;
	push fs													;
	push edx												;
															;
	mov al, 0x13											;clear the screen with dark gray
	call gfx_fill											;
	mov al, 0x48											;draw the green-ish top bar
	xor ebx, ebx											;in the top-left corner
	mov ecx, (16 << 16) | scrn_w							;with the height of 16 pixels and width the same as the display
	call gfx_fill_rect										;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	mov dword ebx, [fs:gqv_ptr_gui_cuc]						;load cursor position into EBX
	mov al, 0x27											;cursor color - bright red
	call gfx_draw_cur										;draw the cursor
															;
	pop edx													;restore the registers
	pop fs													;
	pop eax													;
	pop ecx													;
	pop ebx													;
	ret														;return from subroutine

gui_update_kbdms:											;update the keyboard and mouse
															;input: none
															;output: none
															;comment: TODO: keyboard input
															;
	push ebx												;save the registers
	push eax												;
	push ecx												;
	push edx												;
	push fs													;
															;
	push word [gqv]											;load GQV selector into FS
	pop word fs												;
	mov dword ebx, [fs:gqv_ptr_gui_cuc]						;load cursor position into EBX
															;
	xor eax, eax											;clear EAX
	mov ecx, 3												;repeat 3 times
	gui_update_kbdms_cycle:									;
		call kbdms_wait										;wait for a data byte from keyboard or mouse
		cmp al, 1											;check if it's a mouse data byte
		jne gui_update_kbdms_cycle							;wait again if it's not
		in al, 0x60											;input one byte
		shl eax, 8											;shift EAX to the left
		loop gui_update_kbdms_cycle							;loop
	shr eax, 8												;shift EAX to the right as it's a 3-byte packet
															;
	push eax												;save EAX
	shr eax, 16												;expose the status byte in AL
	mov cl, al												;save it in CL
	pop eax													;restore EAX
	mov dl, al												;store Y movement in DL
	mov al, ah												;store X movement in AL
	and edx, 0x000000FF										;mask the higher bits of X movement out
	and eax, 0x000000FF										;mask the higher bits of Y movement out
	test cl, 1 << 4											;check bit 4 in status byte (X movememnt sign flag)
	jz gui_ukm_skip_xsx										;skip X movement sign extension if it isn't set
	cbw														;sign extend the X movement
	gui_ukm_skip_xsx:										;
	test cl, 1 << 5											;check bit 5 in status byte (Y movememnt sign flag)
	jz gui_ukm_skip_ysx										;skip Y movement sign extension if it isn't set
	xchg dx, ax												;swap DX and AX
	cbw														;sign extend the Y movement
	xchg dx, ax												;swap DX and AX
	gui_ukm_skip_ysx:										;
	xchg eax, edx											;swap EAX and EDX
															;
	test dx, dx												;test the sign of DX
	jns guk_add_edx											;add EDX if the number is positive
	neg dx													;change the sign of EDX and subtract it otherwise
	sub ebx, edx											;
	jmp guk_cont											;
	guk_add_edx:											;
	add ebx, edx											;add X movement to the total coordinate
	guk_cont:												;
	mov dx, scrn_w											;multiply AX by screen width and store the result in DX:AX
	neg eax													;change the sign of EAX
	imul dx													;
	shl edx, 16												;shift the upper bits of the result up
	mov dx, ax												;load the lower bits
	add ebx, edx											;add Y movement multiplied by the screen width to the total coordinate
	cmp ebx, 0												;compare EBX with zero
	jge guk_skip_zero										;we need to clear it if it is less than a zero
	xor ebx, ebx											;clear
	guk_skip_zero:											;
	cmp ebx, scrn_w*scrn_h									;compare EBX with screen size
	jl guk_skip_max											;we need to set it to the maximum possible value if it is out of bounds
	mov ebx, scrn_w*scrn_h-1								;set
	guk_skip_max:											;
															;
	mov dword [fs:gqv_ptr_gui_cuc], ebx						;save cursor position
															;
	pop fs													;restore the registers
	pop edx													;
	pop ecx													;
	pop eax													;
	pop ebx													;
															;
	ret														;return from subroutine
kbdms_wait:													;
	in al, 0x64												;input a byte from port 0x64
	test al, 1												;check if its 0th bit (byte available flag) is set
	jz kbdms_wait											;loop if not
	test al, 1 << 5											;check if its 5th bit (mouse data flag) is set
	jz gui_ukm_sal_kbd										;set AL to 0 if the keyboard event occured
	mov al, 1												;set AL to 1 otherwise
	jmp gui_ukm_ret											;return
	gui_ukm_sal_kbd:										;the label to jump to
	xor al, al												;set AL to 0
	gui_ukm_ret:											;
	ret														;return from subroutine

kbdms_wait_clr:												;wait for the keyboard/mouse to be ready to accept input
															;input: none
															;output: none
															;
	push ax													;save the registers
															;
	in al, 0x64												;input a byte from port 0x64
	test al, 2												;check if its 1th bit (busy flag) is set
	jnz kbdms_wait_clr										;loop if it is
															;
	pop ax													;restore the registers
															;
	ret														;return from subroutine

gui_mouse_init:												;initialize the mouse
															;input: none
															;output: none
															;
	push eax												;save the registers
															;
	pushfd													;push the EFLAGS register onto stack
	pop eax													;pop it into EAX
	and eax, ~((1 << 13) | (1 << 12))						;clear bits 12 and 13
	push eax												;push EAX onto stack
	popfd													;pop it into the EFLAGS register
															;
	call kbdms_wait_clr										;wait for the controller to be ready to accept input
	mov al, 0xA8											;output 0xA8 to port 0x64
	out 0x64, al											;
	mov al, 0xA8											;output 0xAD to port 0x64
	out 0x64, al											;
	call kbdms_wait_clr										;wait for the controller to be ready to accept input
	mov al, 0xD4											;output 0xD4 to port 0x64
	out 0x64, al											;
	call kbdms_wait_clr										;wait for the controller to be ready to accept input
	mov al, 0xF4											;output command 0xF4 - enable packet streaming - to port 0x60
	out 0x60, al											;
	call kbdms_wait											;wait for the mouse to send an ACK byte
	in al, 0x60												;read and discard it
															;
	pop eax													;restore the registers
															;
	ret														;return from subroutine