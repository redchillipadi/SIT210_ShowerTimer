#include "lvgl/lvgl.h"
#include "lvgl/lv_drivers/display/fbdev.h"
#include "lvgl/lv_drivers/indev/evdev.h"
#include "user.h"
#include "main_screen.h"
#include "login_screen.h"
#include "shower_screen.h"
#include "blank_screen.h"
#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

struct timespec watchdog;		// Set by each screen's on click handler to delay start of screen saver
bool screensaver_active;		// Indicate whether the screen saver is active

#define SCREENSAVER_DELAY 60
#define NEARLY_DONE_DELAY 180
#define SHOWER_FINISH_DELAY 240

// Display buffer
#define BUFFER_SIZE 16384
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[BUFFER_SIZE];
static lv_color_t buf_2[BUFFER_SIZE];

int main(int argc, char** argv)
{
  log_info("GUI", "Starting the shower GUI service");
  lv_init();
  fbdev_init();
  evdev_init();

  clock_gettime(CLOCK_REALTIME, &watchdog);
  screensaver_active = false;

  lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, BUFFER_SIZE);

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = fbdev_flush;
  disp_drv.draw_buf = &disp_buf;
  disp_drv.hor_res = LV_HOR_RES_MAX;
  disp_drv.ver_res = LV_VER_RES_MAX;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = evdev_read;
  lv_indev_drv_register(&indev_drv);

  user_load();

  lv_obj_t* scr_mainscreen = main_screen_create(NULL);
  main_screen_update_users(scr_mainscreen);
  lv_obj_t* scr_loginscreen = login_screen_create(NULL);
  lv_obj_t* scr_showerscreen = shower_screen_create(NULL);
  lv_obj_t* scr_blankscreen = blank_screen_create(NULL);

  login_screen_set_main_screen(scr_mainscreen);
  login_screen_set_shower_screen(scr_showerscreen);
  main_screen_set_login_screen(scr_loginscreen);
  main_screen_set_blank_screen(scr_blankscreen);
  shower_screen_set_main_screen(scr_mainscreen);

  lv_scr_load(scr_mainscreen);

  struct timespec current_time;

  while (1)
  {
    lv_tick_inc(5);
    lv_task_handler();

    clock_gettime(CLOCK_REALTIME, &current_time);

    // Set the screensaver if required
    if (!screensaver_active && (current_time.tv_sec - watchdog.tv_sec) >= SCREENSAVER_DELAY) {
      log_info("GUI", "Activating the screensaver");
      screensaver_active = true;
      blank_screen_set_return_screen(lv_scr_act());
      lv_scr_load(scr_blankscreen);
    }

    usleep(5000);
  }

  return 0;
}
