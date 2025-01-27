// Эта инструкция обязательно должна быть первой, т.к. этот код 
// компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарного 
// образа ядра ОС.
extern "C" int kmain();
__declspec(naked) void startup()
{
	__asm {
		call kmain;
	}
}
#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
#define PIC1_PORT (0x20)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана
#define BUFFER (0x8f00)
#define SHUTPORT (0x604)

#define GDT_CS (0x8) // Селектор секции кода, установленный загрузчиком ОС

char keyboard[54] = { ' ', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', ' ', ' ', 'q', 'w', 'e', 'r', 't',
					  'y', 'u', 'i', 'o', 'p', '[', ']', ' ', ' ', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
					  '`', ' ', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/' };
char keyboard_shift[54] = { 0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0, 'Q', 'W', 'E', 'E', 'T',
					  'Y', 'U', 'I', 'O', 'P','{', '}', 0, 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',  ':', '\"', '~', 0,
						'|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?' };


unsigned short shift;
int* color1 = (int*)(0x8000);
//char buf_f[20];
#pragma pack(push, 1)
void scroll()
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	for (int i = 0; i < VIDEO_WIDTH * 25 * 4; i++)
	{
		//video_buf[1] = *color1; // Цвет символа и фона
		*(video_buf + i) = *(video_buf + 80 * 4 + (i));
	}

	return;
}
struct idt_entry
{
	unsigned short base_lo; // Младшие биты адреса обработчика
	unsigned short segm_sel; // Селектор сегмента кода
	unsigned char always0; // Этот байт всегда 0
	unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	unsigned short base_hi; // Старшие биты адреса обработчика
};

// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
};
#pragma pack(pop)

struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt

__declspec(naked) void default_intr_handler()
{
	__asm {
		pusha
	}
	// ... (реализация обработки)
	__asm {
		popa
		iretd
	}
}

typedef void (*intr_handler)();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
	flags, intr_handler hndlr)
{
	unsigned int hndlr_addr = (unsigned int)hndlr;
	g_idt[num].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
	g_idt[num].segm_sel = segm_sel;
	g_idt[num].always0 = 0;
	g_idt[num].flags = flags;
	g_idt[num].base_hi = (unsigned short)(hndlr_addr >> 16);
}

// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_init()
{
	int i;
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	for (i = 0; i < idt_count; i++)
		intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
			default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}

void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	g_idtp.base = (unsigned int)(&g_idt[0]);
	g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
	__asm {
		lidt g_idtp
	}
	//__lidt(&g_idtp);
}

void intr_enable()
{
	__asm sti;
}

void intr_disable()
{
	__asm cli;
}


void out_str(int color, const char* ptr, unsigned int strnum)
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * strnum;
	while (*ptr)
	{
		//*video_buf = (unsigned char)*ptr | (color << 8);
		//video_buf++;
		video_buf[0] = (unsigned char)*ptr; // Символ (код)
		video_buf[1] = color; // Цвет символа и фона
		video_buf += 2;
		ptr++;
	}
}
void out_str1(int color, const char* ptr, unsigned int strnum, unsigned short consti)
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * strnum + consti;
	while (*ptr)
	{
		//*video_buf = (unsigned char)*ptr | (color << 8);
		//video_buf++;
		video_buf[0] = (unsigned char)*ptr; // Символ (код)
		video_buf[1] = color; // Цвет символа и фона
		video_buf += 2;
		ptr++;
	}
}

void out_chr(int color, unsigned char symbol, unsigned short posx, unsigned short posy) {
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * posy + (posx * 2);
	video_buf[0] = symbol;
	video_buf[1] = color;
}

__inline unsigned char inb(unsigned short port)
{
	unsigned char data;
	__asm {
		push dx
		mov dx, port
		in al, dx
		mov data, al
		pop dx
	}
	return data;
}

__inline void outb(unsigned short port, unsigned char data)
{
	__asm {
		push dx
		mov dx, port
		mov al, data
		out dx, al
		pop dx
	}
}

__inline void outw(unsigned short port, unsigned short data)
{
	__asm {
		push dx
		mov dx, port
		mov ax, data
		out dx, ax
		pop dx
	}
}

unsigned short get_cursor() {
	unsigned short pos = 0;
	outb(CURSOR_PORT, 0x0F);
	pos |= inb(CURSOR_PORT + 1);
	outb(CURSOR_PORT, 0x0E);
	pos |= (unsigned short)inb(CURSOR_PORT + 1) << 8;
	return pos;
}

// Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию 
// pos на этой строке(0 – самое левое положение).
void cursor_moveto(unsigned int strnum, unsigned int pos)
{
	unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
	outb(CURSOR_PORT, 0x0F);
	outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
	outb(CURSOR_PORT, 0x0E);
	outb(CURSOR_PORT + 1, (unsigned char)((new_pos >> 8) & 0xFF));
}

char* readline(unsigned int strnum) {
	char* ptr = { 0 };
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	video_buf += 80 * 2 * strnum;
	char i = 0;
	while (i < 40) {
		ptr[i] = video_buf[0];
		video_buf += 2;
		i++;
	}
	ptr[i] = 0;
	return ptr;
}

char strncmp(char* str1, char* str2, unsigned char n) {
	char i = 0;
	while (i < n) {
		if (*str1 == *str2) {
			str1++;
			str2++;
			i++;
		}
		else if (*str1 > *str2) {
			return 1;
		}
		else {
			return -1;
		}
	}
	return 0;
}

char strparse(char* str) {
	char i = 0;
	char col = 0;
	while (i < 40 || str[i]) {
		if (str[i] == ' ') {
			str[i] = 0;
		}
		if (str[i] != ' ' && str[i] != 0 && (str[i + 1] == ' ' || str[i + 1] == 0)) col++;
		i++;
	}
	return col;
}

char* strroll(char* str, char num) {
	char i = 0;
	char col = 0;
	char* ptr = "h";
	while (i < 40 && col < num) {
		if (str[i] != 0 && (i == 0 || str[i - 1] == 0)) {
			col++;
		}
		i++;
	}
	ptr = &str[i - 1];
	return ptr;
}

char* strswap(char* str1, char* str2) {
	char* str3 = "                                                                      ";
	char i = 0;
	while (*str1) {
		str3[i] = *str1;
		str1++;
		i++;
	}
	while (*str2) {
		str3[i] = *str2;
		str2++;
		i++;
	}
	str3[i] = 0;
	return str3;
}

char* numberTostr(char i) {
	char* str = "0";
	char j = 0;
	if (i == 0) {
		str[0] = 48;
		str[1] = 0;
	}
	else {
		while (i % 10 || i / 10) {
			str[j] = (i % 10) + 48;
			i = i / 10;
			j++;
		}
		char k = 0;
		while (k < j / 2) {
			char t = str[k];
			str[k] = str[j - 1 - k];
			str[j - 1 - k] = t;
			k++;
		}
		str[j] = 0;
	}
	return str;
}

char strlen(char* str) {
	char len = 0;
	while (*str!='\0') {
		len++;
		str++;
	}
	return len;
}

void clear() {
	char* cl = "                                                                               ";
	char i = 0;
	while (i < 25) {
		out_str(*color1, cl, i);
		i++;
	}
}

void info(unsigned short statey) {
	char* infostr = "CalcOS. Author: Ksenia Kuzmina, 5131001/20003";
	statey++;
	if (statey > 24) {
		scroll();
		cursor_moveto(24, 0);
		statey = 24;
	}
	out_str(*color1, infostr, statey);
	infostr = "Compilers: bootloader: yasm, kernel: MC C Compiler";
	statey++;
	if (statey > 24) {
		scroll();
		cursor_moveto(24, 0);
		statey = 24;
	}
	out_str(*color1, infostr, statey);
	
	statey++;
	if (statey > 24) {
		scroll();
		cursor_moveto(24, 0);
		statey = 24;
	}
	cursor_moveto(statey, 0);
}
bool checking(char el, char alph[11]) {
	for (int i = 0; i < 11; i++) {
		if (el == alph[i]) {
			return true;
		}
	}
	return false;
}
bool checking_sign(char el, char sign[5]) {
	for (int i = 0; i < 5; i++) {
		if (el == sign[i]) {
			return true;
		}
	}
	return false;
}

int char_to_int(char symbol) {
	switch (symbol) {
	case '1': return 1;
	case '2': return 2;
	case '3': return 3;
	case '4': return 4;
	case '5': return 5;
	case '6': return 6;
	case '7': return 7;
	case '8': return 8;
	case '9': return 9;
	case '0': return 0;
	}
	return -1;
}
void swap(char& a, char& b)
{
	char temp = a;
	a = b;
	b = temp;
}
void strrev(char str[], int length)
{
	int start = 0;
	int end = length - 1;
	while (start < end)
	{
		swap(*(str + start), *(str + end));
		start++;
		end--;
	}
}
char* itoa(int num, char* str, int base)
{
	for (int i = 0; i < 20; i++)str[i] = 0;
	int i = 0;
	int is_negative = 0;

	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	if (num < 0 && base == 10)
	{
		is_negative = 1;
		num = -num;
	}
	while (num != 0)
	{
		int rem = num % base;
		str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		num = num / base;
	}

	if (is_negative)
		str[i++] = '-';
	str[i] = '\0';
	strrev(str, i);
	return str;
}


