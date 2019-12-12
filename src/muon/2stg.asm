;Neutron project
;Muon - second stage loader

%define nfs_buffer   0x800
%define kbd_buf      0x105
%define app_segment  0x175

org 0x0000
use16

dw 0xEBA1					;signature for the 1st stage loader

entry:
	cli						;disable interrupts
	mov al, 0x80			;disable non-maskable interrupts
	out 0x70, al			;
	push ds					;save DS
	push 0					;move 0
	pop ds					;to DS
	mov word [ds:0x8A], cs  ;set the 22h'th segment in the IVT
	mov word [ds:0x88], stg2_func_intr  ;       offset
	pop ds					;restore DS
	mov ax, cs				;
	mov ds, ax				;set DS to CS
	mov byte al, [ds:0]		;fetch the drive number
	mov byte [ds:nfs_drive_no], al ;tell it to the nFS driver
	mov bx, 320				;set the screen position: 2nd line counting from 0
	mov ch, 0x0F			;set the colors: white on black
	call nfs_init			;read boot drive geometry
	call nfs_load_master_sector ;load the nFS data
	mov ch, 0x0F			;set the colors: white on black
	jmp cli_krnl			;try to load the kernel
cli_loop:
	mov ch, 0x87			;set the colors: blinking gray on black
	mov dx, cli_prefix_literal ;set the string to be printed
	call print_str			;print the string
	mov ch, 0x08			;set the colors: white on black
	push kbd_buf			;
	pop fs					;set keyboard buf segment
	mov dx, bx				;save BX (screen pos)
	xor bx, bx				;clear BX
read_loop:
	xor ah, ah				;set BIOS function: kbd. read
	int 0x16				;call the BIOS routine
	cmp ah, 0x1C			;check if enter has been pressed
	je cli_done_reading		;invoke the parser in this case
	cmp ah, 0x0E			;check if backspace has been pressed
	jne no_bkspc			;continue execution if not
	dec bx					;decrement the buffer pointer
	xchg bx, dx				;restore screen pos
	sub bx, 2				;decrement the screen pos
	mov cl, ' '				;print an empty char
	call print_char			;actually print it
	sub bx, 2				;decrement the screen pos
	xchg bx, dx				;save screen pos
	jmp read_loop			;go back
	no_bkspc:
	mov [fs:bx], al			;copy the key code to RAM
	mov cl, al				;set the char to be printed
	inc bx					;increment the pointer
	xchg bx, dx				;restore screen pos
	mov ch, 0x0F			;set the colors: white on black
	call print_char			;print the char
	xchg bx, dx				;restore buf pos
	;call cursor_set		;set the cursor position
	jmp read_loop			;it's a loop...
cli_done_reading:
	mov byte [fs:bx], 0		;save the trailing zero
	xchg bx, dx				;swap screen pos and kbd buf pos
	push kbd_buf			;set the 2nd string segment
	pop gs					;
	mov ax, 0				;set the string to which the first will be compared: input buffer
	push ds					;set the 1st string segment
	pop fs					;
	push cs
	pop ds
	mov dx, command_reboot_literal ;compare with reboot command literal
	call str_cmp			;
	cmp al, 1				;check if if 'reboot' has been typed in
	je cli_reboot			;execute the command if so
	mov dx, command_halt_literal ;compare with halt command literal
	call str_cmp			;
	cmp al, 1				;check if if 'halt' has been typed in
	je cli_halt				;execute the command if so
	mov dx, command_ls_literal ;compare with ls command literal
	call str_cmp			;
	cmp al, 1				;check if if 'ls' has been typed in
	je cli_ls_call			;execute the command if so
	mov dx, command_view_literal ;compare with view command literal
	call str_cmp_wspc		;
	cmp al, 1				;check if if 'view' has been typed in
	je cli_view_call		;execute the command if so
;	mov dx, command_gfx_literal ;compare with gfx command literal
;	call str_cmp			;
;	cmp al, 1				;check if 'gfx' has been typed in
;	je cli_gfx				;execute the command if so
	mov dx, command_run_literal ;compare with run command literal
	call str_cmp_wspc		;
	cmp al, 1				;check if if 'run' has been typed in
	je cli_run				;execute the command if so
	mov dx, command_krnl_literal ;compare with krnl command literal
	call str_cmp_wspc		;
	cmp al, 1				;check if if 'krnl' has been typed in
	je cli_krnl				;execute the command if so
	call print_unknown_cmd	;print an error if an unknown command has been passed
	jmp cli_loop			;it's a loop...
