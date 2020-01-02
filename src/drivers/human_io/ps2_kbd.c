//Neutron Project
//PS/2 keyboard driver

#include "./ps2_kbd.h"
#include "./ps2.h"
#include "../../stdlib.h"
#include "../gfx.h"

/*
 * PS/2 keyboard scancode to Neutron key number map
 */
static ps2_kbd_scan_map_t ps2_kbd_scan_map[] = {
    //First row
    {.ps2_scan = 0x76, .num = KBD_SCAN_ESC},
    {.ps2_scan = 0x05, .num = KBD_SCAN_F1}, {.ps2_scan = 0x06, .num = KBD_SCAN_F2}, {.ps2_scan = 0x04, .num = KBD_SCAN_F3},
    {.ps2_scan = 0x0C, .num = KBD_SCAN_F4}, {.ps2_scan = 0x03, .num = KBD_SCAN_F5}, {.ps2_scan = 0x0B, .num = KBD_SCAN_F6},
    {.ps2_scan = 0x83, .num = KBD_SCAN_F7}, {.ps2_scan = 0x0A, .num = KBD_SCAN_F8}, {.ps2_scan = 0x01, .num = KBD_SCAN_F9},
    {.ps2_scan = 0x09, .num = KBD_SCAN_F10}, {.ps2_scan = 0x78, .num = KBD_SCAN_F11}, {.ps2_scan = 0x07, .num = KBD_SCAN_F12},
    {.ps2_scan = 0xE071, .num = KBD_SCAN_DELETE},
    //Second row
    {.ps2_scan = 0x0E, .num = KBD_SCAN_BACK_TICK}, {.ps2_scan = 0x16, .num = KBD_SCAN_1}, {.ps2_scan = 0x1E, .num = KBD_SCAN_2},
    {.ps2_scan = 0x26, .num = KBD_SCAN_3}, {.ps2_scan = 0x25, .num = KBD_SCAN_4}, {.ps2_scan = 0x2E, .num = KBD_SCAN_5},
    {.ps2_scan = 0x36, .num = KBD_SCAN_6}, {.ps2_scan = 0x3D, .num = KBD_SCAN_7}, {.ps2_scan = 0x3E, .num = KBD_SCAN_8},
    {.ps2_scan = 0x46, .num = KBD_SCAN_9}, {.ps2_scan = 0x45, .num = KBD_SCAN_0}, {.ps2_scan = 0x4E, .num = KBD_SCAN_MINUS},
    {.ps2_scan = 0x55, .num = KBD_SCAN_EQUAL}, {.ps2_scan = 0x66, .num = KBD_SCAN_BACKSPACE},
    //Third row
    {.ps2_scan = 0x0D, .num = KBD_SCAN_TAB}, {.ps2_scan = 0x15, .num = KBD_SCAN_Q}, {.ps2_scan = 0x1D, .num = KBD_SCAN_W},
    {.ps2_scan = 0x24, .num = KBD_SCAN_E}, {.ps2_scan = 0x2D, .num = KBD_SCAN_R}, {.ps2_scan = 0x2C, .num = KBD_SCAN_T},
    {.ps2_scan = 0x35, .num = KBD_SCAN_Y}, {.ps2_scan = 0x3C, .num = KBD_SCAN_U}, {.ps2_scan = 0x43, .num = KBD_SCAN_I},
    {.ps2_scan = 0x44, .num = KBD_SCAN_O}, {.ps2_scan = 0x4D, .num = KBD_SCAN_P}, {.ps2_scan = 0x54, .num = KBD_SCAN_LEFT_SQUARE_BRACKET},
    {.ps2_scan = 0x5B, .num = KBD_SCAN_RIGHT_SQUARE_BRACKET}, {.ps2_scan = 0x5D, .num = KBD_SCAN_BACKSLASH},
    //Fourth row
    {.ps2_scan = 0x58, .num = KBD_SCAN_CAPS_LOCK}, {.ps2_scan = 0x1C, .num = KBD_SCAN_A}, {.ps2_scan = 0x1B, .num = KBD_SCAN_S},
    {.ps2_scan = 0x23, .num = KBD_SCAN_D}, {.ps2_scan = 0x2B, .num = KBD_SCAN_F}, {.ps2_scan = 0x34, .num = KBD_SCAN_G},
    {.ps2_scan = 0x33, .num = KBD_SCAN_H}, {.ps2_scan = 0x3B, .num = KBD_SCAN_J}, {.ps2_scan = 0x42, .num = KBD_SCAN_K},
    {.ps2_scan = 0x4B, .num = KBD_SCAN_L}, {.ps2_scan = 0x4C, .num = KBD_SCAN_SEMICOLON}, {.ps2_scan = 0x52, .num = KBD_SCAN_TICK},
    {.ps2_scan = 0x5A, .num = KBD_SCAN_ENTER},
    //Fifth row
    {.ps2_scan = 0x12, .num = KBD_SCAN_LEFT_SHIFT}, {.ps2_scan = 0x12, .num = KBD_SCAN_Z}, {.ps2_scan = 0x22, .num = KBD_SCAN_X},
    {.ps2_scan = 0x21, .num = KBD_SCAN_C}, {.ps2_scan = 0x2A, .num = KBD_SCAN_V}, {.ps2_scan = 0x32, .num = KBD_SCAN_B},
    {.ps2_scan = 0x31, .num = KBD_SCAN_N}, {.ps2_scan = 0x3A, .num = KBD_SCAN_M}, {.ps2_scan = 0x41, .num = KBD_SCAN_COMMA},
    {.ps2_scan = 0x49, .num = KBD_SCAN_PERIOD}, {.ps2_scan = 0x4A, .num = KBD_SCAN_SLASH}, {.ps2_scan = 0x59, .num = KBD_SCAN_RIGHT_SHIFT},
    //Sixth row
    {.ps2_scan = 0x14, .num = KBD_SCAN_LEFT_CONTROL}, {.ps2_scan = 0x11, .num = KBD_SCAN_LEFT_ALT}, {.ps2_scan = 0x29, .num = KBD_SCAN_SPACE},
    {.ps2_scan = 0xE011, .num = KBD_SCAN_RIGHT_ALT}, {.ps2_scan = 0xE014, .num = KBD_SCAN_RIGHT_CONTROL},
};

