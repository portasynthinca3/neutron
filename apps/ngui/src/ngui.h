#pragma once

#include "nlib.h"
#include "gfx.h"
#include "ps2.h"

//Structure definitions

//Cursor properties
typedef struct {
    char image[128];
    uint64_t img_width,
             img_height;
    uint8_t* img_data;
} cur_prop_t;

//Desktop properties
typedef struct {
    color32_t color;
} desk_prop_t;

//Panel properties
typedef struct {
    int32_t margins,
            height,
            bar_height;
    uint64_t movement_time,
             hold_time;
    color32_t color;
    uint8_t* icon_data;
    //Animation state
    uint64_t last_state_ch;
    uint8_t  state; //0=waiting to hide, 1=moving up, 2=moving down, 3=waiting to show
} panel_prop_t;

//Theme structure
typedef struct {
    cur_prop_t   cur;
    desk_prop_t  desk;
    panel_prop_t panel;
} theme_t;

//Function prototypes

void      time_read   (char* buf);
color32_t parse_color (char* str);
void      load_theme  (char* path);
void      draw_panel  (void);
void      mouse_evt   (mouse_evt_t evt);
void      get_cpu_fq  (void);
void      main        (void* args);