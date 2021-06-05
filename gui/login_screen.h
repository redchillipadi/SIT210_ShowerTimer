#ifndef LOGIN_SCREEN_H
#define LOGIN_SCREEN_H

#include "lvgl/lvgl.h"

lv_obj_t* login_screen_create(lv_obj_t* parent);
void login_screen_add_user(lv_obj_t* screen, int index, int password);
void login_screen_select_user(int index);
void login_screen_set_main_screen(lv_obj_t* mainscreen);
void login_screen_set_shower_screen(lv_obj_t* showerscreen);

#endif
