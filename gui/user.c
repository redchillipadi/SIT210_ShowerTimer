#include "user.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// For each user, store the name, password, image index and most three shower times
struct user
{
  char name[20];
  int password;
  int image;
  struct timespec shower_times[3];
};

#define MAX_USERS 8
static struct user user_list[MAX_USERS];
static int num_users = 0;

#define MAX_IMAGES 4
LV_IMG_DECLARE(red)
LV_IMG_DECLARE(pink)
LV_IMG_DECLARE(blue)
LV_IMG_DECLARE(orange)
static const lv_img_dsc_t* images[MAX_IMAGES] = {
  &red, &pink, &blue, &orange
};

void user_load()
{
  char* line=NULL;
  char* token=NULL;
  const char* comma = ",";
  size_t len=0;
  ssize_t read;

  num_users = 0;
  FILE *fp = fopen("/home/ubuntu/gui/users", "r");
  if (fp == NULL) {
    printf("Error opening users file");
    return;
  }

  read = getline(&line, &len, fp);
  while (read != -1 && num_users < MAX_USERS) {
    char* name = NULL;
    int password = 0;
    int image = 0;
    struct timespec shower[3];

    // Process the user's data in line, which should be comma separated
    token = strtok(line, comma);
    int column = 0;
    while (token != NULL) {
      switch (column) {
        case 0: // Name
          name = token;
          break;
        case 1: // Password
          password = atoi(token);
          break;
        case 2: // Image index
          image = atoi(token);
          break;
        case 3: // Shower 0 tv_sec (int)
          shower[0].tv_sec = atol(token);
          break;
        case 4: // Shower 0 tv_nsec (long int)
          shower[0].tv_nsec = atol(token);
          break;
        case 5: // Shower 1 tv_sec (int)
          shower[1].tv_sec = atol(token);
          break;
        case 6: // Shower 1 tv_nsec (long int)
          shower[1].tv_nsec = atol(token);
          break;
        case 7: // Shower 2 tv_sec (int)
          shower[2].tv_sec = atol(token);
          break;
        case 8: // Shower 2 tv_nsec (long int)
          shower[2].tv_nsec = atol(token);
          break;
      }
      token = strtok(NULL, comma);
      column++;
    }
    if (column == 9)
      user_create(name, password, image, shower);

    read = getline(&line, &len, fp);
  }

  fclose(fp);
}

void user_save()
{
  FILE *fp = fopen("/home/ubuntu/gui/users", "w");
  if (fp == NULL) {
    printf("Error opening users file");
    return;
  }

  for (int i=0; i < num_users; ++i) {
    fprintf(fp, "%s, %d, %d, %ld, %ld, %ld, %ld, %ld, %ld\n",
      user_list[i].name, user_list[i].password, user_list[i].image,
      user_list[i].shower_times[0].tv_sec, user_list[i].shower_times[0].tv_nsec,
      user_list[i].shower_times[1].tv_sec, user_list[i].shower_times[1].tv_nsec,
      user_list[i].shower_times[2].tv_sec, user_list[i].shower_times[2].tv_nsec);
  }

  fclose(fp);
}

int user_create(const char* name, int password, int image, struct timespec *showers)
{
  if (num_users < MAX_USERS)
  {
    strncpy(user_list[num_users].name,name,20);
    user_list[num_users].password = password;
    user_list[num_users].image = image;
    for (int i=0; i<3; i++)
      user_list[num_users].shower_times[i] = showers[i];

    return num_users++;
  }
  return -1;
}


int user_get_count(void)
{
  return num_users;
}

const char* user_get_name(int index)
{
  if (index < 0 || index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_get_name\n", index);
    return NULL;
  } else {
    return user_list[index].name;
  }
}

bool user_check_password(int index, int password)
{
  if (index < 0 || index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_check_password\n", index);
    return NULL;
  } else {
    return user_list[index].password == password;
  }
}

const lv_img_dsc_t* user_get_image(int user_index)
{
  if (user_index < 0 || user_index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_get_image\n", user_index);
    return NULL;
  }
  int index = user_list[user_index].image;
  if (index < 0 || index >= MAX_IMAGES) {
    printf("Error: Invalid image index %d in user_get_image\n", index);
    return NULL;
  }
  return images[index];
}

int user_get_shower_countdown(int user_index)
{
  int countdown = 0;

  if (user_index < 0 || user_index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_get_shower_countdown\n", user_index);
    return false;
  }

  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);

  // If the last shower was within the last hour, don't permit the user to have a shower.
  if (current_time.tv_sec - user_list[user_index].shower_times[0].tv_sec < 3600) {
    countdown = 3600 - (current_time.tv_sec - user_list[user_index].shower_times[0].tv_sec);
    //printf("Shower at %ld was within the last hour of %ld\n", user_list[user_index].shower_times[0].tv_sec, current_time.tv_sec);
  }

  // If the last three showers were all within the last 24 hours, don't permit the user to have a shower
  int today = 0;
  for (int i=0; i<3; i++) {
    if (current_time.tv_sec - user_list[user_index].shower_times[i].tv_sec < 86400) {
      //printf("Shower %d at %ld was within the last day of %ld\n", i, user_list[user_index].shower_times[i].tv_sec, current_time.tv_sec);
      today++;
    }
  }

  if (today >= 3) {
    int daily = 86400 - (current_time.tv_sec - user_list[user_index].shower_times[2].tv_sec);
    return (countdown > daily) ? countdown : daily;
  }

  return countdown;
}

// Return 0 if the user can have a shower, update the shower times and save the data
// Otherwise return the number of seconds before the user can have a shower
int user_start_shower(int user_index)
{
  if (user_index < 0 || user_index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_start_shower\n", user_index);
    return false;
  }

  int countdown = user_get_shower_countdown(user_index);

  if (countdown > 0)
    return countdown;

  // Otherwise the user can have a shower
  // Update the shower times

  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);

  for (int i=2; i > 1; i--)
    user_list[user_index].shower_times[i] = user_list[user_index].shower_times[i-1];
  user_list[user_index].shower_times[0] = current_time;

  user_save();
  return 0;
}

int user_shower_active(int user_index)
{
  if (user_index < 0 || user_index >= MAX_USERS) {
    printf("Error: Invalid index %d in user_start_shower\n", user_index);
    return 0;
  }

  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);

  for (int i=0; i<3; ++i)
  {
    int remaining_time = 240 - (current_time.tv_sec - user_list[user_index].shower_times[i].tv_sec);
    if (remaining_time > 0 && remaining_time <= 240)
      return remaining_time;
  }
  return 0;
}
