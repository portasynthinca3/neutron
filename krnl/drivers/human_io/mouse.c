//Neutron Project
//General mouse driver

//Particular interfaces (PS/2, USB)
//  are handled by their respective drivers

#include "./mouse.h"
#include "../../stdlib.h"
#include "../gfx.h"

//Horizontal mouse coordinate
int32_t pos_x;
//Vertical mouse coordinate
int32_t pos_y;
//Delta horizontal movement this frame
int32_t delta_x;
//Delta vertical movement this frame
int32_t delta_y;
//Is the left button pressed
uint8_t left_btn;
//Is the right button pressed
uint8_t right_btn;

/*
 * Get absolute mouse coordinates
 */
void mouse_abs(int32_t* x, int32_t* y){
    //Write the absolute position
    *x = pos_x;
    *y = pos_y;
}

/*
 * Get delta mouse coordinates
 */
void mouse_delta(int32_t* x, int32_t* y){
    //Write the delta
    *x = delta_x;
    *y = delta_y;
}

/*
 * Get mouse buttons
 */
void mouse_buttons(uint8_t* left, uint8_t* right){
    //Write the data
    *left = left_btn;
    *right = right_btn;
}

/*
 * Specific interface drivers send data using this function
 */
void mouse_callback(int32_t dx, int32_t dy, uint8_t l, uint8_t r){
    //Add the delta (there may be several mouses sending data)
    delta_x += dx;
    delta_y += dy;
    //Add the mouse left/right buttons
    left_btn = l;
    right_btn = r;
}

/*
 * Signal an end of the frame
 */
void mouse_frame_end(void){
    //Add the delta to the absolute position
    pos_x += delta_x;
    pos_y += delta_y;
    //Constrain the absolute coordinates
    if(pos_x < 0)
        pos_x = 0;
    if(pos_y < 0)
        pos_y = 0;
    if(pos_x > gfx_res_x() - 1)
        pos_x = gfx_res_x() - 1;
    if(pos_y > gfx_res_y() - 1)
        pos_y = gfx_res_y() - 1;
    //Reset the delta
    delta_x = 0;
    delta_y = 0;
}