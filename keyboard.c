#include "../keyboard_map.h"

// Current keyboard state
static keyboard_modifiers_t modifiers = {0};
static uint8_t extended_key = 0;
static const keyboard_layout_t* current_layout = &layout_us;

// US QWERTY layout implementation
const keyboard_layout_t layout_us = {
    .normal = {
        // 0x00 - 0x0F
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
        // 0x10 - 0x1F
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
        // 0x20 - 0x2F
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
        // 0x30 - 0x3F
        'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    },
    .shift = {
        // 0x00 - 0x0F
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
        // 0x10 - 0x1F
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
        // 0x20 - 0x2F
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
        // 0x30 - 0x3F
        'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
    },
    .altgr = {0}, // Empty for US layout
    .name = "US QWERTY"
};

// Key name lookup table
static const char* key_names[] = {
    [SC_ESC] = "ESC",
    [SC_F1] = "F1",
    [SC_F2] = "F2",
    [SC_F3] = "F3",
    [SC_F4] = "F4",
    [SC_F5] = "F5",
    [SC_F6] = "F6",
    [SC_F7] = "F7",
    [SC_F8] = "F8",
    [SC_F9] = "F9",
    [SC_F10] = "F10",
    [SC_F11] = "F11",
    [SC_F12] = "F12",
    [SC_BACKSPACE] = "BACKSPACE",
    [SC_TAB] = "TAB",
    [SC_ENTER] = "ENTER",
    [SC_LCTRL] = "LCTRL",
    [SC_LSHIFT] = "LSHIFT",
    [SC_RSHIFT] = "RSHIFT",
    [SC_LALT] = "LALT",
    [SC_CAPS_LOCK] = "CAPS_LOCK",
    [SC_NUM_LOCK] = "NUM_LOCK",
    [SC_SCROLL_LOCK] = "SCROLL_LOCK"
};

// Initialize the keyboard
void keyboard_init(void) {
    // Reset keyboard
    write_port(KB_COMMAND_PORT, KB_CMD_RESET);
    
    // Wait for acknowledgment
    while (read_port(KB_DATA_PORT) != 0xFA);
    
    // Enable scanning
    write_port(KB_COMMAND_PORT, KB_CMD_ENABLE);
    
    // Set default LED state
    keyboard_set_leds(0);
    
    // Clear all modifier states
    modifiers = (keyboard_modifiers_t){0};
}

// Set keyboard LEDs
void keyboard_set_leds(uint8_t leds) {
    write_port(KB_COMMAND_PORT, KB_CMD_SET_LED);
    write_port(KB_DATA_PORT, leds);
}

// Update modifier keys state
static void update_modifiers(uint8_t scancode, uint8_t released) {
    switch(scancode) {
        case SC_LSHIFT:
            modifiers.left_shift = !released;
            break;
        case SC_RSHIFT:
            modifiers.right_shift = !released;
            break;
        case SC_LCTRL:
            if (!extended_key) modifiers.left_ctrl = !released;
            else modifiers.right_ctrl = !released;
            break;
        case SC_LALT:
            if (!extended_key) modifiers.left_alt = !released;
            else modifiers.right_alt = !released;
            break;
        case SC_CAPS_LOCK:
            if (!released) modifiers.caps_lock = !modifiers.caps_lock;
            keyboard_set_leds(
                (modifiers.caps_lock ? LED_CAPS_LOCK : 0) |
                (modifiers.num_lock ? LED_NUM_LOCK : 0) |
                (modifiers.scroll_lock ? LED_SCROLL_LOCK : 0)
            );
            break;
        case SC_NUM_LOCK:
            if (!released) modifiers.num_lock = !modifiers.num_lock;
            keyboard_set_leds(
                (modifiers.caps_lock ? LED_CAPS_LOCK : 0) |
                (modifiers.num_lock ? LED_NUM_LOCK : 0) |
                (modifiers.scroll_lock ? LED_SCROLL_LOCK : 0)
            );
            break;
        case SC_SCROLL_LOCK:
            if (!released) modifiers.scroll_lock = !modifiers.scroll_lock;
            keyboard_set_leds(
                (modifiers.caps_lock ? LED_CAPS_LOCK : 0) |
                (modifiers.num_lock ? LED_NUM_LOCK : 0) |
                (modifiers.scroll_lock ? LED_SCROLL_LOCK : 0)
            );
            break;
    }
}

// Read a key event from the keyboard
key_event_t keyboard_read_event(void) {
    key_event_t event = {0};
    uint8_t scancode = read_port(KB_DATA_PORT);
    
    // Handle extended key sequences
    if (scancode == KEY_EXTENDED) {
        extended_key = 1;
        return event;
    }
    
    event.scancode = scancode & ~KEY_RELEASED;
    event.is_released = (scancode & KEY_RELEASED) != 0;
    event.is_extended = extended_key;
    event.modifiers = modifiers;
    
    // Update modifier states
    update_modifiers(event.scancode, event.is_released);
    
    // Convert scancode to ASCII if possible
    event.ascii = keyboard_get_ascii(event);
    
    // Reset extended key flag
    extended_key = 0;
    
    return event;
}

// Convert a key event to ASCII
uint8_t keyboard_get_ascii(key_event_t event) {
    if (event.is_released || event.is_extended) {
        return 0;
    }
    
    uint8_t shift = event.modifiers.left_shift || event.modifiers.right_shift;
    uint8_t altgr = event.modifiers.right_alt;
    
    // Handle Caps Lock
    if (event.modifiers.caps_lock) {
        // Only affect letters
        if ((current_layout->normal[event.scancode] >= 'a' && 
             current_layout->normal[event.scancode] <= 'z') ||
            (current_layout->normal[event.scancode] >= 'A' && 
             current_layout->normal[event.scancode] <= 'Z')) {
            shift = !shift;
        }
    }
    
    if (altgr && current_layout->altgr[event.scancode]) {
        return current_layout->altgr[event.scancode];
    } else if (shift && current_layout->shift[event.scancode]) {
        return current_layout->shift[event.scancode];
    } else {
        return current_layout->normal[event.scancode];
    }
}

// Set the current keyboard layout
void keyboard_set_layout(const keyboard_layout_t* layout) {
    if (layout) {
        current_layout = layout;
    }
}

// Get the name of a key from its scancode
const char* keyboard_get_key_name(uint8_t scancode) {
    if (scancode < sizeof(key_names) / sizeof(key_names[0]) && key_names[scancode]) {
        return key_names[scancode];
    }
    return "UNKNOWN";
}

// Keyboard interrupt handler
void keyboard_handler(void) {
    key_event_t event = keyboard_read_event();
    
    // Only process key press events (not releases)
    if (!event.is_released && event.ascii) {
        // Add the character to the keyboard buffer
        keyboard_buffer_add(event.ascii);
    }
    
    // Send End of Interrupt
    write_port(0x20, 0x20);
}