print_unknown_cmd:
	call print_ln			;go to a new line
	mov ch, 0x0C			;set the colors: bright red on black
	mov dx, unknown_cmd_literal	;set the string to be printed
	call print_str			;print the string
	push ds					;save the DS
	push kbd_buf			;
	pop ds					;set the new DS value
	mov dx, 0				;ptr to the input str
	call print_str			;print the string
	pop ds					;restore the DS
	mov dx, unknown_cmd_end_literal	;set the string to be printed
	call print_str_line		;print the string
	ret						;return from subroutine
cli_reboot:
	call reboot_cmd			;try to reboot
	call print_ln			;go to a new line
	mov ch, 0x0C			;set the colors: bright red on black
	mov dx, command_reboot_failed_literal ;set the string to be printed
	call print_str			;print the string
	jmp $					;hang if we didn't reboot
cli_halt:
	hlt						;halt execution
cli_ls_call:
	call print_ln			;go to the new line
	mov ch, 0x0F			;set the colors: white on black
	mov dx, command_ls_info_literal ;set the string to be printed
	call print_str			;print it
	mov dx, nfs_part_name   ;set the string to be printed: partition label
	call print_str_line		;print it
	call nfs_load_master_filetable ;load the master file table
	push ax					;save AX
	push dx					;save DX
	push bx					;save BX
	xor bx, bx				;clear BX
cli_ls_loop:
	mov dl, [ds:nfs_buffer+bx] ;load one byte from memory
	cmp dl, 0				;check if we've reached the end of the file list
	je cli_ls_ret			;return from subroutine if so
	mov dx, bx				;set the string to be printed: file name
	add dx, nfs_buffer		;also add the buffer offset to the total one
	pop ax					;restore screen pos
	xchg bx, ax				;swap AX and BX
	mov ch, 0x0F			;set the colors: white on black
	call print_str_line		;print the filename if not
	xchg bx, ax				;swap AX and BX
	push ax					;save screen pos
	add bx, 32				;increment the pointer
	jmp cli_ls_loop			;looping
cli_ls_ret:
	pop bx					;restore BX
	pop dx					;restore DX
	pop ax					;restore AX
	jmp cli_loop			;return from subroutine
cli_view_call:
	call print_ln			;go to the new line
	call nfs_load_master_filetable ;load the master file table
	push ax					;save AX
	push cx					;save CX
	push dx					;save DX
	push bx					;save BX
	xor bx, bx				;clear BX
	mov ax, 5				;set AX to 5, so the string at GS:AX now contains the file name we're looking for
	cmp byte [ds:nfs_status], 0	;check if the operation was successful
	jne cli_view_ret_error	;print an error if not
cli_view_search_loop:
	mov dl, [ds:nfs_buffer+bx] ;load one byte from memory
	cmp dl, 0				;check if we've reached the end of the file list
	je cli_view_ret_error	;return from subroutine if so
	mov dx, bx				;set the string to be compaed: candidate file name
	add dx, nfs_buffer		;also add the buffer offset to the total one
	pop cx					;restore screen pos
	xchg bx, cx				;swap CX and BX
	push ds					;move
	pop fs					;FS <- DS
	mov ax, 5				;set AX to 5, so the string at GS:AX now contains the file name we're looking for
	call str_cmp			;compare the string at GS:AX (input) and FS:DX (candidate)
	cmp al, 1				;if they're equal
	je cli_view_found		;jump to the print routine
	xchg bx, cx				;swap CX and BX
	push cx					;save screen pos
	add bx, 32				;increment the pointer
	jmp cli_view_search_loop ;looping
cli_view_found:
	mov ch, 0x0F			;set the colors: white on black
	add dx, 24				;add 24 to the DX so DS:DX now points to the file size
	xchg bx, dx				;exchange BX with the indexing register
	mov cx, [ds:bx]			;load the file size into CX
	mov ax, [ds:bx+4]		;load the file sector into AX
	xchg bx, dx				;exchange BX with the indexing register
	call nfs_read_drive_sector ;read the file sector
	mov dx, nfs_buffer		;point to the buffer
cli_view_loop:
	push cx					;store CX
	xchg bx, dx				;exchange BX with the indexing register
	mov cl, [ds:bx]			;load the next byte
	push cx					;save CX
	xchg bx, dx				;exchange BX with the indexing register
	mov ch, 0x07			;set the colors: gray on black
	call print_hex			;print it
	mov cl, ' '				;print a whitespace
	call print_char			;
	pop cx					;restore CX
	mov ch, 0x0F			;set the colors: white on black
	call print_char			;print the original char
	mov cl, ' '				;print a whitespace
	call print_char			;again
	inc dx					;increment the pointer
	sub dx, nfs_buffer		;get the bare offset
	cmp dx, 511				;check if the pointer is out of bounds
	ja cli_view_load_sect	;load the next sector if so
	add dx, nfs_buffer		;return to the pointer
	cli_view_load_sect_ret:
	pop cx					;restore CX
	loop cli_view_loop		;looping...
	call print_ln			;go to the new line
	push bx					;save screen pos
	jmp cli_view_ret		;return to the CLI
