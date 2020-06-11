#pragma once

#include "nlib.h"

#include "gfx.h"

//Definitions

//Component types
#define CMP_TYPE_SCREEN                 0
#define CMP_TYPE_WINDOW                 1
#define CMP_TYPE_LABEL                  2
#define CMP_TYPE_BUTTON                 3
//Component property value types
#define CMP_PV_TYPE_POINT               0
#define CMP_PV_TYPE_STRING              1
#define CMP_PV_TYPE_INTEGER             2
#define CMP_PV_TYPE_DOUBLE              3
#define CMP_PV_TYPE_POINT_F             4
#define CMP_PV_TYPE_COLOR               5
//Component alignments
#define CMP_ALIGN_TOP                   1
#define CMP_ALIGN_MIDDLE                2
#define CMP_ALIGN_BOTTOM                3
#define CMP_ALIGN_LEFT                  (1 << 2)
#define CMP_ALIGN_CENTER                (2 << 2)
#define CMP_ALIGN_RIGHT                 (3 << 2)

//Macros for converting to property values
#define PROP_POINT(v) (&(prop_val_t){.type = CMP_PV_TYPE_POINT, .linked = 0, .point = v})
#define PROP_STRING(v) (&(prop_val_t){.type = CMP_PV_TYPE_STRING, .linked = 0, .string = v})
#define PROP_INTEGER(v) (&(prop_val_t){.type = CMP_PV_TYPE_INTEGER, .linked = 0, .integer = v})
#define PROP_DOUBLE(v) (&(prop_val_t){.type = CMP_PV_TYPE_DOUBLE, .linked = 0, .dbl = v})
#define PROP_POINT_F(v) (&(prop_val_t){.type = CMP_PV_TYPE_POINT_F, .linked = 0, .point_f = v})
#define PROP_COLOR(v) (&(prop_val_t){.type = CMP_PV_TYPE_COLOR, .linked = 0, .color = v})

//Structures

//Component property value
typedef struct {
    //The type of the value
    uint32_t  type;
    //Is the property linked to another component's property? And if it is, to which one and how?
    uint8_t   linked;
    uint64_t  link_id;
    char*     link_name;
    char*     link_expr;
    //All possible value types
    p2d_t     point;
    char*     string;
    uint64_t  integer;
    double    dbl;
    p2df_t    point_f;
    color32_t color;
} prop_val_t;

//A control
typedef struct _comp_s{
    uint64_t        id;
    uint64_t        type;
    dict_t*         properties;
    ll_t*           children;
    uint8_t         rendered;
    raw_img_t       buf;
    struct _comp_s* parent;
    uint8_t         state;
} component_t;

//Function prototypes

void         comps_init     (void);
void         create_screens (void);
component_t* get_comp_by_id (uint64_t id);
prop_val_t*  prop_get       (component_t* comp, char* name);
void         prop_set       (component_t* comp, char* name, prop_val_t* val);
component_t* comp_create    (uint64_t type, uint64_t parent);
p2d_t        comp_size      (component_t* c);
p2d_t        comp_pos_abs   (component_t* c);
p2d_t        comp_pos       (component_t* c);
void         comp_render    (component_t* c);
void         comps_draw     (void);