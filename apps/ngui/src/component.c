//Neutron project
//Standard GUI manager - cdow controller

#include "nlib.h"

#include "component.h"
#include "gfx.h"
#include "ngui.h"

//List of all components
ll_t* comps = NULL;
//List of all screens
ll_t* screens = NULL;
//The next ID of the component
uint64_t next_id = 0;

/*
 * Initialize stuff
 */
void comps_init(void){
    comps = ll_create();
    screens = ll_create();
    create_screens();
}

/*
 * Create screens
 */
void create_screens(void){
    //Neutron currently only supports one screen but
    //Future-proofing ya' know
    component_t* comp = (component_t*)malloc(sizeof(component_t));
    comp->id = next_id++;
    comp->type = CMP_TYPE_SCREEN;
    comp->children = ll_create();
    comp->properties = NULL;
    comp->rendered = 1;
    comp->buf = gfx_screen();
    ll_append(comps, comp);
    ll_append(screens, comp);
}

/*
 * Gets the component by ID
 */
component_t* get_comp_by_id(uint64_t id){
    //Find the component, saving the iterator
    component_t* comp;
    ll_node_t* prev_iter = comps->cur_iter;
    comps->cur_iter = NULL;
    while((comp = (component_t*)ll_iter(comps, LL_ITER_DIR_UP)))
        if(comp->id == id)
            break;
    //Restore the iterator and return the component
    comps->cur_iter = prev_iter;
    return comp;
}

/*
 * Gets the component property by name
 */
prop_val_t* prop_get(component_t* comp, char* name){
    prop_val_t* val = (prop_val_t*)dict_get(comp->properties, name);
    if(!val->linked)
        return val;
    else
        return (prop_val_t*)dict_get(get_comp_by_id(val->link_id)->properties, name);
}

/*
 * Sets the component property by name
 */
void prop_set(component_t* comp, char* name, prop_val_t* val){
    dict_set(comp->properties, name, val);
    //The component probably needs updating after that
    comp->rendered = 0;
}

/*
 * Creates a component
 */
component_t* comp_create(uint64_t type, uint64_t parent){
    //Create the component
    component_t* comp = (component_t*)malloc(sizeof(component_t));
    comp->id = next_id++;
    comp->type = type;
    comp->children = ll_create();
    comp->properties = dict_create();
    comp->rendered = 0;
    comp->state = 0;
    //Add it to the global list and to the list of the parent's children
    ll_append(comps, comp);
    component_t* parent_c = get_comp_by_id(parent);
    ll_append(parent_c->children, comp);
    comp->parent = parent_c;
    //Return the component
    return comp;
}

/*
 * Calculates the size of the component in pixels
 */
p2d_t comp_size(component_t* c){
    p2d_t size;
    switch(c->type){
        case CMP_TYPE_SCREEN: {
            size = c->buf.size;
            break;
        }
        case CMP_TYPE_WINDOW: {
            if(prop_get(c, "fullscreen")->integer)
                size = P2D(gfx_screen().size.x, gfx_screen().size.y - gui_theme()->panel.bar_height);
            else
                size = prop_get(c, "size")->point;
            break;
        }
        case CMP_TYPE_LABEL: {
            size = gfx_text_bounds(gui_theme()->global.main_font, prop_get(c, "text")->string);
            break;
        }
        case CMP_TYPE_BUTTON: {
            size = prop_get(c, "size")->point;
            break;
        }
    }
    return size;
}

/*
 * Calculates the position of the component in pixels relative to the screen
 */
p2d_t comp_pos_abs(component_t* c){
    p2d_t total_pos = P2D(0, 0);
    while(c->type != CMP_TYPE_SCREEN){
        p2d_t pos = comp_pos(c);
        total_pos.x += pos.x;
        total_pos.y += pos.y;
        c = c->parent;
    }
    return total_pos;
}

/*
 * Calculates the position of the component in pixels realtive to its parent
 */
p2d_t comp_pos(component_t* c){
    p2d_t pos = P2D(0, 0);
    switch(c->type){
        case CMP_TYPE_SCREEN: {
            pos = P2D(0, 0);
            break;
        }
        case CMP_TYPE_WINDOW: {
            if(prop_get(c, "fullscreen")->integer)
                pos = P2D(0, gui_theme()->panel.bar_height);
            else
                pos = prop_get(c, "pos")->point;
            break;
        }
        case CMP_TYPE_BUTTON:
        case CMP_TYPE_LABEL: {
            //Get the position relative to the relation point
            pos = prop_get(c, "pos")->point;
            //Adjust for parent targeting
            p2d_t p_size = comp_size(c->parent);
            uint8_t rel = prop_get(c, "relative")->integer;
            uint8_t rel_y = rel & 3, rel_x = rel & 12;
            if(rel_x == CMP_ALIGN_CENTER)
                pos.x += p_size.x / 2;
            else if(rel_x == CMP_ALIGN_RIGHT)
                pos.x += p_size.x;
            if(rel_y == CMP_ALIGN_MIDDLE)
                pos.y += p_size.y / 2;
            else if(rel_y == CMP_ALIGN_BOTTOM)
                pos.y += p_size.y;
            //Adjust for pivot targeting
            p2d_t size = comp_size(c);
            uint8_t piv = prop_get(c, "pivot")->integer;
            uint8_t piv_y = piv & 3, piv_x = piv & 12;
            if(piv_x == CMP_ALIGN_CENTER)
                pos.x -= size.x / 2;
            else if(piv_x == CMP_ALIGN_RIGHT)
                pos.x -= size.x;
            if(piv_y == CMP_ALIGN_MIDDLE)
                pos.y -= size.y / 2;
            else if(piv_y == CMP_ALIGN_BOTTOM)
                pos.y -= size.y;
            break;
        }
    }
    return pos;
}

