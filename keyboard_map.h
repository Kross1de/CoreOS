#ifndef KEYBOARD_MAP_H
#define KEYBOARD_MAP_H

#include <stdint.h>

// keyboard ports
#define KB_DATA_PORT        0x60
#define KB_STATUS_PORT      0x64
#define KB_COMMAND_PORT     0x64

// Keyboard commands
#define KB_CMD_SET_LED   0xED
#define KB_CMD_ECHO      0xEE
#define KB_CMD_SCANCODE  0xF0
#define KB_CMD_SET_RATE  0xF3
#define KB_CMD_ENABLE    0xF4
#define KB_CMD_RESET     0xFF

// LED masks
#define LED_SCROLL_LOCK  0x01
#define LED_NUM_LOCK     0x02
#define LED_CAPS_LOCK    0x04

// Special key flags
#define KEY_RELEASED     0x80
#define KEY_EXTENDED     0xE0

// Modifier key states
typedef struct {
    uint8_t left_shift : 1;
    uint8_t right_shift : 1;
    uint8_t left_ctrl : 1;
    uint8_t right_ctrl : 1;
    uint8_t left_alt : 1;
    uint8_t right_alt : 1;
    uint8_t caps_lock : 1;
    uint8_t num_lock : 1;
    uint8_t scroll_lock : 1;
} keyboard_modifiers_t;

// Scancodes for modifier keys
#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_LCTRL        0x1D
#define SC_RCTRL        0x1D    // Extended (0xE0)
#define SC_LALT         0x38
#define SC_RALT         0x38    // Extended (0xE0)
#define SC_CAPS_LOCK    0x3A
#define SC_NUM_LOCK     0x45
#define SC_SCROLL_LOCK  0x46

// Function key scancodes
#define SC_F1           0x3B
#define SC_F2           0x3C
#define SC_F3           0x3D
#define SC_F4           0x3E
#define SC_F5           0x3F
#define SC_F6           0x40
#define SC_F7           0x41
#define SC_F8           0x42
#define SC_F9           0x43
#define SC_F10          0x44
#define SC_F11          0x57
#define SC_F12          0x58

// Navigation key scancodes
#define SC_ESC          0x01
#define SC_BACKSPACE    0x0E
#define SC_TAB          0x0F
#define SC_ENTER        0x1C
#define SC_HOME         0x47
#define SC_END          0x4F
#define SC_PGUP         0x49
#define SC_PGDN         0x51
#define SC_DEL          0x53
#define SC_INSERT       0x52

// Arrow key scancodes (Extended 0xE0)
#define SC_UP           0x48
#define SC_DOWN         0x50
#define SC_LEFT         0x4B
#define SC_RIGHT        0x4D

// Key event structure
typedef struct {
    uint8_t scancode;
    uint8_t key;
    uint8_t ascii;
    uint8_t is_extended;
    uint8_t is_released;
    keyboard_modifiers_t modifiers;
} key_event_t;

// Keyboard layouts
typedef struct {
    uint8_t normal[128];
    uint8_t shift[128];
    uint8_t altgr[128];
    const char* name;
} keyboard_layout_t;

// US QWERTY layout
extern const keyboard_layout_t layout_us;

// Function declarations
void keyboard_init(void);
void keyboard_set_layout(const keyboard_layout_t* layout);
void keyboard_set_leds(uint8_t leds);
key_event_t keyboard_read_event(void);
uint8_t keyboard_get_ascii(key_event_t event);
const char* keyboard_get_key_name(uint8_t scancode);

#endif // KEYBOARD_MAP_H