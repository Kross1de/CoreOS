#include "keyboard_map.h"
#include "drivers/keyboard.c"
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

// Function prototype for clear_screen
void clear_screen(void);
void outb(unsigned short port, unsigned char data);
void outw(unsigned short port, unsigned short data);
void update_cursor(int position);
void reboot(void);
void print_colored(const char *str, unsigned char color);
void printn_colored(int num, unsigned char color);

// Colors
#define COLOR_BLACK 0x00
#define COLOR_BLUE 0x01
#define COLOR_GREEN 0x02
#define COLOR_CYAN 0x03
#define COLOR_RED 0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN 0x06
#define COLOR_LIGHT_GRAY 0x07
#define COLOR_DARK_GRAY 0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN 0x0B
#define COLOR_LIGHT_RED 0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW 0x0E
#define COLOR_WHITE 0x0F

unsigned char current_color = COLOR_LIGHT_GRAY; // Current text color
unsigned char caps_lock = 0;
unsigned char shift_pressed = 0;
unsigned char alt_pressed = 0;
unsigned char ctrl_pressed = 0;

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES)
// KEYBOARD
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define ENTER_KEY_CODE 0x1C
#define BACKSPACE_KEY_CODE 0x0E
#define CAPS_LOCK_CODE 0x3A
#define LEFT_SHIFT_CODE 0x2A
#define RIGHT_SHIFT_CODE 0x36
#define LEFT_SHIFT_RELEASE_CODE 0xAA
#define RIGHT_SHIFT_RELEASE_CODE 0xB6
#define CTRL_CODE 0x1D
#define ALT_CODE 0x38
#define ESC_KEY_CODE 0x01
#define F1_KEY_CODE 0x3B
#define F2_KEY_CODE 0x3C
#define F3_KEY_CODE 0x3D
#define F4_KEY_CODE 0x3E
#define F5_KEY_CODE 0x3F
#define F6_KEY_CODE 0x40
#define F7_KEY_CODE 0x41
#define F8_KEY_CODE 0x42
#define F9_KEY_CODE 0x43
#define F10_KEY_CODE 0x44
#define F11_KEY_CODE 0x57
#define F12_KEY_CODE 0x58
//////////
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define MAX_INPUT_SIZE 256 // Максимальная длина ввода
#define MAX_INPUT_BUFFER_SIZE 256
#define INT_MAX 2147483647
#define INT_MIN -2147483648

char input_buffer[MAX_INPUT_BUFFER_SIZE];
unsigned int input_buffer_index = 0; 

extern unsigned char keyboard_map[128];
extern unsigned char keyboard_map_caps[128];
extern unsigned char inb(unsigned short port);
extern void keyboard_handler(void);
extern const keyboard_layout_t layout_us;
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
extern void print(const char *str);
extern void print_char(char c);
extern void clear_screen();
extern void reboot();
extern void keyboard_init(void);
extern key_event_t keyboard_read_event(void);
extern void keyboard_set_layout(const keyboard_layout_t* layout);

unsigned int current_loc = 0;
char *vidptr = (char*)0xb8000;
unsigned int lines = 0; // Переменная для отслеживания количества строк

struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void set_color(unsigned char color) {
    current_color = color; 
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    while (*str == ' ') str++;

    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        int digit = *str - '0';
        if (result > (INT_MAX - digit) / 10) {
            return (sign == 1) ? INT_MAX : INT_MIN;
        }
        result = result * 10 + digit;
        str++;
    }

    return sign * result;
}

void show_cursor() {
	vidptr[current_loc * 2] = '_'; // Cursor
	vidptr[current_loc * 2 + 1] = 0x07; // Cursor color
	update_cursor(current_loc);
}

void hide_cursor() {
	vidptr[current_loc * 2] = ' '; // Hiding cursor
	vidptr[current_loc * 2 + 1] = 0x07; 
	update_cursor(current_loc);
}

void update_cursor(int position) {
	unsigned short pos = (unsigned short)position;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void print(const char *str) {
    print_colored(str, current_color);
}

void print_colored(const char *str, unsigned char color) {
    unsigned int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            current_loc += 80 - (current_loc % 80);
            lines++;
        } else {
            vidptr[current_loc * 2] = str[i];
            vidptr[current_loc * 2 + 1] = color;
            current_loc++;
        }

        if (current_loc >= 80 * 25) {
            current_loc = 0;
        }
        i++;
    }

    if (lines >= LINES) {
        clear_screen();
        lines = 0;
    }

    // Updating cursor after printing text
    update_cursor(current_loc);
}

void printn(int num) {
    printn_colored(num, current_color);
}

