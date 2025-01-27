
[BITS 16]  ; 32-������ ��� 
[ORG 0x7c00]; �����, �� �������� ����� �������� ���� ��� ����������� ���� 
start:
	mov ax, cs ; ���������� ������ �������� ���� � ax
	mov ds, ax ; ���������� ����� ������ ��� ������ �������� ������
	mov ss, ax ; � �������� �����
	mov sp, start ; ���������� ������ ����� ��� ����� ������ ���������� ���� ����. ���� ����� ����� ����� � �� ��������� ���.
	
	
	mov ah, 0x00
	mov al, 0x00
	int 0x10
	mov ah, 03h
	mov bh, 0x00
	int 0x10

 ; � ah ����� ������� BIOS: 0x0e - ����� ������� �� �������� ����� �������� (�������� ���������)

	mov  si, message1
	mov di, colors ; ��������� �� ������ ������
	cld                     ; ����������� ��� ��������� ������
	mov  bh, 0x00    ; �������� �����������
puts_loop1:
	mov ah, 0x09
	lodsb            ; ��������� ��������� ������ � al
	mov bl, [di]
	mov cx, 1
	test al, al      ; ������� ������ �������� ����� ������
	jz   puts_loop_exit1
	int  0x10        ; �������� ������� BIOS
	mov bl, dh
	mov ah, 03h      ;perestanovka kursora stb

	mov bh,0x00
	int 0x10
	mov ah, 0x02;/
	inc dl
	int 0x10	
	jmp  puts_loop1

puts_loop_exit1:
	cmp dh, 5
	je my_point
	
	mov ah, 03h     ;perestanovka kursora str++
	mov bh, 0x00
	int 0x10
	mov ah, 0x02;/
	mov bh, 0x00
	inc dh
	mov dl, 0x00
	int 0x10
			;proverka na printed stroki
	inc di
	jmp puts_loop1
my_point:
        
        mov ah, 0x00 
        int 0x16 
        
		 
       
		cmp ah, 0x02
        je make_gray;inf_loop

		cmp ah, 0x03
        je make_white;inf_loop

		cmp ah, 0x04
        je make_yellow;inf_loop

		cmp ah, 0x05
        je make_blue;inf_loop

		cmp ah, 0x06
        je make_red;inf_loop

		cmp ah, 0x07
        je make_green;inf_loop

        jmp my_point ; default





;inf_loop:

;jmp inf_loop ; ����������� ����
make_gray:
	mov ah, 0x07
	je load_kernel

make_white:
	mov ah, 0x0f
	je load_kernel

make_yellow:
	mov ah, 0x0e
	je load_kernel

make_blue:
	mov ah, 0x09
	je load_kernel

make_red:
	mov ah, 0x0C
	je load_kernel

make_green:
	mov ah, 0x02
	je load_kernel

colors:
	db 0x07, 0x0F, 0x0E, 0x09, 0x0C, 0x02
message1:
	db   "press 1 - for gray", 0
	db   "press 2 - for white", 0
	db   "press 3 - for yellow", 0
	db   "press 4 - for blue", 0
	db   "press 5 - for red", 0
	db   "press 6 - for green", 0



clear:
	mov ax,3
	int 0x10
	ret

load_kernel:
	
	mov [0x8000], ah; zdes ah=color
	xor bh, bh
	mov bl, 0x8000 ; ���� ������	
	mov ax, 3
	int 0x10
	
	;call load_main
	call clear
	mov bx, 0x1000
    mov es, bx    
  ;  mov bx, ax

    mov dl, 0x01 ;����� �����
    mov dh, 0x00 ;����� �������

    mov cl, 0x02 ;����� �������

    mov ch, 0x00 ;����� ��������

    mov al, 14 ;12  ;���������� ��������

    mov ah, 0x02
    int 0x13

	; mov ah, 0x02
	mov bx, 0x1000
    mov es, bx
    mov bx, 0x3000
   ; mov bx, ax

    mov dl, 0x01;����� �����
    mov dh, 0x00 ;����� �������
    mov cl, 16;14;���� �������

    mov ch, 0x00 ;����� ��������
    mov al, 6;6  ;���������� ��������
    mov ah, 0x02
    int 0x13
    
	; ���������� ����������
	cli
	; �������� ������� � ������ ������� ������������
	lgdt [gdt_info] ; ��� GNU assembler ������ ���� "lgdt gdt_info"
	; ��������� �������� ����� �20
	in al, 0x92
	or al, 2
	out 0x92, al
	; ��������� ���� PE �������� CR0 - ��������� �������� � ���������� �����
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x8:protected_mode ; "�������" ������� ��� �������� ���������� ���������� � cs (������������� ����������� �� ��������� ����� ������� ��������).


[BITS 32] 
protected_mode:
; ����� ���� ������ ���������� � ���������� ������
	mov ax, 0x10    
	mov es, ax
	mov ds, ax
	mov ss, ax
	call 0x11000;12

gdt:
; ������� ����������
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
; ������� ����: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=1(code), Access=00A, G=1, B=32bit
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
; ������� ������: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=0(data), Access=0W0, G=1, B=32bit
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info: ; ������ � ������� GDT (������, ��������� � ������)
    dw gdt_info - gdt ; ������ ������� (2 �����)
    dw gdt, 0 ; 32-������ ���������� ����� �������.

; ��������! ������ ����� ��������� �����������, ���� �������� � ����� ����� 512 ������ ��� ��������� �����: 0x55 � 0xAA
    times (512 - ($ - start) - 2) db 0 ; ���������� ������ �� ������� 512- 2 ������� �����
    db 0x55, 0xAA ; 2 ����������� ����� ����� ������ �������� �����������

