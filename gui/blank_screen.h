#ifndef BLANK_SCREEN_H
#define BLANK_SCREEN_H

#include "lvgl/lvgl.h"

lv_obj_t* blank_screen_create(lv_obj_t* parent);
void blank_screen_set_return_screen(lv_obj_t* returnscreen);

#endif
