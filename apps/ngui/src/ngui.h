#pragma once

#include "nlib.h"
#include "gfx.h"

//Structure definitions

//Cursor properties structure
typedef struct {
    char image[128];
    uint64_t img_width,
             img_height;
    uint8_t* img_data;
} cur_prop_t;

//Desktop properties structure
typedef struct {
    color32_t color;
} desk_prop_t;

//Panel properties structure
typedef struct {
    uint32_t margins,
             height,
             bar_height;
    color32_t color;
} panel_prop_t;

//Theme structure
typedef struct {
    cur_prop_t   cur;
    desk_prop_t  desk;
    panel_prop_t panel;
} theme_t;

//Function prototypes

void load_theme (char* path);
void draw_panel (void);
void main       (void* args);