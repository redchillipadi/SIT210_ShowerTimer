#include "logger.h"
#include <stdio.h>
#include <time.h>

/* get_timestamp - Set a string with the current time
 * Params:
 *  timestamp - pointer to the c string to store the result
 *  size - the maximum size of the string, including the null character
 * Returns: Nothing
 */
void get_timestamp(char* buffer, unsigned int size)
{
  struct timespec current_time;	// Hold the time as a seconds/nanoseconds since epoch
  struct tm tm;			// Hold the time broken into components

  if (buffer == NULL)
    return;

  clock_gettime(CLOCK_REALTIME, &current_time);
  gmtime_r(&current_time.tv_sec, &tm);
  strftime(buffer, size, "%Y-%m-%d %H:%M:%S,000", &tm);
}

/* log_info - Write an info message to the log file
 * Params - message - the message to be written to the log
 * Returns - Nothing
 *
 * Writes the message to the log file
 * The log is not written often, so it is opened and closed each time
 */
void log_info(const char* tag, const char* message)
{
  char timestamp[24];
  get_timestamp(timestamp, sizeof(timestamp));
  FILE *fp = fopen("/var/log/shower", "a");
  if (fp == NULL) {
    printf("Unable to open log file for writing");
  } else {
    fprintf(fp, "%s - %s - %s\n", timestamp, tag, message);
  }
  fclose(fp);
}
