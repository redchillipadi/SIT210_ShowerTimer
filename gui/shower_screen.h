#ifndef SHOWER_SCREEN_H
#define SHOWER_SCREEN_H

#include "lvgl/lvgl.h"

lv_obj_t* shower_screen_create(lv_obj_t* parent);
void shower_screen_select_user(int index);
void shower_screen_set_main_screen(lv_obj_t* mainscreen);
void shower_update_label(int index);

#endif
