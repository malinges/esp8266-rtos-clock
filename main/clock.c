#include <sys/time.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "tm1637.h"

#include "./clock.h"

static const char TAG[] = "clock";

#define CLOCK_BRIGHTNESS ((uint8_t) CONFIG_CLOCK_BRIGHTNESS)
#define CLOCK_CLK_PIN ((gpio_num_t) CONFIG_CLOCK_CLK_PIN)
#define CLOCK_DIO_PIN ((gpio_num_t) CONFIG_CLOCK_DIO_PIN)

static const uint8_t num_table[] = {
    //XGFEDCBA
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
};

static const uint8_t colon_digit = 1;
static const uint8_t colon_mask = 0b10000000;

static tm1637_led_t *display;

static void display_buffer(const uint8_t buf[4]) {
    tm1637_set_segment_raw(display, 0, buf[0]);
    tm1637_set_segment_raw(display, 1, buf[1]);
    tm1637_set_segment_raw(display, 2, buf[2]);
    tm1637_set_segment_raw(display, 3, buf[3]);
}

esp_err_t clock_init() {
    display = tm1637_init(CLOCK_CLK_PIN, CLOCK_DIO_PIN);

    if (display == NULL) return ESP_FAIL;

    tm1637_set_brightness(display, CLOCK_BRIGHTNESS);
    const uint8_t buf[] = {0xff, 0xff, 0xff, 0xff};
    display_buffer(buf);

    return ESP_OK;
}

void clock_display_time(struct tm *tm) {
    struct tm tm_;

    // If tm isn't NULL, use it.
    // Otherwise, use current time.
    if (tm == NULL) {
        time_t t;
        time(&t);
        localtime_r(&t, &tm_);
        tm = &tm_;
    }

    uint8_t digits[4];

    digits[0] = num_table[tm->tm_hour / 10];
    digits[1] = num_table[tm->tm_hour % 10];
    digits[2] = num_table[tm->tm_min / 10];
    digits[3] = num_table[tm->tm_min % 10];

    if (tm->tm_sec % 2 == 0) {
        digits[colon_digit] |= colon_mask;
    }

    display_buffer(digits);
}

static void wait_until_clock_synchronized() {
    time_t t;
    struct tm tm;

    while (1) {
        time(&t);
        localtime_r(&t, &tm);
        // tm_year is the current year - 1900.
        // If it's less than 100, it means the year is less than 2000,
        // which in turn means the clock isn't synchronized yet.
        if (tm.tm_year >= 100) {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void clock_task(void *pvParameters) {
    ESP_ERROR_CHECK(clock_init());

    wait_until_clock_synchronized();

    ESP_LOGI(TAG, "Clock synchronized, starting time display");

    while (1) {
        struct timeval update_start;
        ESP_ERROR_CHECK(gettimeofday(&update_start, NULL));

        clock_display_time(NULL);

        struct timeval update_end;
        ESP_ERROR_CHECK(gettimeofday(&update_end, NULL));

        suseconds_t ms_until_next = (1000000 - update_end.tv_usec) / 1000;
        TickType_t ticks_until_next = ms_until_next / portTICK_RATE_MS + 1;

        ESP_LOGI(TAG, "Update delay: %ldus, next update in %u ticks", update_start.tv_usec, ticks_until_next);
        vTaskDelay(ticks_until_next);
    }

    vTaskDelete(NULL);
}