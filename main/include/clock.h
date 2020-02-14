#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>

#include "esp_err.h"

esp_err_t clock_init();
void clock_display_time(struct tm *tm);
void clock_task(void *pvParameters);

#endif