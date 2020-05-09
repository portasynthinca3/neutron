#pragma once

//Definitions

//Mouse buttons
#define MOUSE_BTN_LEFT              1
#define MOUSE_BTN_MIDDLE            2
#define MOUSE_BTN_RIGHT             4

//Structures

typedef struct{
    int32_t rel_x,
            rel_y,
            rel_z;
    uint8_t buttons;
} mouse_evt_t;

//Function prototypes

void ps2_set_mouse_cb (void(*cb)(mouse_evt_t));
void ps2_init         (void);
void ps2_check        (void);