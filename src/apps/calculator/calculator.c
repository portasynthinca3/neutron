//Neutron Project
//Calculator - one of the built-in apps

#include "./calculator.h"
#include "../app.h"
#include "../../gui/gui.h"
#include "../../drivers/gfx.h"

color32_t calculator_display_bg = COLOR32(255, 128, 128, 128);
color32_t calculator_button_bg = COLOR32(255, 128, 128, 128);
color32_t calculator_button_unused_bg = COLOR32(255, 64, 64, 64);
color32_t calculator_equal_bg = COLOR32(255, 255, 128, 0);

control_t* calculator_label_result;

uint8_t calculator_state;
uint8_t calculator_operation;
int64_t calculator_x;
int64_t calculator_y;

/*
 * Display the result on the screen
 */
void calculator_display_result(char* result){
    //Calculate the bounds of the text
    p2d_t bounds = gfx_text_bounds(result);
    //Set the text
    memcpy(((control_ext_label_t*)calculator_label_result->extended)->text, result, strlen(result) + 1);
    //Set the position
    calculator_label_result->position.x = 86 - bounds.x;
    //Set the size
    calculator_label_result->size = bounds;
}

/*
 * Any button was pressed on the calculator
 */
void calculator_pressed(ui_event_args_t* args){
    if(args->type == GUI_EVENT_CLICK){
        //Calculate the click position relative to the button
        p2d_t click_pos = (p2d_t){.x = args->mouse_pos.x - ((window_t*)args->win)->position.x - ((control_t*)args->control)->position.x - 1,
                                  .y = args->mouse_pos.y - ((window_t*)args->win)->position.y - ((control_t*)args->control)->position.y - 13};
        //If the click position is sane
        if(click_pos.x <= 87 && click_pos.y <= 87){
            //Calculate X and Y button index
            uint8_t btn_x = click_pos.x / 22;
            uint8_t btn_y = click_pos.y / 22;

            //3rd column = operators
            if(btn_x == 3){
                calculator_operation = btn_y;
                //Change the state
                calculator_state = CALCULATOR_STATE_ENTERING_Y;
            }
            //2nd column 3rd row = "="
            else if(btn_x == 2 && btn_y == 3){
                //Store the result of operation between X and Y in Y
                if(calculator_operation == CALCULATOR_OPERATION_ADDITION)
                    calculator_x += calculator_y;
                else if(calculator_operation == CALCULATOR_OPERATION_SUBTRACTION)
                    calculator_x -= calculator_y;
                else if(calculator_operation == CALCULATOR_OPERATION_MULTIPLICATION)
                    calculator_x *= calculator_y;
                else if(calculator_operation == CALCULATOR_OPERATION_DIVISION)
                    calculator_x /= calculator_y;
                //Reset the state to "entering X"
                calculator_state = CALCULATOR_STATE_ENTERING_X;
                //Reset Y
                calculator_y = 0;
                //Display the X contents
                char temp[20];
                calculator_display_result(sprintu(temp, calculator_x, 1));
            }
            //1st column 3rd row = reset
            else if(btn_x == 1 && btn_y == 3){
                calculator_state = CALCULATOR_STATE_ENTERING_X;
                calculator_x = 0;
                calculator_y = 0;
                calculator_display_result("0");
            }
            else {
                //Decode the digit
                uint8_t digit = 0;
                if(btn_y == 0)
                    digit = btn_x + 7;
                else if(btn_y == 1)
                    digit = btn_x + 4;
                else if(btn_y == 2)
                    digit = btn_x + 1;
                else if(btn_y == 0)
                    digit = 0;
                //Depending on the state, either append that digit to
                //  and display contents of the X or Y register
                char temp[20];
                if(calculator_state == CALCULATOR_STATE_ENTERING_X){
                    calculator_x *= 10;
                    calculator_x += digit;
                    calculator_display_result(sprintu(temp, calculator_x, 1));
                } else {
                    calculator_y *= 10;
                    calculator_y += digit;
                    calculator_display_result(sprintu(temp, calculator_y, 1));
                }
            }
        }
    }
}