int main_maker(char* s, unsigned short statey,char buf_f[20]) {

		char symbol = s[0];
		int i = 0;

		unsigned int stack_mas[200];
		int stack_i = 0;

		char symb_mas[200];
		int symb_i = 0;

		char alph[11] = "1234567890";
		char alph_sign[5] = "+*-/";

		int value = 0;

		int counter = 0;

		int un_sub = 1;
		int flag = 0;

		int ind = -1;
		
		while (symbol != '\0') {
			
			if (checking(symbol, alph) == true) {
				value = char_to_int(symbol);
				//printf("%c %d\n", symbol, initial_value);
				i++;
				symbol = s[i];
				while (checking_sign(symbol, alph_sign) == false && symbol != '\0' && symbol != ')') {
					value *= 10;
					value += char_to_int(symbol);
					i++;
					symbol = s[i];
				}
				//printf(" %d\n", value);
				stack_mas[stack_i] = value * un_sub;
				un_sub = 1;
				stack_i++;
			}
			if (symb_i > 0 && symb_mas[symb_i - 1] == '*' && symb_mas[symb_i] != '(' && stack_i > symb_i) {
				stack_mas[stack_i - 2] *= stack_mas[stack_i - 1];
				symb_i--;
				stack_i--;
			}
			else if (symb_i > 0 && symb_mas[symb_i - 1] == '/' && symb_mas[symb_i] != '(' && stack_i > symb_i) {
				if (stack_mas[stack_i - 1] == 0) {
					//printf("Error: division by 0\0");
					//out_str(0x007, "Error: division by 0", strum1);
					//cursor_moveto(strum1, 0);
					return -1;
				}
				stack_mas[stack_i - 2] /= stack_mas[stack_i - 1];
				symb_i--;
				stack_i--;
			}
			else if (symbol == ')') {
				while (symb_mas[symb_i - 1] != '(') {
					if (symb_i > 0 && symb_mas[symb_i - 1] == '+') {
						stack_mas[stack_i - 2] += stack_mas[stack_i - 1];
						stack_mas[stack_i - 1] = value;
						//symb_mas[symb_i - 2] = symb_mas[symb_i - 1];
						symb_i--;
						stack_i--;
					}
					else if (symb_i > 1 && symb_mas[symb_i - 1] == '-') {
						stack_mas[stack_i - 2] -= stack_mas[stack_i - 1];
						stack_mas[stack_i - 1] = value;
						//symb_mas[symb_i - 2] = symb_mas[symb_i - 1];
						symb_i--;
						stack_i--;
					}
					else if (symb_i > 1 && symb_mas[symb_i - 1] == '*') {
						stack_mas[stack_i - 2] *= stack_mas[stack_i - 1];
						stack_mas[stack_i - 1] = value;
						//symb_mas[symb_i - 2] = symb_mas[symb_i - 1];
						symb_i--;
						stack_i--;
					}
					else {
						if (stack_mas[stack_i - 1] == 0) {
							return -1;
						}
						stack_mas[stack_i - 2] /= stack_mas[stack_i - 1];
						stack_mas[stack_i - 1] = value;
						//symb_mas[symb_i - 2] = symb_mas[symb_i - 1];
						symb_i--;
						stack_i--;
					}
				}




				//	stack_mas[ind[ind_c-1]] = stack_mas[ind[ind_c-1] + 1];
				//	stack_i = ind[ind_c - 1] + 1;
				stack_mas[symb_i - 1] = stack_mas[symb_i];
				stack_i = symb_i;
				symb_i -= 1;
				//symb_i = ind[ind_c - 1];

				i++;
				symbol = s[i];
			}
			


			if (symbol == '(') {

				counter += 1;


				symb_mas[symb_i] = symbol;
				ind = symb_i;
				symb_i++;
				stack_i++;

				i++;
				symbol = s[i];

				if (symbol == '+' || symbol == '*' || symbol == '/') {
					//printf("Error: incorrect expression\n");
					//out_str(0x007, "Error: incorrect expression", strum1);
					//ursor_moveto(strum1, 0);
					return 0;
					//break;
				}
				if (symbol == '-') {
					un_sub = -1;
					i++;
					symbol = s[i];
					while (checking(symbol, alph) != true && symbol == '-') {
						i++;
						symbol = s[i];
					}
				}
				if (symbol == '+' || symbol == '*' || symbol == '/') {
					//printf("Error: incorrect expression\n");
					//out_str(0x007, "Error: incorrect expression", strum1);
					//cursor_moveto(strum1, 0);
					//break;
					return 0;
				}



			}
		

			if (symbol == '\0') {
				//printf("%d %d %d\n", symb_i, stack_i, stack_mas[0]);
			//	for (int i = 0; i < stack_i; i++) printf("%d ", stack_mas[i]);
			//	printf("\n");
			//	for (int i = 0; i < symb_i; i++) printf("%c ", symb_mas[i]);
			//	printf("\n");
				while (symb_i > 0) {
					if (symb_mas[symb_i - 1] == '+') stack_mas[symb_i - 1] += stack_mas[symb_i];
					else if (symb_mas[symb_i - 1] == '*') stack_mas[symb_i - 1] *= stack_mas[symb_i];
					else if (symb_mas[symb_i - 1] == '/') stack_mas[symb_i - 1] /= stack_mas[symb_i];
					else stack_mas[symb_i - 1] -= stack_mas[symb_i];
					symb_i--;
				}
				//	printf("%d\n", stack_mas[0]);
				break;
			}


			if (checking_sign(symbol, alph_sign) == true) {
				if (i > 0) {
					if (symbol == '-') {
						un_sub = -1;
						symbol = '+';
					}
					symb_mas[symb_i] = symbol;
					symb_i++;
					i++;
					symbol = s[i];
					if (checking_sign(symbol, alph_sign) == true) {
						//printf("Error: incorrect expression\n");
						//out_str(0x007, "Error: incorrect expression", strum1);
						//cursor_moveto(strum1, 0);
						return 0;
						//break;
					}
				}
				else if (i == 0) {//если че flag сюда
					if (symbol == '+' || symbol == '*' || symbol == '/') {
						//printf("Error: incorrect expression\n");
						//printf("Error: incorrect expression\n");
						//out_str(0x007, "Error: incorrect expression", strum1);
						//cursor_moveto(strum1, 0);
						return 0;

					}
					if (symbol == '-') {
						un_sub = -1;
						i++;
						symbol = s[i];
						while (checking(symbol, alph) != true && symbol == '-') {
							i++;
							symbol = s[i];
						}
					}
					if (symbol == '+' || symbol == '*' || symbol == '/') {
						//printf("Error: incorrect expression\n");
						//out_str(0x007, "Error: incorrect expression", strum1);
						//cursor_moveto(strum1, 0);

						return 0;
					}
				}
			}

		}
		//printf("%d", stack_mas[0]);
		
		itoa(stack_mas[0], buf_f, 10);
		if ((stack_mas[0] >= 0 && stack_mas[0] > 0x7fffffff && strlen(buf_f) >= 10) || (stack_mas[0] < 0 && stack_mas[0] * (-1) > 0x80000000 && strlen(buf_f) >= 11))
		{
			//out_str(0x007, "Error: Integer overflow", strum1);
			//cursor_moveto(strum1, 0);
			return -2;
		}
		


	return 1;
}


