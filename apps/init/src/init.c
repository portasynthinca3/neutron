//Neutron project
//Default initialization application

#include "nlib.h"
#include "app_desc.h"

void main(void* args){
    _gfx_println_verbose("Neutron standard initializer, version:");
    _gfx_println_verbose(__APP_VERSION);
    _gfx_fill_rect(COLOR32(255, 255, 255, 255), P2D(300, 300), P2D(300, 50));
    _gfx_draw_str(P2D(310, 310), COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0), "test");
    _gfx_flip();
    while(1);
}