/*
 * The entry point for the calculator
 */
void calculator_entry(void){
    //Create the window
    window_t* window = 
    gui_create_window("Calc", calculator_icon_8, GUI_WIN_FLAGS_STANDARD, (p2d_t){.x = 100, .y = 100}, (p2d_t){.x = 90, .y = 127}, NULL);
    //Create a progress bar mimicking the display
    gui_create_progress_bar(window, (p2d_t){.x = 1, .y = 0}, (p2d_t){86, 25}, calculator_display_bg, COLOR32(0, 0, 0, 0),
        COLOR32(255, 0, 0, 0), 100, 0, NULL);
    //Create the result label
    calculator_label_result = 
    gui_create_label(window, (p2d_t){.x = 0, .y = 9}, (p2d_t){.x = 0, .y = 0}, "                   ",
        COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL);
    calculator_display_result("0");
    //Create one big button that really handles the events
    gui_create_button(window, (p2d_t){.x = 1, .y = 27}, (p2d_t){.x = 87, .y = 87}, "",
        COLOR32(0, 0, 0, 0), gui_get_color_scheme()->win_bg, gui_get_color_scheme()->win_bg, gui_get_color_scheme()->win_bg, calculator_pressed);
    
    
    //Create the "7" button
    gui_create_button(window, (p2d_t){.x = 1, .y = 27}, (p2d_t){.x = 20, .y = 20}, "7",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "8" button
    gui_create_button(window, (p2d_t){.x = 23, .y = 27}, (p2d_t){.x = 20, .y = 20}, "8",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "9" button
    gui_create_button(window, (p2d_t){.x = 45, .y = 27}, (p2d_t){.x = 20, .y = 20}, "9",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "+" button
    gui_create_button(window, (p2d_t){.x = 67, .y = 27}, (p2d_t){.x = 20, .y = 20}, "+",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);

    //Create the "4" button
    gui_create_button(window, (p2d_t){.x = 1, .y = 49}, (p2d_t){.x = 20, .y = 20}, "4",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "5" button
    gui_create_button(window, (p2d_t){.x = 23, .y = 49}, (p2d_t){.x = 20, .y = 20}, "5",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "6" button
    gui_create_button(window, (p2d_t){.x = 45, .y = 49}, (p2d_t){.x = 20, .y = 20}, "6",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "-" button
    gui_create_button(window, (p2d_t){.x = 67, .y = 49}, (p2d_t){.x = 20, .y = 20}, "-",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);

    //Create the "1" button
    gui_create_button(window, (p2d_t){.x = 1, .y = 71}, (p2d_t){.x = 20, .y = 20}, "1",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "2" button
    gui_create_button(window, (p2d_t){.x = 23, .y = 71}, (p2d_t){.x = 20, .y = 20}, "2",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "3" button
    gui_create_button(window, (p2d_t){.x = 45, .y = 71}, (p2d_t){.x = 20, .y = 20}, "3",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "*" button
    gui_create_button(window, (p2d_t){.x = 67, .y = 71}, (p2d_t){.x = 20, .y = 20}, "*",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);

    //Create the "0" button
    gui_create_button(window, (p2d_t){.x = 1, .y = 93}, (p2d_t){.x = 20, .y = 20}, "0",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "AC" button
    gui_create_button(window, (p2d_t){.x = 23, .y = 93}, (p2d_t){.x = 20, .y = 20}, "AC",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "=" button
    gui_create_button(window, (p2d_t){.x = 45, .y = 93}, (p2d_t){.x = 20, .y = 20}, "=",
        COLOR32(255, 255, 255, 255), calculator_equal_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
    //Create the "/" button
    gui_create_button(window, (p2d_t){.x = 67, .y = 93}, (p2d_t){.x = 20, .y = 20}, "/",
        COLOR32(255, 255, 255, 255), calculator_button_bg, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0), NULL);
}