#ifndef USER_H
#define USER_H

#include "lvgl/lvgl.h"
#include <time.h>

void user_load();
void user_save();
int user_create(const char* name, int password, int image, struct timespec* shower);
int user_get_count(void);
const char* user_get_name(int index);
bool user_check_password(int index, int password);
const lv_img_dsc_t* user_get_image(int index);
int user_start_shower(int index);
int user_get_shower_countdown(int index);
int user_shower_active(int index);

#endif