/*
 * Renders a component
 */
void comp_render(component_t* c){
    //(Re-)initialize the buffer
    p2d_t size = comp_size(c);
    if(c->buf.size.x != size.x || c->buf.size.y != size.y){
        c->buf.size = size;
        free(c->buf.data);
        c->buf.data = (color32_t*)malloc(size.x * size.y * sizeof(color32_t));
    }
    //Determine if the component needs re-rendering
    switch(c->type){
        case CMP_TYPE_BUTTON: {
            //Check if the we're hovering or clicking
            uint8_t hovering = gfx_point_in_rect(gui_cursor_pos(), comp_pos_abs(c), size);
            uint8_t clicked = gui_mouse_flags() & MOUSE_BTN_LEFT;
            uint8_t state = hovering + clicked;
            //If the state was changed, we need to re-render
            if(state != c->state){
                c->state = state;
                c->rendered = 0;
            }
            break;
        }
    }
    //Render the component
    if(!c->rendered){
        switch(c->type){
            case CMP_TYPE_WINDOW: {
                //Get the properties
                char* title = prop_get(c, "title")->string;
                color32_t bg = prop_get(c, "bg")->color;
                color32_t title_c = prop_get(c, "title_color")->color;
                //Draw the window
                gfx_fill(c->buf, COLOR32(0, 0, 0, 0));
                gfx_draw_round_rect(c->buf, P2D(0, 0), size, 2, bg);
                gfx_draw_round_rect(c->buf, P2D(0, 0), P2D(size.x, 16), 2, title_c);
                p2d_t text_size = gfx_text_bounds(gui_theme()->global.main_font, title);
                int64_t spacing = (16 - text_size.y) / 2;
                gfx_draw_str(c->buf, gui_theme()->global.main_font, P2D(spacing, spacing + gui_theme()->global.main_font->ascent),
                    COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), title);
                break;
            }
            case CMP_TYPE_LABEL: {
                //Get the properties
                char* text = prop_get(c, "text")->string;
                color32_t bg = prop_get(c, "bg")->color;
                color32_t fg = prop_get(c, "color")->color;
                //Draw the background color
                gfx_fill(c->buf, bg);
                //Draw the text
                gfx_draw_str(c->buf, gui_theme()->global.main_font, P2D(0, gui_theme()->global.main_font->ascent),
                    fg, COLOR32(0, 0, 0, 0), text);
                break;
            }
            case CMP_TYPE_BUTTON: {
                //Get the properties
                uint8_t hovering = gfx_point_in_rect(gui_cursor_pos(), comp_pos_abs(c), size);
                uint8_t clicked = gui_mouse_flags() & MOUSE_BTN_LEFT;
                char* text = prop_get(c, "text")->string;
                color32_t bg = hovering ? (clicked ? prop_get(c, "bg_click")->color : prop_get(c, "bg_hover")->color) :
                                          prop_get(c, "bg")->color;
                color32_t t_color = prop_get(c, "t_color")->color;
                uint64_t rad = prop_get(c, "radius")->integer;
                //Clear the buffer
                gfx_fill(c->buf, COLOR32(0, 0, 0, 0));
                //Draw the rectangle
                gfx_draw_round_rect(c->buf, P2D(0, 0), size, rad, bg);
                //Draw the text
                p2d_t t_size = gfx_text_bounds(gui_theme()->global.main_font, text);
                gfx_draw_str(c->buf, gui_theme()->global.main_font, P2D((size.x - t_size.x) / 2, (size.y + t_size.y) / 2), t_color, COLOR32(0, 0, 0, 0), text);
                break;
            }
        }
        c->rendered = 1;
    }
    //Render its children
    component_t* child;
    while((child = (component_t*)ll_iter(c->children, LL_ITER_DIR_UP)))
        comp_render(child);
    //Render the component onto the the parent component's buffer
    p2d_t pos = comp_pos(c);
    gfx_draw_raw(c->parent->buf, pos, (uint8_t*)c->buf.data, c->buf.size);
}

/*
 * Redraw everyting
 */
void comps_draw(void){
    //Render each screen
    component_t* screen;
    while((screen = ll_iter(screens, LL_ITER_DIR_UP)))
        comp_render(screen);
}