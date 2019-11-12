;Neutron project
;nFS driver

nfs_loaded_sector_no: dd 0xFFFFFFFF
nfs_drive_no: db 0xFF
nfs_partition_no: db 0xFF
nfs_status: db 0xFF

nfs_part_name: db '               ', 0
nfs_cluster_size: db 0xFF
nfs_mft_size: db 0xFF

nfs_read_drive_sector:				;reads a sector from the drive
									;input: EAX = LBA of the sector
									;       BL = 1 to force read
									;              otherwise caching is used
									;output: nfs_status = 0 on success
									;                     1 on disk I/O err
									;        nfs_buffer contains valid data only if 
									;                     nfs_status is 0
									;
	nop
	nop
	nop
	nop
	push ax							;save all registers
	push bx							;
	push cx							;
	push dx							;
	push es							;
									;
	cmp eax, [ds:nfs_loaded_sector_no] ;check if this sector is already in cache
	je nfs_read_sector_return		;return if it is
	push ds							;set the load segment to the data segment
	pop es							;>>
	mov bx, nfs_buffer				;set the target location: a buffer
									;
	mov dl, [ds:nfs_drive_no]		;set the drive number to load from
	xor dh, dh						;TODO:
	xor ch, ch						;LBA to CHS conversion
	mov cl, al						;set the number of the sector to read from
	inc cl							;numbering starts from 1...
	mov al, 1						;set the amount of sectors to be read: 1
	mov ah, 2						;BIOS routine no.: disk read
	int 0x13						;execute the BIOS routine
	cmp ah, 0						;check if the operation was successful
	jne nfs_rs_io_err				;I/O error handler
	mov byte [ds:nfs_status], 0		;set the success flag
	jmp nfs_read_sector_return		;jump straight to the return handler if the operation was successful
nfs_rs_io_err:
	mov byte [ds:nfs_status], 1		;set the disk I/O err flag
	jmp nfs_read_sector_return		;jump to the return handler
nfs_read_sector_return:
	mov [ds:nfs_loaded_sector_no], eax ;save the number of the sector that is loaded
	pop es							;restore all registers
	pop dx							;
	pop cx							;
	pop bx							;
	pop ax							;
	ret								;return from subroutine
	
nfs_check_drive_fs:					;checks if a drive has valid nFS signature
									;input: none
									;output: nfs_status = 0 if drive has the correct signature
									;                     1 on disk I/O err
									;                     2 otherwise
									;
	push ax							;save AX
									;
	mov ax, 5						;set the sector to be read: nFS master sector
	call nfs_read_drive_sector		;read it
	cmp byte [ds:nfs_status], 0		;check if the read was successful
	jne nfs_cdfs_return				;return from subroutine preserving the status code if not
	cmp dword [ds:nfs_buffer], 0xDEADF500 ;check the signature
	je nfs_cdfs_return				;return from subroutine if the signature is correct
	mov byte [ds:nfs_status], 2		;set the non-valid signature flag otherwise
	jmp nfs_cdfs_return				;return from subroutine
nfs_cdfs_return:
	pop ax							;restore EAX
	ret								;return from subroutine
	
nfs_load_master_sector:				;loads and parses the master sector info
									;input: nothing
									;output: nfs_status = 0 on success
									;                     1 on disk I/O err
									;                     2 on an incorrect signature
									;
	push ax							;save all registers
	push bx							;
	xor bx, bx						;clear BX
	call nfs_check_drive_fs			;check the signature
									;this has an added advantage of the sector already being buffered
	cmp byte [ds:nfs_status], 0		;check the success
	jne nfs_lms_return				;return from subroutine preserving the status code if an error occured
nfs_lms_copy_loop:
	add bx, 4						;add the offset for the label
	mov byte al, [ds:nfs_buffer+bx] ;intermediately load a byte
	sub bx, 4						;subtract it to return to the normal state
	mov byte [ds:nfs_part_name+bx], al ;copy a byte
	inc bx							;increment pointer
	cmp bx, 19						;terminate the copying at 18 bytes
	je nfs_lms_return				;>>
	jmp nfs_lms_copy_loop			;loop should be a loop
nfs_lms_return:
	pop bx							;restore all registers
	pop ax							;
	ret								;return from subroutine
	
nfs_load_master_filetable:			;loads the master file table
									;input: nothing
									;output: nfs_status = 0 on success
									;					  1 on disk I/O err
									;
	push ax							;save AX
	mov ax, 6						;set the sector to be read: 6th
	call nfs_read_drive_sector		;read the sector
	pop ax							;restore AX
	ret								;return from subroutine
	
nfs_find:							;find a file on the disk
									;input: GS:AX = the target file name
									;output: nfs_status = 0 on success
									;                     1 on disk I/O err
									;                     2 if file wasn't found
									;        DS:DX contains the found entry (only valid if nfs_status = 0)
	push ax							;save AX
	push cx							;save CX
	push bx							;save BX
	call nfs_load_master_filetable	;load the master file table
	cmp byte [ds:nfs_status], 0		;check if operation was successful
	jne nfs_find_ret				;return preserving the status if not
	xor bx, bx						;clear BX
nfs_find_search_loop:
	mov dl, [ds:nfs_buffer+bx]		;load one byte from memory
	cmp dl, 0						;check if we've reached the end of the file list
	je nfs_find_error_ret			;return from subroutine if so
	mov dx, bx						;set the string to be compaed: candidate file name
	add dx, nfs_buffer				;also add the buffer offset to the total one
	pop cx							;restore screen pos
	xchg bx, cx						;swap CX and BX
	push ds							;move
	pop fs							;FS <- DS
	push ax							;save AX
	call str_cmp					;compare the string at GS:AX (input) and FS:DX (candidate)
	cmp al, 1						;if they're equal
	pop ax							;restore AX
	je nfs_find_found_ret			;jump to the found routine
	xchg bx, cx						;swap CX and BX
	push cx							;save screen pos
	add bx, 32						;increment the pointer
	jmp nfs_find_search_loop		;looping
nfs_find_ret:
	pop bx							;restore BX
	pop cx							;restore CX
	pop ax							;restore AX
	ret								;return from subroutine
nfs_find_error_ret:
	mov byte [ds:nfs_status], 2		;set the status code 2
	jmp nfs_find_ret				;return normally
nfs_find_found_ret:
	push bx
	mov byte [ds:nfs_status], 0		;set the status code 0
	jmp nfs_find_ret				;return normally