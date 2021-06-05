#include "shower_screen.h"
#include "user.h"
#include "logger.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>

static char greeting[20];
static int selected_user;
static lv_obj_t* title;
static lv_obj_t* scr_mainscreen;
static lv_obj_t* countdown_label;

#define BTN_BACK 0
#define BTN_SHOWER 1
#define BTN_USAGE 2
#define BTN_WEEKLY 3

extern struct timespec watchdog;

static void start_shower()
{
  FILE *fp_shower;
  fp_shower = fopen("/tmp/shower", "w");
  if (fp_shower == NULL) {
    printf("Unable to open shower control file for writing");
  } else {
    fprintf(fp_shower, "O\n");
  }
  fclose(fp_shower);
}

static void event_handler(lv_obj_t* obj, lv_event_t event)
{
  char buffer[50];	// Used to hold string for logging

  if (event == LV_EVENT_CLICKED) {
    clock_gettime(CLOCK_REALTIME, &watchdog);
    int index = (int)lv_event_get_user_data();
    int countdown;

    switch (index)
    {
    case BTN_BACK:
      if (scr_mainscreen != NULL)
      {
        selected_user = -1;
        lv_scr_load(scr_mainscreen);
      } else {
        printf("Mainscreen is NULL in shower_screen\n");
      }
      break;

    case BTN_SHOWER:
      countdown = user_start_shower(selected_user);
      if (countdown == 0) {
        snprintf(buffer, sizeof(buffer), "Starting shower for %s", user_get_name(selected_user));
        log_info("GUI", buffer);
        start_shower();
      } else {
        printf("User %d must wait another %d seconds\n", selected_user, countdown);
      }
      break;
    }
  }
  shower_update_label(selected_user);
}

lv_obj_t* shower_screen_create(lv_obj_t* parent)
{
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

  title = lv_label_create(screen);
  lv_label_set_text(title, "Hi!");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t* button;
  lv_obj_t* label;

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)BTN_BACK);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 20, 20);
  label = lv_label_create(button);
  lv_label_set_text(label, "Back");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)BTN_SHOWER);
  lv_obj_set_size(button, 430, 230);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 20, 70);
  countdown_label = lv_label_create(button);
  lv_label_set_text(countdown_label, "Shower available");
  lv_obj_align(countdown_label, LV_ALIGN_BOTTOM_MID, 0, 0);

/*
  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)BTN_USAGE);
  lv_obj_set_size(button, 130, 230);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 170, 70);
  label = lv_label_create(button);
  lv_label_set_text(label, "Usage");
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)BTN_WEEKLY);
  lv_obj_set_size(button, 130, 230);
  lv_obj_align(button, LV_ALIGN_TOP_LEFT, 320, 70);
  label = lv_label_create(button);
  lv_label_set_text(label, "Weekly");
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
*/

  selected_user = -1;
  scr_mainscreen = NULL;

  return screen;
}

void shower_update_label(int index)
{
  if (countdown_label == 0)
    return;

  static char buffer[50];
  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);
  int remaining_time = user_shower_active(index);
  if (remaining_time > 0 && remaining_time <= 240) {
    sprintf(buffer, "Your shower has\n%d seconds remaining", remaining_time);
    lv_label_set_text(countdown_label, buffer);
  } else {
    int countdown = user_get_shower_countdown(index);
    if (countdown == 0) {
      lv_label_set_text(countdown_label, "Click to start\nyour shower");
    } else {
      sprintf(buffer, "The shower is unavailable\nfor %d seconds", countdown);
      lv_label_set_text(countdown_label, buffer);
    }
  }
}

void shower_screen_select_user(int index)
{
  snprintf(greeting, 20, "Hi, %s!", user_get_name(index));
  selected_user = index;
  lv_label_set_text(title, greeting);
  shower_update_label(index);
}

void shower_screen_set_main_screen(lv_obj_t* mainscreen)
{
  scr_mainscreen = mainscreen;
}
