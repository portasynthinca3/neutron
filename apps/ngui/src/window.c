//Neutron project
//Standard GUI manager - window controller

#include "nlib.h"

#include "window.h"
#include "gfx.h"

//List of all windows
ll_t* wins = NULL;

/*
 * Creates an empty window
 */
window_t* win_create(char* title, p2d_t size){
    window_t* win = (window_t*)malloc(sizeof(window_t));
    //Set the title, size and position
    strcpy(win->title, title);
    win->size = size;
    win->pos = P2D((gfx_screen().size.x - size.x) / 2, (gfx_screen().size.y - size.y) / 2); //centered relative to the screen by default
    //The window needs to be rendered
    win->needs_rendering = 1;
    win->buf.size = P2D(0, 0);
    win->buf.data = NULL;
    //Add it to the window list
    if(wins == NULL)
        wins = ll_create();
    ll_append(wins, win);

    return win;
}

/*
 * Renders a window if needed
 */
void win_render(window_t* win){
    if(!win->needs_rendering)
        return;
    //(Re-)initialize the buffer
    if(win->buf.size.x != win->size.x || win->buf.size.y != win->size.y){
        win->buf.size = win->size;
        free(win->buf.data);
        win->buf.data = (color32_t*)malloc(win->size.x * win->size.y * sizeof(color32_t));
    }

    win->needs_rendering = false;

    memset(win->buf.data, 128, win->size.x * win->size.y * sizeof(color32_t));
}

/*
 * Draws all windows to the screen
 */
void wins_draw(void){
    if(wins == NULL)
        return;
    window_t* win = NULL;
    while((win = ll_iter(wins, LL_ITER_DIR_UP))){
        win_render(win);
        gfx_draw_raw(gfx_screen(), win->pos, (uint8_t*)win->buf.data, win->buf.size);
    }
}