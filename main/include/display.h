#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>

#include "esp_err.h"

void display_set_brightness(uint8_t brightness);
void display_task(void *pvParameters);

#endif