void printn_colored(int num, unsigned char color) {
    char buffer[32];
    int i = 0, isNegative = 0;

    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    do {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    if (isNegative) {
        buffer[i++] = '-';
    }

    for (int j = i - 1; j >= 0; j--) {
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;

        if (current_loc >= 80 * 25) {
            current_loc = 0;
        }
    }

    if (lines >= LINES) {
        clear_screen();
        lines = 0;
    }
}

void kprint_newline(void) {
    current_loc += (80 - (current_loc % 80));
    lines++; // Увеличиваем количество строк
}

void clear_screen(void) {
    unsigned int i = 0;
    while (i < SCREENSIZE) {
        vidptr[i++] = ' ';
        vidptr[i++] = 0x07;
    }
    current_loc = 0;
    lines = 0; // Сбрасываем количество строк
}

void keyboard_handler_main(void) {
    key_event_t event = keyboard_read_event();
    
    if (!event.is_released && event.ascii) {
        // Handle normal keypresses
        vidptr[current_loc * 2] = event.ascii;
        vidptr[current_loc * 2 + 1] = current_color;
        current_loc++;
        
        if (current_loc >= 80 * 25) {
            current_loc = 0;
        }
    }
    
    // Send End of Interrupt
    write_port(0x20, 0x20);
}

void keyboard_init() {
    // initialize keyboard_map
    for (int i = 0; i < 128; i++) {
        keyboard_map[i] = 0;
        keyboard_map_caps[i] = 0;
    }
    outb(0x21, 0xFD);
}

void input(char *buffer, int max_size) {
    unsigned int index = 0;
    while (index < max_size - 1) {
        unsigned char status = read_port(KEYBOARD_STATUS_PORT);
        if (status & 0x01) {
            unsigned char keycode = read_port(KEYBOARD_DATA_PORT);
            if (keycode == ENTER_KEY_CODE) {
                break;
            }
            if (keycode < 128) {
                char c = keyboard_map[keycode];
                if (c != 0) {
                    if (c == '\b') {
                        if (index > 0) {
                            index--;
                            vidptr[current_loc * 2 - 2] = ' ';
                            vidptr[current_loc * 2 - 1] = 0x07;
                            current_loc--;
                        }
                    } else {
                        // Apply Caps Lock and Shift logic
                        if (caps_lock ^ shift_pressed) { // XOR: if Caps Lock or Shift is pressed, but not both
                            if (c >= 'a' && c <= 'z') {
                                c -= 32; // Convert to uppercase
                            }
                        }
                        buffer[index++] = c;
                        vidptr[current_loc * 2] = c;
                        vidptr[current_loc * 2 + 1] = 0x07;
                        current_loc++;
                    }
                }
            }
        }
        // Updating cursor after entering every symbol
        update_cursor(current_loc);
    }
    buffer[index] = '\0';
    kprint_newline();
}

int inputn() {
    char buffer[MAX_INPUT_SIZE];
    input(buffer, MAX_INPUT_SIZE);
    return atoi(buffer);
}

void idt_init(void) {
    unsigned long keyboard_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];

    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    write_port(0x20, 0x11);
    write_port(0xA0, 0x11);
    write_port(0x21, 0x20);
    write_port(0xA1, 0x28);
    write_port(0x21, 0x00);
    write_port(0xA1, 0x00);
    write_port(0x21, 0x01);
    write_port(0xA1, 0x01);

    write_port(0x21, 0xff);
    write_port(0xA1, 0xff);

    idt_address = (unsigned long)IDT;
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16;
    load_idt(idt_ptr);
}

void kb_init(void) {
    write_port(0x21, 0xFD);
}

