
[BITS 16]  ; 32-битный код 
[ORG 0x7c00]; Адрес, по которому будет загружен этот код загрузчиком ядра 
start:
	mov ax, cs ; Сохранение адреса сегмента кода в ax
	mov ds, ax ; Сохранение этого адреса как начало сегмента данных
	mov ss, ax ; И сегмента стеку
	mov sp, start ; Сохранение адреса стека как адрес первой инструкции того кода. Стек будет расти вверх и не перекроет код.
	
	
	mov ah, 0x00
	mov al, 0x00
	int 0x10
	mov ah, 03h
	mov bh, 0x00
	int 0x10

 ; В ah номер функции BIOS: 0x0e - вывод символа на активную видео страницу (эмуляция телетайпа)

	mov  si, message1
	mov di, colors ; Указатель на массив цветов
	cld                     ; направление для строковых команд
	mov  bh, 0x00    ; страница видеопамяти
puts_loop1:
	mov ah, 0x09
	lodsb            ; загружаем очередной символ в al
	mov bl, [di]
	mov cx, 1
	test al, al      ; нулевой символ означает конец строки
	jz   puts_loop_exit1
	int  0x10        ; вызываем функцию BIOS
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

;jmp inf_loop ; Бесконечный цикл
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
	mov bl, 0x8000 ; цвет текста	
	mov ax, 3
	int 0x10
	
	;call load_main
	call clear
	mov bx, 0x1000
    mov es, bx    
  ;  mov bx, ax

    mov dl, 0x01 ;номер диска
    mov dh, 0x00 ;номер головки

    mov cl, 0x02 ;номер сектора

    mov ch, 0x00 ;номер цилиндра

    mov al, 14 ;12  ;количество секторов

    mov ah, 0x02
    int 0x13

	; mov ah, 0x02
	mov bx, 0x1000
    mov es, bx
    mov bx, 0x3000
   ; mov bx, ax

    mov dl, 0x01;номер диска
    mov dh, 0x00 ;номер головки
    mov cl, 16;14;омер сектора

    mov ch, 0x00 ;номер цилиндра
    mov al, 6;6  ;количество секторов
    mov ah, 0x02
    int 0x13
    
	; Отключение прерываний
	cli
	; Загрузка размера и адреса таблицы дескрипторов
	lgdt [gdt_info] ; Для GNU assembler должно быть "lgdt gdt_info"
	; Включение адресной линии А20
	in al, 0x92
	or al, 2
	out 0x92, al
	; Установка бита PE регистра CR0 - процессор перейдет в защищенный режим
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x8:protected_mode ; "Дальний" переход для загрузки корректной информации в cs (архитектурные особенности не позволяют этого сделать напрямую).


[BITS 32] 
protected_mode:
; Здесь идут первые инструкции в защищенном режиме
	mov ax, 0x10    
	mov es, ax
	mov ds, ax
	mov ss, ax
	call 0x11000;12

gdt:
; Нулевой дескриптор
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
; Сегмент кода: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=1(code), Access=00A, G=1, B=32bit
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
; Сегмент данных: base=0, size=4Gb, P=1, DPL=0, S=1(user),
; Type=0(data), Access=0W0, G=1, B=32bit
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
gdt_info: ; Данные о таблице GDT (размер, положение в памяти)
    dw gdt_info - gdt ; Размер таблицы (2 байта)
    dw gdt, 0 ; 32-битный физический адрес таблицы.

; Внимание! Сектор будет считаться загрузочным, если содержит в конце своих 512 байтов два следующих байта: 0x55 и 0xAA
    times (512 - ($ - start) - 2) db 0 ; Заполнение нулями до границы 512- 2 текущей точки
    db 0x55, 0xAA ; 2 необходимых байта чтобы сектор считался загрузочным

