#pragma once

#include "nlib.h"

#include "gfx.h"

//Structure definitions

//A window
typedef struct {
    char title[256];
    p2d_t pos;
    p2d_t size;
    ll_t* controls;

    uint8_t needs_rendering;
    raw_img_t buf;
} window_t;

//Function prototypes

window_t* win_create (char* title, p2d_t size);
void      win_render (window_t* win);
void      wins_draw  (void);