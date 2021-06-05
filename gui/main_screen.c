#include "main_screen.h"
#include "blank_screen.h"
#include "user.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define BTN_BLANK -1

static lv_obj_t* scr_mainscreen;
static lv_obj_t* scr_loginscreen;
static lv_obj_t* scr_blankscreen;

extern struct timespec watchdog;

static void event_handler(lv_obj_t* obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED) {
    clock_gettime(CLOCK_REALTIME, &watchdog);
    int index = (int)lv_event_get_user_data();
    if (index == BTN_BLANK) {
      if (scr_blankscreen != NULL) {
        blank_screen_set_return_screen(scr_mainscreen);
        lv_scr_load(scr_blankscreen);
      } else {
        printf("Error: blankscreen in mainscreen is NULL\n");
      }
    } else {
      if (scr_loginscreen != NULL) {
         login_screen_select_user(index);
         lv_scr_load(scr_loginscreen);
      } else {
        printf("Error: loginscreen in mainscreen is NULL\n");
      }
    }
  }
}

lv_obj_t* main_screen_create(lv_obj_t* parent)
{
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

  lv_obj_t* title = lv_label_create(screen);
  lv_label_set_text(title, "Welcome!");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t* arrow;

  arrow = lv_label_create(screen);
  lv_label_set_text(arrow, LV_SYMBOL_LEFT);
  lv_obj_align(arrow, LV_ALIGN_LEFT_MID, 0, 0);

  arrow = lv_label_create(screen);
  lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
  lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);

  scr_loginscreen = NULL;
  scr_blankscreen = NULL;
  scr_mainscreen = screen;

  return screen;
}

void add_main_screen_user(lv_obj_t* screen, const char* username, const lv_img_dsc_t* image_src, int index)
{
  lv_obj_t* button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)index);
  lv_obj_set_pos(button, 20 + 115 * (index % 4), 20 + 150 * (index / 4));
  lv_obj_set_size(button, 95, 130);

  lv_obj_t* image = lv_img_create(button);
  lv_img_set_src(image, image_src);
  lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, username);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void main_screen_update_users(lv_obj_t* screen)
{
  for (int i=0; i<user_get_count(); ++i)
    add_main_screen_user(screen, user_get_name(i), user_get_image(i), i);
}

void main_screen_set_login_screen(lv_obj_t* loginscreen)
{
  scr_loginscreen = loginscreen;
}

void main_screen_set_blank_screen(lv_obj_t* blankscreen)
{
  scr_blankscreen = blankscreen;
}
