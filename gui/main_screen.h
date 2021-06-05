#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include "lvgl/lvgl.h"

lv_obj_t* main_screen_create(lv_obj_t* parent);
void main_screen_update_users(lv_obj_t* screen);
void main_screen_set_login_screen(lv_obj_t* loginscreen);
void main_screen_set_blank_screen(lv_obj_t* blankscreen);

#endif
