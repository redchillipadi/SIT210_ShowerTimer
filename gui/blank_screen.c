#include "blank_screen.h"
#include "logger.h"
#include "lvgl/src/misc/lv_color.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>

static lv_obj_t* scr_returnscreen;
extern struct timespec watchdog;
extern bool screensaver_active;

static void event_handler(lv_obj_t* obj, lv_event_t event)
{
  if (event == LV_EVENT_CLICKED) {
    if (scr_returnscreen != NULL) {
      log_info("GUI", "Deactivating screen saver");
      clock_gettime(CLOCK_REALTIME, &watchdog);
      lv_scr_load(scr_returnscreen);
      screensaver_active = false;
    } else {
      printf("Returnscreen is NULL in shower_screen\n");
    }
  }
}

lv_obj_t* blank_screen_create(lv_obj_t* parent)
{
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_color(&style, LV_COLOR_MAKE(0, 0, 0));
  lv_obj_add_style(screen, &style, LV_PART_MAIN);

  lv_obj_add_event_cb(screen, event_handler, NULL);

  scr_returnscreen = NULL;

  return screen;
}

void blank_screen_set_return_screen(lv_obj_t* returnscreen)
{
  scr_returnscreen = returnscreen;
}