//The current scancode being read
uint64_t current_scan;

/*
 * Performs an initialization of the PS/2 keyboard
 */
void ps2_kbd_init(void){
    gfx_verbose_println("Initializing PS/2 keyboard");
    ps2_send(0xF5); //Disable scanning
    ps2_read();
    ps2_send(0xF6); //Set default parameters
    ps2_read();
    ps2_send(0xF0); //Set scancode set 2
    ps2_send(2);
    ps2_read();
    ps2_read();
    ps2_send(0xF4); //Enable scanning
    ps2_read();
    //Reset the scancode
    current_scan = 0;
}

/*
 * Set keyboard "lock" LEDs
 */
void ps2_kbd_leds(uint8_t scroll, uint8_t num, uint8_t caps){
    ps2_send(0xED); //Command: set LEDs
    ps2_send((scroll & 1) | ((num & 1) << 1) | ((caps & 1) << 2));
}

/*
 * Translates the PS/2 scancode number into Neutron key number
 */
kbd_scan_code_t ps2_search_key_map(uint16_t scan){
    //Scan the scancode map
    uint32_t map_entries = sizeof(ps2_kbd_scan_map) / sizeof(*ps2_kbd_scan_map);
    for(uint32_t i = 0; i < map_entries; i++)
        if(ps2_kbd_scan_map[i].ps2_scan == scan)
            return ps2_kbd_scan_map[i].num;
    //If we didn't return, it means there are no keys with this scancode
    return -1;
}

/*
 * One scancode was read
 */
void ps2_kbd_scan_done(void){
    if(current_scan == 0xE11477E1F014F077){
        //PauseBreak key is special, and doesn't generate the "lift" event
        kbd_set_key(KBD_SCAN_PAUSE_BREAK, KBD_KEY_STATE_PRESSED);
        kbd_set_key(KBD_SCAN_PAUSE_BREAK, KBD_KEY_STATE_NOT_PRESSED);
    } else if((current_scan & 0xFFFF00) == 0xE0F000){
        //Extended lift
        uint16_t search_for = 0xE000 | (current_scan & 0xFF);
        kbd_scan_code_t key = ps2_search_key_map(search_for);
        //Ignore the key if it has an invalid scancode
        if(key != -1)
            kbd_set_key(key, KBD_KEY_STATE_NOT_PRESSED);
    } else if((current_scan & 0xFF00) == 0xE000){
        //Extended press
        uint16_t search_for = 0xE000 | (current_scan & 0xFF);
        kbd_scan_code_t key = ps2_search_key_map(search_for);
        if(key != -1)
            kbd_set_key(key, KBD_KEY_STATE_PRESSED);
    } else if((current_scan & 0xFF00) == 0xF000){
        //Normal lift
        uint16_t search_for = current_scan & 0xFF;
        kbd_scan_code_t key = ps2_search_key_map(search_for);
        if(key != -1)
            kbd_set_key(key, KBD_KEY_STATE_NOT_PRESSED);
    } else {
        //Normal press
        uint16_t search_for = current_scan & 0xFF;
        kbd_scan_code_t key = ps2_search_key_map(search_for);
        if(key != -1)
            kbd_set_key(key, KBD_KEY_STATE_PRESSED);
    }
}

/*
 * Parses data available in the keyboard buffer
 */
void ps2_kbd_parse(uint8_t* buf, uint16_t* head, uint16_t* tail){
    //While bytes are available
    while(fifo_av(head, tail)){
        //Read the byte
        uint8_t ps2_byte = fifo_popb(buf, head, tail);
        //Assign it to the lowest one in the current scancode
        current_scan |= ps2_byte;
        //If the current scancode suggests that it consists
        //  of multiple bytes, shift the current scancode
        //  and do not trigger an event
        //Else, trigger an event and clear the scancode
        if(current_scan == 0xE0 ||
           current_scan == 0xF0 ||
           current_scan == 0xE0F0 ||
           //Long PauseBreak code
           current_scan == 0xE1 ||
           current_scan == 0xE114 ||
           current_scan == 0xE11477 ||
           current_scan == 0xE11477E1 ||
           current_scan == 0xE11477E1F0 ||
           current_scan == 0xE11477E1F014 ||
           current_scan == 0xE11477E1F014F0){
            current_scan <<= 8;
        } else {
            ps2_kbd_scan_done();
            current_scan = 0;
        }
    }
}