void expr(char* word, unsigned short statey) {
	
	word = strroll(word, 2);
	char buf_f[20];
	int k = main_maker(word,statey,buf_f);
	
	statey++;
	switch (k) {
	case 1:
		out_str(*color1, "Result: ", statey);
		//cursor_moveto(statey, 8);
		out_str1(*color1,buf_f, statey,20);
		break;

	case -1:
		out_str(*color1, "Error: division by 0", statey);break;

	case 0:
		out_str(*color1, "Error: incorrect expression", statey); break;

	case -2:
		out_str(*color1, "Error: Integer overflow", statey); break;

	}

	statey++;
	if (statey > 24) {
		//clear();
		scroll();
		cursor_moveto(24, 0);
		statey = 24;
	}
	cursor_moveto(statey, 0);

	return;
}

void shutdown() {
	outw(SHUTPORT, 0x2000);
}

char check_command(char* command) {
	char* prog1 = "info";
	char* prog2 = "expr";

	char* prog5 = "shutdown";
	char col = strparse(command);
	if (!strncmp(strroll(command, 1), prog1, 5)) {
		if (col == 1) return 0;
		else return -2;
	}
	else if (!strncmp(strroll(command, 1), prog2, 5)) {
		if (col == 2) return 1;
		else return -2;
	}

	else if (!strncmp(strroll(command, 1), prog5, 9)) {
		if (col == 1) return 4;
		else return -2;
	}
	return -1;
}

