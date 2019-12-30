//Neutron project
//Terminal - one of the built-in apps

#include "./term.h"
#include "../app.h"
#include "../../gui/gui.h"
#include "../../drivers/gfx.h"
#include "../../drivers/human_io/kbd.h"

//Default terminal color lookup table
color32_t term_color_lut[16] = {
    COLOR32(255, 0, 0, 0),
    COLOR32(255, 0, 0, 170),
    COLOR32(255, 0, 170, 0),
    COLOR32(255, 0, 170, 170),
    COLOR32(255, 170, 0, 0),
    COLOR32(255, 170, 0, 170),
    COLOR32(255, 170, 85, 0),
    COLOR32(255, 170, 170, 170),
    COLOR32(255, 85, 85, 85),
    COLOR32(255, 85, 85, 255),
    COLOR32(255, 85, 255, 85),
    COLOR32(255, 85, 255, 255),
    COLOR32(255, 255, 85, 85),
    COLOR32(255, 255, 85, 255),
    COLOR32(255, 255, 255, 85),
    COLOR32(255, 255, 255, 255)
};

//The array of terminals
term_t terms[32];
//The ID that will be assigned to the next terminal
uint32_t next_term_id = 0;

/*
 * Process terminal input
 */
void term_process_input(term_t* term){
    char* input = term->input_buf;
    if(input[0] == 0){
        //Do nothing
    } else if(strcmp(input, "clear") == 0){
        //Clear the terminal
        for(uint32_t o = 0; o < term->size.x * term->size.y; o++)
            term->buf[o] = (term_char_t){.character = ' ', .fcolor = 15, .bcolor = 0};
        //Reset the cursor position
        term->cursor = (term_coord_t){.x = 2, .y = 0};
    } else if(memcmp(input, "echo ", 5) == 0){
        //Split the string
        char buf[100];
        buf[0] = 0;
        memcpy(buf, input + 5, strlen(input) - 5 + 1);
        //Print it
        term_puts(term, buf, 15, 0);
        term_puts(term, "\n", 15, 0);
    } else if(memcmp(input, "app ", 4) == 0){
        if(strcmp(input, "app list") == 0){
            //Scan through the app list
            for(uint32_t id = 0; id < app_count(); id++){
                //Retrieve app information
                app_t* app = app_get_id(id);
                //Print app information
                char temp[10];
                term_puts(term, "\"", 15, 0);
                term_puts(term, app->name, 15, 0);
                term_puts(term, "\" v", 15, 0);
                term_puts(term, sprintu(temp, app->ver_major, 1), 15, 0);
                term_puts(term, ".", 15, 0);
                term_puts(term, sprintu(temp, app->ver_minor, 1), 15, 0);
                term_puts(term, ".", 15, 0);
                term_puts(term, sprintu(temp, app->ver_patch, 1), 15, 0);
                term_puts(term, "\n", 15, 0);
            }
        } else if(memcmp(input, "app start ", 10) == 0){
            //Retrieve app information
            app_t* app = app_get_name(input + 10);
            //If the application had been found
            if(app != NULL)
                app->entry_point(); //Call its entry point
            else
                term_puts(term, "Application not found", 0xC, 0);
        } else {
            term_puts(term, "Usage:\n    app list\nprints the list of available applications\n    app start <name>\ncalls the entry point for application with name <name>\n", 15, 0);
        }
    } else {
        term_puts(term, "Command not found\n", 0xC, 0);
    }
}

/*
 * Terminal Window event handler
 */
