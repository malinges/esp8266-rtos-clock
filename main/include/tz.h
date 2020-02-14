#ifndef TZ_H
#define TZ_H

#include "esp_err.h"

typedef struct {
    const char *name;
    const char *tz;
} timezone_t;

extern const timezone_t timezones[];

esp_err_t tz_set_tz(const char *name);

#endif
