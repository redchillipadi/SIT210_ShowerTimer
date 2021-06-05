#include "login_screen.h"
#include "user.h"
#include "shower_screen.h"
#include "logger.h"
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define BTN_OK -1
#define BTN_BACK -2
#define BTN_CANCEL -3

#define MAX_PASSWORD_LENGTH 8

static char password[MAX_PASSWORD_LENGTH];
static int password_length;
static char display_password[MAX_PASSWORD_LENGTH * 2 + 1];
static char greeting[20];

static lv_obj_t* title;
static lv_obj_t* instructions;
static lv_obj_t* display_pin;
static lv_obj_t* button_ok;
static lv_obj_t* scr_mainscreen;
static lv_obj_t* scr_showerscreen;
static int selected_user;

extern struct timespec watchdog;

void clear_password()
{
  password_length = 0;
  display_password[0] = 0;
}

void add_digit(int digit)
{
  if (password_length < MAX_PASSWORD_LENGTH)
  {
    password[password_length] = digit;
    display_password[2 * password_length] = '*';
    display_password[2 * password_length + 1] = ' ';
    display_password[2 * password_length + 2] = 0;
    password_length++;
  }
}

int get_password_integer()
{
  int result=password_length;
  for (int i=0; i<password_length; ++i)
  {
    result = result * 10 + password[i];
  }
  return result;
}

void login_screen_update_form()
{
  lv_label_set_text(display_pin, display_password);
  if (password_length == 0)
    lv_obj_add_flag(button_ok, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_clear_flag(button_ok, LV_OBJ_FLAG_HIDDEN);
}

static void event_handler(lv_obj_t* obj, lv_event_t event)
{
  char buffer[50];	// Used to hold string for logging

  if (event == LV_EVENT_CLICKED) {
    clock_gettime(CLOCK_REALTIME, &watchdog);
    int index = (int)lv_event_get_user_data();
    if (index >= 0 && index < 10) {
      add_digit(index);
      login_screen_update_form();
    } else if (index == BTN_OK) {
      if (user_check_password(selected_user, get_password_integer()))
      {
        lv_label_set_text(instructions, "Enter your Pin:");
        clear_password();
        login_screen_update_form();

        if (scr_showerscreen != NULL)
        {
          snprintf(buffer, sizeof(buffer), "Logged in as %s", user_get_name(selected_user));
          log_info("GUI", buffer);
          shower_screen_select_user(selected_user);
          lv_scr_load(scr_showerscreen);
        } else {
          printf("Showerscreen is null in login_screen\n");
        }
      } else {
        snprintf(buffer, sizeof(buffer), "Invalid log in attempt for %s", user_get_name(selected_user));
        log_info("GUI", buffer);
        lv_label_set_text(instructions, "Incorrect Pin. Please try again:");
        clear_password();
        login_screen_update_form();
      }
    } else if (index == BTN_CANCEL) {
      clear_password();
      login_screen_update_form();
    } else if (index == BTN_BACK) {
      if (scr_mainscreen != NULL) {
        lv_scr_load(scr_mainscreen);
      } else {
        printf("Error: mainscreen is null in login_screen\n");
      }
    } else {
      printf("Unhandled entry %d in login_screen event_handler\n", index);
    }
  }
}

lv_obj_t* login_screen_create(lv_obj_t* parent)
{
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

  clear_password();

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

  instructions  = lv_label_create(screen);
  lv_label_set_text(instructions, "Enter your Pin:");
  lv_obj_align(instructions, LV_ALIGN_TOP_MID, 0, 40);

  display_pin = lv_label_create(screen);
  lv_label_set_text(display_pin, display_password);
  lv_obj_align(display_pin, LV_ALIGN_TOP_MID, 0, 64);

  button_ok = lv_btn_create(screen);
  lv_obj_add_event_cb(button_ok, event_handler, (void *)BTN_OK);
  lv_obj_set_size(button_ok, 55, 30);
  lv_obj_align(button_ok, LV_ALIGN_TOP_MID, 70, 60);
  label = lv_label_create(button_ok);
  lv_label_set_text(label, "OK");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)7);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, -70, 110);
  label = lv_label_create(button);
  lv_label_set_text(label, "7");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)8);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 110);
  label = lv_label_create(button);
  lv_label_set_text(label, "8");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)9);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 70, 110);
  label = lv_label_create(button);
  lv_label_set_text(label, "9");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)4);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, -70, 160);
  label = lv_label_create(button);
  lv_label_set_text(label, "4");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)5);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 160);
  label = lv_label_create(button);
  lv_label_set_text(label, "5");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)6);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 70, 160);
  label = lv_label_create(button);
  lv_label_set_text(label, "6");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)1);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, -70, 210);
  label = lv_label_create(button);
  lv_label_set_text(label, "1");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)2);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 210);
  label = lv_label_create(button);
  lv_label_set_text(label, "2");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)3);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 70, 210);
  label = lv_label_create(button);
  lv_label_set_text(label, "3");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)0);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, 260);
  label = lv_label_create(button);
  lv_label_set_text(label, "0");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  button = lv_btn_create(screen);
  lv_obj_add_event_cb(button, event_handler, (void *)BTN_CANCEL);
  lv_obj_set_size(button, 55, 30);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 70, 260);
  label = lv_label_create(button);
  lv_label_set_text(label, "Clear");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  selected_user = -1;
  scr_mainscreen = NULL;

  login_screen_update_form();

  return screen;
}

void login_screen_select_user(int index)
{
  snprintf(greeting, 20, "Hi, %s!", user_get_name(index));
  selected_user = index;
  lv_label_set_text(title, greeting);
  clear_password();
  lv_label_set_text(instructions, "Enter your Pin:");
  login_screen_update_form();
}

void login_screen_set_main_screen(lv_obj_t* mainscreen)
{
  scr_mainscreen = mainscreen;
}

void login_screen_set_shower_screen(lv_obj_t* showerscreen)
{
  scr_showerscreen = showerscreen;
}