void onkey(unsigned char scan_code, unsigned short statex, unsigned short statey)
{
	if (scan_code == 14 && statex > 0) {
		cursor_moveto(statey, statex - 1);
		const char str = '\0';
		out_chr(*color1, str, statex - 1, statey);
	}
	else if (scan_code == 57 && statex < 40) {
		const char str = ' ';
		out_chr(*color1, str, statex, statey);
		cursor_moveto(statey, statex + 1);
	}
	else if (scan_code == 28) {
		char* line = readline(statey);
		char commandId = check_command(line);
		if (commandId == 0) {
			info(statey);
		}
		else if (commandId == 1) {
			expr(line,statey);
		}
		
		else if (commandId == 4) {
			shutdown();
		}
		else if (commandId == -2) {
			statey++;
			if (statey > 24) {
				scroll();
				cursor_moveto(24, 0);
				statey = 24;
			}
			out_str(*color1, "Many or few parametres", statey);
			statey++;
			if (statey > 24) {
				scroll();
				cursor_moveto(24, 0);
				statey = 24;
			}
			cursor_moveto(statey, 0);
		}
		else if (commandId == -1) {
			statey++;
			if (statey > 24) {
				scroll();
				cursor_moveto(24, 0);
				statey = 24;
			}
			out_str(*color1, "Have not this command", statey);
			statey++;
			if (statey > 24) {
				scroll();
				cursor_moveto(24, 0);
				statey = 24;
			}
			cursor_moveto(statey, 0);
		}
	}
	else if (scan_code == 42 || scan_code == 54)
	{
		shift = 1;
	}
	else if ((scan_code < 54 && scan_code > 1) && (scan_code < 14 || scan_code > 15) && (scan_code < 28 || scan_code > 29) && scan_code != 42
		&& statex < 40) {

		if (shift) {
			const char str = keyboard_shift[scan_code];
			out_chr(*color1, str, statex, statey);
			cursor_moveto(statey, statex + 1);
			shift = 0;
		}
		else {
			const char str = keyboard[scan_code];
			out_chr(*color1, str, statex, statey);
			cursor_moveto(statey, statex + 1);
		}

	}

	else if ((scan_code < 54 && scan_code > 1) && (scan_code < 14 || scan_code > 15) && (scan_code < 28 || scan_code > 29) && scan_code != 42
		&& statex < 40) {
		const char str = keyboard[scan_code];
		out_chr(*color1, str, statex, statey);
		cursor_moveto(statey, statex + 1);
	}
}

void keyb_process_keys()
{
	// Проверка что буфер PS/2 клавиатуры не пуст (младший бит 
	// присутствует)
	if (inb(0x64) & 0x01)
	{
		unsigned char scan_code;
		unsigned short statex;
		unsigned short statey;
		scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
		if (scan_code < 128) { // Скан-коды выше 128 - это отпускание клавиши
			statey = get_cursor() / VIDEO_WIDTH;
			statex = get_cursor() % VIDEO_WIDTH;
			onkey(scan_code, statex, statey);
			statey = get_cursor() / VIDEO_WIDTH;
			if (statey > 24) {
				scroll();
				cursor_moveto(24, 0);
				statey = 24;
			}
		}
	}
}

__declspec(naked) void keyb_handler()
{
	__asm pusha;
	// Обработка поступивших данных
	keyb_process_keys();
	// Отправка контроллеру 8259 нотификации о том, что прерывание 
	// обработано.Если не отправлять нотификацию, то контроллер не будет посылать
	//	новых сигналов о прерываниях до тех пор, пока ему не сообщать что
	//	прерывание обработано.
	outb(PIC1_PORT, 0x20);
	__asm {
		popa
		iretd
	}
}

void keyb_init()
{
	// Регистрация обработчика прерывания
	intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
	// segm_sel=0x8, P=1, DPL=0, Type=Intr
	// Разрешение только прерываний клавиатуры от контроллера 8259
	outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1(клавиатура).
	// Разрешены будут только прерывания, чьи биты установлены в 0
}

extern "C" int kmain()
{
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();
	const char* hello = "Welcome to CalcOS!";
	// Вывод строки
	//out_str(0x07, hello, 0);
	//cursor_moveto(1, 0);

	out_str(*color1, hello, 0);
	cursor_moveto(1, 0);
	// Бесконечный цикл
	while (1)
	{
		__asm hlt;
	}
	return 0;
}