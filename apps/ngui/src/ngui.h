#pragma once

#include "nlib.h"
#include "gfx.h"

//Structure definitions

//Desktop properties structure
typedef struct {
    color32_t color;
} desk_prop_t;

//Panel properties structure
typedef struct {
    uint32_t margins,
             height;
    color32_t color;
} panel_prop_t;

//Theme structure
typedef struct {
    desk_prop_t  desk;
    panel_prop_t panel;
} theme_t;

//Function prototypes

void load_theme (char* path);
void draw_panel (void);
void main       (void* args);