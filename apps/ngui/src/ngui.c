//Neutron project
//Standard GUI manager - main file

#include "nlib.h"
#include "app_desc.h"

#include "ngui.h"
#include "gfx.h"

//The current theme
theme_t theme;

/*
 * Parses the ARGB color
 */
color32_t parse_color(char* str){
    color32_t c;
    
    char* next = str;

    *strchr(next, ',') = 0;
    c.a = atoi(next);
    next += strlen(next) + 1;

    *strchr(next, ',') = 0;
    c.r = atoi(next);
    next += strlen(next) + 1;

    *strchr(next, ',') = 0;
    c.g = atoi(next);
    next += strlen(next) + 1;

    c.b = atoi(next);

    return c;
}

/*
 * Loads the theme file
 */
void load_theme(char* path){
    //Prepare a buffer
    char line[512];
    //Open the file
    FILE* fp = fopen(path, "r");
    if(fp == NULL)
        return;
    //Scan the config file
    while(true){
        //Read one line
        fgets(line, 512, fp);
        //If it's empty, EOF has been reached
        if(strlen(line) == 0)
            break;
        //Skip the line if it's empty or starts with a hash
        if(line[0] == '\n' || line[0] == '#')
            continue;
        //Truncate the trailing \n
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;
        //Get key and value
        char key[256];
        char val[256];
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        char* eq_occur = strchr(line, '=');
        memcpy(key, line, eq_occur - line);
        strcpy(val, eq_occur + 1);

        //Parse the config line
        if(memcmp(key, "desktop.", 8) == 0){
            char* desk = key + 8;
            if(strcmp(desk, "color") == 0)
                theme.desk.color = parse_color(val);
        } else if(memcmp(key, "panel.", 6) == 0){ //Panel settings
            char* panel = key + 6;
            if(strcmp(panel, "margins") == 0)
                theme.panel.margins = atoi(val);
            else if(strcmp(panel, "height") == 0)
                theme.panel.height = atoi(val);
            else if(strcmp(panel, "color") == 0)
                theme.panel.color = parse_color(val);
        }
    }
    fclose(fp);
}

/*
 * Draws the panel
 */
void draw_panel(void){
    //Calculate panel size and position
    p2d_t size = P2D(gfx_res().x - (2 * theme.panel.margins), theme.panel.height);
    p2d_t pos  = P2D(theme.panel.margins, gfx_res().y - theme.panel.margins - size.y);
    //Draw the rectangle
    gfx_draw_filled_rect(pos, size, theme.panel.color);
}

/*
 * Entry point
 */
void main(void* args){
    //Welcome everybody!
    _km_write(__APP_SHORT_NAME, "ngui is starting");
    //Initialize the graphics library
    gfx_init();
    //Load the GUI config file
    load_theme("ngui.cfg");

    //In an endless loop
    while(1){
        //Fill the buffer with background color
        gfx_fill(theme.desk.color);
        //Draw the panel
        draw_panel();
        //Update the framebuffer
        gfx_flip();
    }
}