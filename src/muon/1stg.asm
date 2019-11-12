;Neutron project
;Muon - first stage loader

org 0x7C00
use16

entry:
	mov ax, 0x0300
	mov ss, ax
	xor ax, ax
	push dx					;save the boot drive number
	xor ch, ch				;clear CH
lp:
	xor ax, ax				;set the segment registers
	mov ds, ax				;data segment
	mov es, ax				;E data segment
	mov fs, ax				;F data segment
	mov gs, ax				;G data segment
	call screen_clear		;clear the screen
	xor bx, bx				;clear BX
	mov ch, 0x0F			;set the colors: white on black
	mov dx, startup_literal	;set the string to be printed
	call print_str			;print the string
	mov ax, 0x50			;
	mov es, ax				;set ES to the load ptr
	xor bx, bx				;clear BX
	mov al, 4				;4 sectors to read
	pop dx					;restore drive number
	xor dh, dh				;head 0
	xor ch, ch				;cylinder 0
	mov cl, 2				;sector 2
	mov ah, 2				;BIOS routine no.
	int 0x13				;BIOS call
	cmp ah, 0				;compare return code with success
	jne print_io_err		;print io err if not successful
	xor bx, bx				;clear BX
	mov ax, [es:bx]			;read the signature
	cmp ax, 0xEBA1			;check the signature
	je run_2nd				;run the 2nd stage loader if successful
	mov bx, 160				;set the screen position: 1st line counting from 0
	mov ch, 0x0C			;set the colors: light red on black
	mov dx, error_literal	;set the string to be printed
	call print_str			;print the string
	mov dx, error_bad_signature_literal	;set the string to be printed
	call print_str			;print the string
	jmp $					;infinite loop
print_io_err:
	mov bx, 160				;set the screen position: 1st line counting from 0
	mov ch, 0x0C			;set the colors: light red on black
	mov dx, error_literal	;set the string to be printed
	call print_str			;print the string
	mov dx, error_diskio_literal ;set the string to be printed
	call print_str			;print the string
	jmp $					;infinite loop
run_2nd:
	push 0x050				;select the 2nd stage segment
	pop gs					;
	xor bx, bx				;and offset
	mov [gs:bx], dl			;tell the drive number to the 2nd stage loader
	mov bx, 160				;set the screen position: 1st line counting from 0
	mov ch, 0x0A			;set the colors: light green on black
	mov dx, successful_literal ;set the string to be printed
	call print_str			;print the string
	mov ax, 0x50			;
	jmp 0x050:0x0002		;jump to the 2stg code
	mov bx, 320				;set the screen position: 1st line counting from 0
	mov ch, 0x0C			;set the colors: light red on black
	mov dx, error_literal	;set the string to be printed
	call print_str			;print the string
	mov dx, error_noexec_literal ;set the string to be printed
	call print_str			;print the string
	jmp $
	
print_char:
	push ds					;save the DS
	mov ax, 0xB800			;
	mov ds, ax				;point to the VGA buffer
	mov [bx], cx			;move the data to the buffer
	pop ds					;restore the DS
	add bx, 2				;increment the pointer by 2
	ret						;return from subroutine
	
print_str:
	xchg bx, dx				;swap BX and DX
	mov cl, [bx]			;load the char
	xchg bx, dx				;swap BX and DX
	cmp cl, 0				;compare the char with 0
	je print_ret			;return if equal
	call print_char			;print the char
	inc dx					;increment the pointer
	jmp print_str			;it's a loop after all
print_ret:
	ret						;return from subroutine
	
screen_clear:
	xor bx, bx				;clear BX
	xor cx, cx				;clear CX
screen_clear_loop:
	call print_char			;print the char
	cmp bx, 3999			;check if we have reached the end of the screen
	jge screen_clear_exit	;exit if so
	jmp screen_clear_loop	;continue if not
screen_clear_exit:
	ret						;return from subroutine
	
startup_literal: db "Muon-1 has been started", 0
error_literal: db "Load error: ", 0
error_bad_signature_literal: db "bad signature", 0
error_diskio_literal: db "disk I/O", 0
error_noexec_literal: db "Muon-2 stage loader didn't execute", 0
successful_literal: db "Executing Muon-2", 0

times 446-($-$$) db 0		;fill-in

;MBR partition table
mbr_part_entry_1:
	mbr_pe1_status: db 0x80
	mbr_pe1_start_chs: db 0, 0, 0
	mbr_pe1_part_type: db 0x58
	mbr_pe1_end_chs: db 0, 0, 0
	mbr_pe1_start_lba: dd 5
	mbr_pe1_length_lba: dd 2876
mbr_part_entry_2: dd 0, 0, 0, 0
mbr_part_entry_3: dd 0, 0, 0, 0
mbr_part_entry_4: dd 0, 0, 0, 0

dw 0xAA55					;boot signature