unsigned long factorial(int n) {
    if (n < 0) return 0; // Факториал для отрицательных чисел не определен
    unsigned long result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

void factorial_calculator(void) {
    print("\nFACTORIAL CALCULATOR");
    kprint_newline();
    int num;
    print("Please enter a number:");
    num = inputn();
    unsigned long result = factorial(num);
    printn(num);
    print("! = ");
    printn(result);
    print("\n");
    kprint_newline();
}

void decimal_to_binary(int n) {
    if (n == 0) {
        print("0");
        return;
    }
    char binary[32];
    int index = 0;
    while (n > 0) {
        binary[index++] = (n % 2) + '0';
        n /= 2;
    }
    for (int i = index - 1; i >= 0; i--) {
        vidptr[current_loc * 2] = binary[i];
        vidptr[current_loc * 2 + 1] = 0x07;
        current_loc++;
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

unsigned int rand() {
    static unsigned int seed = 12345; // Начальное значение
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed % 3; // Получаем число от 0 до 2
}

void delay(unsigned int count) {
    for (volatile unsigned int i = 0; i < count; i++);
}

void welcome_screen(void) {
    clear_screen();
    print_colored("========================================\n", COLOR_LIGHT_BLUE);
    print_colored("          Welcome to CoreOS             \n", COLOR_LIGHT_GREEN);
    print_colored("========================================\n", COLOR_LIGHT_BLUE);
    print_colored("Version: 1.0\n", COLOR_LIGHT_CYAN);
    print_colored("Developed by: Syncas\n", COLOR_LIGHT_MAGENTA);
    print_colored("License: MIT License\n", COLOR_BROWN);
    print_colored("Credits to: PRoX (i've used source code of 'PRoX Kernel v1.0)\n", COLOR_LIGHT_RED);
    print_colored("========================================\n", COLOR_LIGHT_BLUE);
    print_colored("Press any key to continue...\n", COLOR_LIGHT_GREEN);

    // Wait for any key press
    while (1) {
        unsigned char status = read_port(KEYBOARD_STATUS_PORT);
        if (status & 0x01) {
            read_port(KEYBOARD_DATA_PORT);
            break;
        }
    }
}

void shutdown() {
    outw(0x604, 0x2000);
}

void reboot() {
    outb(0x64, 0xFE);
}

// New function to display a fake date
void display_date() {
    print_colored("Current date: 2023-10-01\n", COLOR_LIGHT_CYAN);
}

// New function to echo text
void echo_command(const char *text) {
    print_colored(text, COLOR_LIGHT_GREEN);
    print("\n");
}

void kmain(void) {
    // initializing keyboard
    keyboard_init();
    keyboard_set_layout(&layout_us);
    welcome_screen();
    clear_screen();
    print_colored("Hello, user\nCoreOS are successfully booted!\n", COLOR_LIGHT_GREEN);

    char input_buffer[MAX_INPUT_SIZE];
    while (1) {
        print_colored("[CoreOS]$ ", COLOR_LIGHT_BLUE);
        show_cursor(); // Showing cursor after prompt
        input(input_buffer, MAX_INPUT_SIZE);
        hide_cursor(); // Hiding cursor after entering symbol

        if (strcmp(input_buffer, "shutdown") == 0) {
            print_colored("Shutting down...\n", COLOR_LIGHT_RED);
            shutdown();
        } else if (strcmp(input_buffer, "reboot") == 0) {
            print_colored("Rebooting...\n", COLOR_LIGHT_RED);
            reboot();
        } else if (strcmp(input_buffer, "clear") == 0) {
            clear_screen();
        } else if (strcmp(input_buffer, "factorial") == 0) {
            factorial_calculator();
        } else if (strcmp(input_buffer, "binary") == 0) {
            print("Enter a number: ");
            int num = inputn();
            print("Binary: ");
            decimal_to_binary(num);
            print("\n");
        } else if (strcmp(input_buffer, "color") == 0) {
            print("Enter color (0-15): ");
            int color = inputn();
            if (color >= 0 && color <= 15) {
                set_color(color);
                print_colored("Color changed!\n", color);
            } else {
                print_colored("Invalid color!\n", COLOR_LIGHT_RED);
            }
        } else if (strcmp(input_buffer, "date") == 0) {
            display_date();
        } else if (strcmp(input_buffer, "echo") == 0) {
            print("Enter text: ");
            input(input_buffer, MAX_INPUT_SIZE);
            echo_command(input_buffer);
        } else if (strcmp(input_buffer, "help") == 0) {
            print_colored("Available commands:\n", COLOR_LIGHT_GREEN);
            print_colored("  clear - Clear the screen\n", COLOR_LIGHT_GRAY);
            print_colored("  factorial - Calculate factorial of a number\n", COLOR_LIGHT_GRAY);
            print_colored("  binary - Convert a number to binary\n", COLOR_LIGHT_GRAY);
            print_colored("  color - Change text color\n", COLOR_LIGHT_GRAY);
            print_colored("  date - Display current date\n", COLOR_LIGHT_GRAY);
            print_colored("  echo - Echo text\n", COLOR_LIGHT_GRAY);
            print_colored("  shutdown - Shutdown PC\n", COLOR_LIGHT_GRAY);
            print_colored("  reboot - Reboot PC\n", COLOR_LIGHT_GRAY);
            print_colored("  help - Show this help message\n", COLOR_LIGHT_GRAY);
        } else {
            print_colored("Unknown command: ", COLOR_LIGHT_RED);
            print_colored(input_buffer, COLOR_LIGHT_RED);
            print("\n");
        }
    }
}
