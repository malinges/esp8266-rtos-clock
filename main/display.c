#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"

#include "tm1637.h"

#include "display.h"

static const char TAG[] = "display";

#define CLK_PIN ((gpio_num_t) CONFIG_CLOCK_CLK_PIN)
#define DIO_PIN ((gpio_num_t) CONFIG_CLOCK_DIO_PIN)

#define MAX_BRIGHTNESS 7

#define COLON_DIGIT 1
#define COLON_MASK 0b10000000

static tm1637_led_t *display = NULL;

DRAM_ATTR static const uint8_t num_table[] = {
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

static volatile EventGroupHandle_t eventGroupHandle;

#define DISPLAY_UPDATE_PENDING_BIT 1

static esp_err_t get_time(struct tm *tm) {
    time_t t = time(NULL);
    if (t == (time_t) -1) {
        ESP_LOGE(TAG, "time() failed!");
        return ESP_FAIL;
    }

    if (localtime_r(&t, tm) == NULL) {
        ESP_LOGE(TAG, "localtime_r() failed!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t time_to_buffer(struct tm *tm, uint8_t *out) {
    if (tm == NULL || out == NULL) {
        ESP_LOGE(TAG, "time_to_buffer() was passed a NULL %s argument!", tm == NULL ? "tm" : "out");
        return ESP_FAIL;
    }

    out[0] = num_table[tm->tm_hour / 10];
    out[1] = num_table[tm->tm_hour % 10];
    out[2] = num_table[tm->tm_min / 10];
    out[3] = num_table[tm->tm_min % 10];

    if (tm->tm_sec % 2 == 0) {
        out[COLON_DIGIT] |= COLON_MASK;
    }

    return ESP_OK;
}

static void display_buffer(const uint8_t *buf) {
    taskENTER_CRITICAL();
    tm1637_set_segment_raw(display, 0, buf[0]);
    tm1637_set_segment_raw(display, 1, buf[1]);
    tm1637_set_segment_raw(display, 2, buf[2]);
    tm1637_set_segment_raw(display, 3, buf[3]);
    taskEXIT_CRITICAL();
}

static esp_err_t display_time() {
    struct tm tm;
    if (get_time(&tm) != ESP_OK) {
        return ESP_FAIL;
    }

    uint8_t buffer[4];
    if (time_to_buffer(&tm, buffer) != ESP_OK) {
        return ESP_FAIL;
    }

    display_buffer(buffer);

    return ESP_OK;
}

static void wait_until_clock_synchronized() {
    struct tm tm;

    while (1) {
        if (get_time(&tm) != ESP_OK) {
            ESP_LOGE(TAG, "get_time() failed!");
        } else if (tm.tm_year >= 100) {
            // tm_year is the current year - 1900.
            // If it's less than 100, it means the year is less than 2000,
            // which in turn means the clock isn't synchronized yet.
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static esp_err_t display_init() {
    display = tm1637_init(CLK_PIN, DIO_PIN);

    if (display == NULL) return ESP_FAIL;

    tm1637_set_brightness(display, MAX_BRIGHTNESS);
    const uint8_t buf[] = {0xff, 0xff, 0xff, 0xff};
    display_buffer(buf);

    return ESP_OK;
}

IRAM_ATTR static void hw_timer_callback(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xEventGroupSetBitsFromISR(eventGroupHandle, DISPLAY_UPDATE_PENDING_BIT, &xHigherPriorityTaskWoken) == pdTRUE) {
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
}

void display_set_brightness(uint8_t brightness) {
    tm1637_set_brightness(display, brightness > MAX_BRIGHTNESS ? MAX_BRIGHTNESS : brightness);
}

void display_task(void *pvParameters) {
    ESP_ERROR_CHECK(display_init());

    eventGroupHandle = xEventGroupCreate();

    ESP_ERROR_CHECK(hw_timer_init(&hw_timer_callback, NULL));

    wait_until_clock_synchronized();
    ESP_LOGI(TAG, "Clock synchronized, starting time display");

    struct timeval tv;

    while (1) {
        taskENTER_CRITICAL();

        ESP_ERROR_CHECK(gettimeofday(&tv, NULL));
        ESP_LOGI(TAG, "Called at: %ldus", tv.tv_usec);

        display_time();

        ESP_ERROR_CHECK(gettimeofday(&tv, NULL));
        uint32_t alarm_us = 1001000 - tv.tv_usec;
        ESP_ERROR_CHECK(hw_timer_alarm_us(alarm_us < 10 ? 10 : alarm_us, false));

        taskEXIT_CRITICAL();

        while ((xEventGroupWaitBits(eventGroupHandle, DISPLAY_UPDATE_PENDING_BIT, pdTRUE, pdFALSE, 2000 / portTICK_RATE_MS) & DISPLAY_UPDATE_PENDING_BIT) == 0) {
            ESP_LOGW(TAG, "Still waiting for display update...");
        }
    }
}