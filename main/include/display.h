#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>

#include "esp_err.h"

esp_err_t display_init();
void display_set_brightness(uint8_t brightness);
void display_task(void *pvParameters);

#endif