void term_win_event(ui_event_args_t* args){
    //React to "render end" event by drawing the terminal
    if(args->type == GUI_EVENT_RENDER_END){
        //Go through the terminal list and find one that
        //  corresponds to the window that sent the event
        for(uint32_t i = 0; i < 32; i++){
            if(terms[i].win == args->win){
                term_t* term = &terms[i];
                if(term->cursor.x == 0 && term->cursor.y == 0)
                    term->cursor = (term_coord_t){.x = 1, .y = 0};
                //Draw the terminal character by character
                p2d_t window_offset = (p2d_t){.x = ((window_t*)(args->win))->position.x + 7,
                                              .y = ((window_t*)(args->win))->position.y + 11};
                uint32_t offset = 0;
                for(uint32_t y = 0; y < term->size.y; y++){
                    for(uint32_t x = 0; x < term->size.x; x++){
                        term_char_t term_char = term->buf[(y * term->size.x) + x];
                        //Draw the character
                        gfx_putch((p2d_t){.x = window_offset.x + (x * 6), .y = window_offset.y + (y * 8)},
                            term_color_lut[term_char.fcolor], term_color_lut[term_char.bcolor], term->buf[offset].character);
                        //Move on to the next character
                        offset++;
                    }
                }
                //Draww^_^ the cursor
                if(term->cursor_visible){
                    p2d_t pos_on_screen = (p2d_t){.x = window_offset.x + (term->cursor.x * 6) - 6, .y = window_offset.y + (term->cursor.y * 8) + 7};
                    uint8_t anim_prog = term->cur_anim_prog;
                    gfx_draw_hor_line(pos_on_screen, ((anim_prog > 20) ? 1 : 0) * 5, term_color_lut[15]);
                    if(++term->cur_anim_prog > 40)
                        term->cur_anim_prog = 0;
                }
                break;
            }
        }
    }
    //React to the "keyboard" event
    else if (args->type == GUI_EVENT_KEYBOARD){
        //Fetch the event as extra event data
        kbd_event_t* event = (kbd_event_t*)args->extra_data;
        //Check if event type is "character"
        if(event->state == KBD_KEY_STATE_CHAR){
            //Go through the terminal list and find one that
            //  corresponds to the window that sent the event
            for(uint32_t i = 0; i < 32; i++){
                if(terms[i].win == args->win){
                    term_t* term = &terms[i];
                    //If the character is "enter", process the entered command
                    if(event->character == '\n'){
                        term->input_buf[term->input_buf_pos] = 0;
                        term_puts(term, "\n", 0x7, 0);
                        term_process_input(term);
                        term_puts(term, ">", 0x7, 0);
                        term->input_buf_pos = 0;
                    }
                    //If the character is "backspace" and the cursor isn't at the start, delete the last character
                    else if(event->character == 8){
                        if(term->input_buf_pos > 0){
                            //Remove the character from the terminal
                            uint32_t offset = (term->cursor.y * term->size.x) + term->cursor.x - 1;
                            term->buf[offset].character = ' ';
                            term->cursor.y = offset / term->size.x;
                            term->cursor.x = offset % term->size.x;
                            //Remove the character from the buffer
                            term->input_buf_pos--;
                        }
                    }
                    //Print the character in any other case
                    else {
                        char temp[2];
                        temp[0] = event->character;
                        temp[1] = 0;
                        term_puts(term, temp, 15, 0);
                        //Put the character into the input buffer
                        term->input_buf[term->input_buf_pos++] = event->character;
                    }
                    break;
                }
            }
        }
    }
}

/*
 * Puts a string at the cursor position and advances it by that much
 */
void term_puts(term_t* term, char* str, uint8_t fcolor, uint8_t bcolor){
    //Calculate the offset
    uint32_t offset = (term->cursor.y * term->size.x) + term->cursor.x;
    //Fetch a character from the string
    char c;
    while(c = *(str++)){
        switch(c){
            case '\n':
                offset -= offset % term->size.x;
                offset += term->size.x;
                break;
            default:
                //Normal printing
                term->buf[offset++] = (term_char_t){.character = c, .fcolor = fcolor, .bcolor = bcolor};
                break;
        }
        //If the cursor is out of bounds
        if(offset >= term->size.x * term->size.y){
            //Shift the text up one line
            for(uint32_t x = 0; x < term->size.x; x++){
                for(uint32_t y = 1; y < term->size.y; y++){
                    uint32_t offs = (y * term->size.x) + x;
                    term->buf[offs - term->size.x] = term->buf[offs];
                }
            }
            //Set the new cursor position
            offset -= term->size.x;
            //Clear the new line
            for(uint32_t x = 0; x < term->size.x; x++){
                term->buf[offset - (offset % term->size.x) + x] = (term_char_t){.character = ' ', .fcolor = fcolor, .bcolor = bcolor};
            }
        }
    }
    //Set the new cursor coordinates
    term->cursor.y = offset / term->size.x;
    term->cursor.x = offset % term->size.x;
}

/*
 * Create a terminal with default parameters
 */
void create_terminal(void){
    term_t term;
    //The default size is 60x40
    term.size = (term_coord_t){.x = 60, .y = 40};
    //Allocate some buffers
    term.buf = (term_char_t*)calloc(term.size.x * term.size.y, sizeof(term_char_t));
    //Fill the buffer with spaces
    for(uint32_t y = 0; y < term.size.y; y++)
        for(uint32_t x = 0; x < term.size.x; x++)
            term.buf[(y * term.size.x) + x] = (term_char_t){.character = ' ', .bcolor = 0, .fcolor = 15};
    term.input_buf = (char*)malloc(128 * sizeof(char));
    //Assign an ID
    term.id = next_term_id++;
    //Reset some properties to their default values
    term.cursor = (term_coord_t){.x = 0, .y = 0};
    term.cursor_visible = 1;
    term.cur_anim_prog = 0;
    term.input_buf_pos = 0;
    
    //Create and assign the window
    char temp[25];
    temp[0] = 0;
    char temp2[10];
    strcat(temp, "Terminal #");
    strcat(temp, sprintu(temp2, term.id, 1));
    p2d_t win_size = (p2d_t){.x = term.size.x * 6 + 1, .y = term.size.y * 8 + 11};
    window_t* win =
    gui_create_window(temp, term_icon_8, GUI_WIN_FLAGS_STANDARD, (p2d_t){.x = (gfx_res_x() - win_size.x) / 2,
                                                                         .y = (gfx_res_y() - win_size.y) / 2},
        win_size, term_win_event);
    term.win = win;
    //Assign the terminal to the list
    terms[term.id] = term;

    term_puts(&term, "> ", 0x7, 0);
}

/*
 * The entry point for the terminal
 */
void term_entry(void){
    //Create a terminal
    create_terminal();
}