cli_view_load_sect:
	inc ax					;increment the sector no
	call nfs_read_drive_sector ;read the file sector
	mov dx, nfs_buffer
	jmp cli_view_load_sect_ret ;return
cli_view_ret_error:
	pop bx					;restore screen pos
	mov ch, 0x0C			;set the text color - red
	mov dx, command_view_error_fnf_literal ;set the string to be printed
	call print_str_line		;print it
	push bx					;save screen pos again
	jmp cli_view_ret		;return
cli_view_ret:
	pop bx					;restore BX
	pop dx					;restore DX
	pop cx					;restore CX
	pop ax					;restore AX
	jmp cli_loop			;return to the command line
cli_run:
	call print_ln			;go to a new line
	push 0x0400				;load 0x0400 (keyboard buffer segment)
	pop gs					;into GS
	mov ax, 4				;the file name starts from the 4th char
	call nfs_find			;find the file on the disk
	cmp byte [ds:nfs_status], 0	;check if the operation was successful
	jne cli_run_ret_error	;return printing the error if not
	mov si, dx				;load the nFS entry address into SI
	mov ax, [ds:si+28]		;load its location into AX
	mov cx, [ds:si+24]		;and its size into CX
	call nfs_read_drive_sector ;read the sector with the application
	cmp dword [ds:nfs_buffer], 0x4558456e ;check the signature
	jne cli_run_ret_error	;return if it isn't valid
	push es					;save ES
	push di					;save DI
	push si					;save SI
	push app_segment		;move app segment
	pop es					;into ES
	mov si, nfs_buffer		;source: nFS buffer + executable offset
	mov di, 0				;destination: offset 0
	rep movsb				;copy the application executable 
	push ds					;save DS
	push app_segment		;move app segment
	pop ds					;into DS
	call app_segment:0x0007	;call the loaded application
	pop ds					;restore DS
	pop si					;restore SI
	pop di					;restore DI
	pop es					;restore ES
	call print_ln			;go to a new line
	jmp cli_loop			;return to the command line
cli_run_ret_error:
	mov ch, 0x0C			;set the text color - red
	mov dx, command_run_error_literal ;set the string to be printed
	call print_str_line		;print it
	jmp cli_loop			;return to the command line
cli_krnl:
	call print_ln			;go to a new line
	call krnl_load			;load the kernel
	cmp ax, 0				;check if the load was successful
	jne cli_krnl_ret		;return if not
	push 0x93C0				;segment 0x93C0 - this is where the map is stored
	pop es					;
	xor di, di				;clear DI
	call map_memory			;map the memory while we didn't overwrite the IVT
	jmp krnl_run			;run the kernel!
cli_krnl_ret:
	mov ch, 0x0C
	mov dx, command_krnl_error
	call print_str_line
	jmp cli_loop
krnl_load:
	;load the kernel into RAM
	push gs					;save the registers
	push es					;
	push si					;
	push di					;
	push ax					;
	push dx					;
	push ecx					;
	push bx					;
	mov ax, ds				;move DS
	mov gs, ax				;into GS
	mov ax, command_krnl_file ;find the kernel file
	call nfs_find			;
	cmp byte [ds:nfs_status], 0 ;check if the operation was successful
	jne krnl_load_ret_err	;return immediately if not
	mov bx, dx				;load DX into the indexing register
	mov ax, [ds:bx+28]		;load the file location into AX
	mov dx, [ds:bx+24]		;load the file size into DX
	push 0xF0				;load 0xF0 into
	pop es					;ES
	xor di, di				;clear DI
krnl_load_cycle:
	cmp dx, 0				;check if the entire file was read
	jle krnl_load_ret		;return if so
	pop bx
	push dx
	push cx
	mov dx, command_krnl_debug
	mov ch, 0x0F
	call print_str
	pop cx
	pop dx
	push bx
	call nfs_read_drive_sector ;read one sector
	cmp byte [ds:nfs_status], 0 ;check if the operation was successful
	jne krnl_load_ret_err	;return immediately if not
	mov ecx, 512			;set the number of bytes to copy: 512
	mov si, nfs_buffer		;set SI to the nFS buffer
	rep movsb				;copy the bytes
	inc ax					;increment the sector number
	sub dx, 512				;subtract the sector size from the file size
	jmp krnl_load_cycle		;looping
