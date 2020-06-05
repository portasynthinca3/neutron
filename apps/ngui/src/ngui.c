//Neutron project
//Standard GUI manager - main file

#include "nlib.h"
#include "app_desc.h"

#include "ngui.h"
#include "gfx.h"
#include "ps2.h"

//The current theme
theme_t theme;
//Current absolute cursor position
p2d_t cursor_pos;
//CPU frequency (cycles/ms)
uint64_t cpu_fq;

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
        if(memcmp(key, "cur.", 4) == 0){ //Cursor settings
            char* cur = key + 4;
            if(strcmp(cur, "image") == 0){
                strcpy(theme.cur.image, val);
                //Load the image file
                FILE* img = fopen(val, "rb");
                if(img != NULL){
                    theme.cur.img_width = fgetc(img);
                    theme.cur.img_height = fgetc(img);
                    uint64_t img_data_sz = theme.cur.img_width * theme.cur.img_height * 4;
                    theme.cur.img_data = (uint8_t*)malloc(img_data_sz);
                    fread(theme.cur.img_data, 1, img_data_sz, img);
                    fclose(img);
                }
            }
        } else if(memcmp(key, "desktop.", 8) == 0){ //Desktop settings
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
            else if(strcmp(panel, "bar_height") == 0)
                theme.panel.bar_height = atoi(val);
            else if(strcmp(panel, "movement_time") == 0)
                theme.panel.movement_time = cpu_fq * atol(val);
            else if(strcmp(panel, "hold_time") == 0)
                theme.panel.hold_time = cpu_fq * atol(val);
        }
    }
    fclose(fp);
}

/*
 * Draws the panel
 */
void draw_panel(void){
    //Draw the top bar
    gfx_draw_filled_rect(P2D(0, 0), P2D(gfx_res().x, theme.panel.bar_height), theme.panel.color);
    //Calculate overall size and position
    int32_t panel_offs = 0;
    if(theme.panel.state == 3){
        if(cursor_pos.y >= gfx_res().y - 10){
            theme.panel.state = 1;
            theme.panel.last_state_ch = rdtsc();
        } else {
            return;
        }
    } else if(theme.panel.state == 0){
        if(cursor_pos.y >= gfx_res().y - (theme.panel.height + theme.panel.margins)){
            theme.panel.state = 0;
            theme.panel.last_state_ch = rdtsc();
        }
        if(rdtsc() - theme.panel.last_state_ch >= theme.panel.hold_time){
            theme.panel.state = 2;
            theme.panel.last_state_ch = rdtsc();
        }
    } else if(theme.panel.state == 1){
        panel_offs = theme.panel.height + theme.panel.margins - 
            (uint64_t)(theme.panel.height + theme.panel.margins) *
            (rdtsc() - theme.panel.last_state_ch) / theme.panel.movement_time;
        if(panel_offs <= 0){
            theme.panel.state = 0;
            theme.panel.last_state_ch = rdtsc();
        }
    } else if(theme.panel.state == 2){
        panel_offs = (uint64_t)(theme.panel.height + theme.panel.margins) *
            (rdtsc() - theme.panel.last_state_ch) / theme.panel.movement_time;
        if(panel_offs > theme.panel.height + theme.panel.margins){
            theme.panel.state = 3;
            theme.panel.last_state_ch = rdtsc();
        }
    }
    p2d_t size = P2D(gfx_res().x - (2 * theme.panel.margins), theme.panel.height);
    p2d_t pos  = P2D(theme.panel.margins, gfx_res().y - theme.panel.margins - size.y + panel_offs);
    //Draw the left square
    gfx_draw_round_rect(pos, P2D(size.y, size.y), 4, theme.panel.color);
    //Draw the main panel
    gfx_draw_round_rect(P2D(pos.x + size.y + theme.panel.margins, pos.y),
                        P2D(size.x - (size.y + theme.panel.margins), size.y), 4, theme.panel.color);
}

/*
 * Bit Scan and Reverse
 */
uint64_t _bsr(uint64_t n){
    uint64_t val;
    asm("bsr %1, %0;" : "=r"(val) : "r"(n));
    return val;
}

/*
 * PS/2 mouse event handler
 */
void mouse_evt(mouse_evt_t evt){
    //_bsr is used as a fast approximation of log base 2
    int velocity = _bsr((abs(evt.rel_x) + abs(evt.rel_y)) / 50);
    if(velocity < 1)
        velocity = 1;
    //Scale by the velocity
    cursor_pos.x += evt.rel_x * velocity;
    cursor_pos.y += evt.rel_y * velocity;
    //Limit the cursor position to screen borders
    if(cursor_pos.x < 0)
        cursor_pos.x = 0;
    if(cursor_pos.y < 0)
        cursor_pos.y = 0;
    if(cursor_pos.x >= gfx_res().x)
        cursor_pos.x = gfx_res().x - 1;
    if(cursor_pos.y >= gfx_res().y)
        cursor_pos.y = gfx_res().y - 1;
}

/*
 * Gets CPU frequency from /sys/cpufq
 */
void get_cpu_fq(void){
    FILE* f = fopen("/sys/cpufq", "r");
    char buf[64];
    fgets(buf, 64, f);
    cpu_fq = atol(buf) / 1000;
    fclose(f);
}

/*
 * Entry point
 */
void main(void* args){
    //Welcome everybody!
    _km_write(__APP_SHORT_NAME, "ngui is starting");
    //Initialize stuff
    gfx_init();
    cursor_pos = P2D(gfx_res().x / 2, gfx_res().y / 2);
    ps2_init();
    ps2_set_mouse_cb(mouse_evt);
    get_cpu_fq();
    //Load the GUI config file
    load_theme("ngui.cfg");

    //In an endless loop
    while(1){
        //Update the PS/2 state
        ps2_check();
        //Draw everything
        gfx_fill(theme.desk.color);
        draw_panel();
        gfx_draw_raw(cursor_pos, theme.cur.img_data, P2D(theme.cur.img_width, theme.cur.img_height));
        //Update the framebuffer
        gfx_flip();
    }
}