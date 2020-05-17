//Neutron Project
//General keyboard driver

//Particular interfaces (PS/2, USB)
//  are handled by their respective drivers

#include "./kbd.h"
#include "../../stdlib.h"

void krnl_dump(void);

/*
 * Neutron key number to character map
 */
static kbd_scan_char_map_t kbd_scan_char_map[] = {
    {KBD_SCAN_A, 'a', 'A'}, {KBD_SCAN_B, 'b', 'B'}, {KBD_SCAN_C, 'c', 'C'}, {KBD_SCAN_D, 'd', 'D'}, {KBD_SCAN_E, 'e', 'E'},
    {KBD_SCAN_F, 'f', 'F'}, {KBD_SCAN_G, 'g', 'G'}, {KBD_SCAN_H, 'h', 'H'}, {KBD_SCAN_I, 'i', 'I'}, {KBD_SCAN_J, 'j', 'J'},
    {KBD_SCAN_K, 'k', 'K'}, {KBD_SCAN_L, 'l', 'L'}, {KBD_SCAN_M, 'm', 'M'}, {KBD_SCAN_N, 'n', 'N'}, {KBD_SCAN_O, 'o', 'O'},
    {KBD_SCAN_P, 'p', 'P'}, {KBD_SCAN_Q, 'q', 'Q'}, {KBD_SCAN_R, 'r', 'R'}, {KBD_SCAN_S, 's', 'S'}, {KBD_SCAN_T, 't', 'T'},
    {KBD_SCAN_U, 'u', 'U'}, {KBD_SCAN_V, 'v', 'V'}, {KBD_SCAN_W, 'w', 'W'}, {KBD_SCAN_X, 'x', 'X'}, {KBD_SCAN_Y, 'y', 'Y'},
    {KBD_SCAN_Z, 'z', 'Z'},

    {KBD_SCAN_1, '1', '!'}, {KBD_SCAN_2, '2', '@'}, {KBD_SCAN_3, '3', '#'}, {KBD_SCAN_4, '4', '$'}, {KBD_SCAN_5, '5', '%'},
    {KBD_SCAN_6, '6', '^'}, {KBD_SCAN_7, '7', '&'}, {KBD_SCAN_8, '8', '*'}, {KBD_SCAN_9, '9', '('}, {KBD_SCAN_0, '0', ')'},
    {KBD_SCAN_LEFT_SQUARE_BRACKET, '[', '{'}, {KBD_SCAN_RIGHT_SQUARE_BRACKET, ']', '}'}, {KBD_SCAN_BACKSLASH, '\\', '|'},
    {KBD_SCAN_SEMICOLON, ':', ';'}, {KBD_SCAN_TICK, '\'', '"'}, {KBD_SCAN_ENTER, '\n', '\n'}, {KBD_SCAN_COMMA, ',', '<'},
    {KBD_SCAN_PERIOD, '.', '>'}, {KBD_SCAN_SLASH, '/', '?'}, {KBD_SCAN_SPACE, ' ', ' '}, {KBD_SCAN_BACKSPACE, 8, 8},
    {KBD_SCAN_MINUS, '-', '_'}, {KBD_SCAN_EQUAL, '=', '+'}
};

//An array holding the state of all keys
uint8_t key_state[KBD_SCAN_CODE_COUNT];
kbd_event_t event_queue[KBD_EVENT_QUEUE_DEPTH];
uint16_t event_queue_tail = 0;
uint16_t event_queue_head = 0;

/*
 * Returns a character corresponding to the scancode
 */
char kbd_find_char(kbd_scan_code_t scan, uint8_t shift){
    //Calculate the amount of entries in the map
    uint32_t map_entries = sizeof(kbd_scan_char_map) / sizeof(*kbd_scan_char_map);
    //Scan through the map
    for(uint32_t i = 0; i < map_entries; i++)
        if(kbd_scan_char_map[i].scan == scan)
            return shift ? kbd_scan_char_map[i].c_s : kbd_scan_char_map[i].c_n; //Return the normal or the shifted char
    //If we didn't return, there's no such character
    return 0;
}

/*
 * Sets a state of a key and adds the event to the event queue
 */
void kbd_set_key(kbd_scan_code_t scan_code, uint8_t state){
    //Set the key state
    key_state[scan_code] = state;
    //If this is a Ctrl+Alt+Del sequence
    if((key_state[KBD_SCAN_LEFT_CONTROL] || key_state[KBD_SCAN_RIGHT_CONTROL]) &&
       (key_state[KBD_SCAN_LEFT_ALT] || key_state[KBD_SCAN_RIGHT_ALT]) &&
        key_state[KBD_SCAN_DELETE]){
        //Invoke kernel dump
        krnl_dump();
        //Abort
        abort();
    }
    //Construct an event
    kbd_event_t event = (kbd_event_t){.scan_code = scan_code, .state = state};
    //Add it to the event queue at its head;
    //  increment the head position
    event_queue[event_queue_head++] = event;
    //Try to get the character of this scancode
    if(state == KBD_KEY_STATE_PRESSED) {
        char c = kbd_find_char(scan_code, key_state[KBD_SCAN_LEFT_SHIFT] || key_state[KBD_SCAN_RIGHT_SHIFT]);
        if(c != 0){
            //Construct and send the character event
            event = (kbd_event_t){.character = c, .state = KBD_KEY_STATE_CHAR};
            event_queue[event_queue_head++] = event;
        }
    }
}

/*
 * Tries to get a keyboard event from the queue. Returns 1 on success
 */
uint8_t kbd_pop_event(kbd_event_t* event){
    //If there are no events available, return 0
    if(event_queue_tail >= event_queue_head)
        return 0;
    
    //Get the element at the tail
    if(event != NULL)
        *event = event_queue[event_queue_tail];
    //Increment the tail
    event_queue_tail++;
    //If the tail excceds the head in position,
    //  reset both of them to zero
    if(event_queue_tail >= event_queue_head){
        event_queue_head = 0;
        event_queue_tail = 0;
    }
    //Return success
    return 1;
}