krnl_load_ret_err:
	pop bx					;restore the registers
	pop ecx					;
	pop dx					;
	pop ax					;
	pop di					;
	pop si					;
	pop es					;
	pop gs					;
	mov ax, 1				;tell that the error occured
	ret						;return from subroutine
krnl_load_ret:
	pop bx					;restore the registers
	pop ecx					;
	pop dx					;
	pop ax					;
	pop di					;
	pop si					;
	pop es					;
	pop gs					;
	mov ax, 0				;tell that the load was successfull
	ret						;return from subroutine
krnl_run:
	;shit is getting unreal here
	;i mean, switch the CPU into the protected mode
	;(opposite of what we've had before up until now, the real mode)
	mov ch, 0x0F
	mov dx, command_krnl_debug_run
	call print_str
	cli						;disable interrupts
	mov al, 0x80			;disable non-maskable interrupts
	out 0x70, al			;
	push 0x050				;load 0x050
	pop ds					;into the data segment
	mov ecx, gdt_end - gdt	;copy the entire GDT
	mov si, gdt				;source: predefined GDT
	push 0					;destination: start of the memory
	pop es					;
	xor di, di				;
	rep movsb				;copy
	mov ecx, 6				;copy the entire GDT descriptor
	mov si, gdt_desc		;source: predefined GDT descriptor
	xor di, di				;destination: start of the memory
	rep movsb				;copy
	xor dx, dx				;set DS to 0
	mov ds, dx				;
	lgdt [0]				;Load Global Descriptor Table
	call gfx_go_best		;exit the text mode, as the kernel can't do it on its own
	push 0x8FC0				;load 0x8C0
	pop es					;into ES
	mov [es:0], ecx			;save ECX (gfx buf linear address) in RAM
	mov [es:4], edx			;save EDX (gfx resolution) in RAM
	mov dl, [ds:nfs_drive_no] ;get the drive number
	mov byte [es:8], dl		;save boot drive in RAM
	mov eax, cr0			;load the control register 0 into EAX
	or eax, 1				;set its last bit
	mov cr0, eax			;load the EAX the control register 0
	;YAAY! we're in Protected Mode now!
	cli						;disable interrupts
	mov ax, 0x10			;load 0x10
	mov ds, ax				;into DS
	mov ss, ax				;and SS
	mov es, ax				;and ES
	mov fs, ax				;and FS
	mov gs, ax				;and GS
	mov esp, 0x4FFFFF		;set the top-of-stack pointer
	pushf					;set IOPL to 3
	pop ax					;
	or ax, 3 << 12			;
	push ax					;
	popf					;
	jmp 8:0xF00				;far jump into the beforehand-loaded kernel
	
stg2_func_intr:				;is executed whenever there was a software IRQ from the running application
	cmp ah, 0				;AH = 0
	je stg2_func_print		;print a string
stg2_func_intr_ret:			;
	iret					;return from interrupt
stg2_func_print:
	call print_str			;print the string
	jmp stg2_func_intr_ret  ;return
	
%include "nfs_driver.asm"	;include the nFS driver
%include "gfx_driver.asm"	;include the graphics driver
%include "stdlib.asm"		;include the "standard library"
	
cli_prefix_literal: db 'nCLI>', 0
unknown_cmd_literal: db 'Error: unknown command "', 0
unknown_cmd_end_literal: db '"', 0

command_reboot_literal: db 'reboot', 0
command_reboot_failed_literal: db 'Error: failed to reboot. Hanging.', 0

command_halt_literal: db 'halt', 0

command_ls_literal: db 'ls', 0
command_ls_info_literal: db 'Listing root of: ', 0

command_view_literal: db 'view', 0
command_view_error_fnf_literal: db 'Error: file not found', 0

command_run_literal: db 'run', 0
command_run_error_literal: db "Error: the file either doesn't exist, or it isn't Muon-2-executable", 0

command_krnl_literal: db 'krnl', 0
command_krnl_file: db 'quark', 0
command_krnl_error: db "An error occured while loading Quark. Try the 'krnl' command to try to load it again. A reboot may help too", 0
command_krnl_debug: db "SL ", 0
command_krnl_debug_run: db "KEXEC", 0

font_file: db 'neutral.nfnt', 0
num_hex_const: db '0123456789ABCDEF'

gdt:                    ;Global Descriptor Table

gdt_null:               ;null segment
	dd 0
    dd 0
gdt_code:               ;GDT code segment
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0
gdt_data:               ;GDT data segment
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:
gdt_desc:                   ;GDT descriptor
    dw gdt_end - gdt - 1
    dd 0					;we store GDT at the start of the memory

reboot_cmd:
	db 0xEA
	dw 0x0000, 0xFFFF
	ret
