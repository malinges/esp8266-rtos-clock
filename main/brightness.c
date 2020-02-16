#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "display.h"
#include "brightness.h"

#define HYSTERESIS 0.25

static const char TAG[] = "brightness";

void brightness_task(void *pvParameters) {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing ADC");

    adc_config_t adc_config = {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 8
    };

    err = adc_init(&adc_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    TickType_t tick = xTaskGetTickCount();
    int8_t last_brightness = -1;
    uint16_t last_value;

    while (1) {
        uint16_t value;
        err = adc_read(&value);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC: %s", esp_err_to_name(err));
        } else if (value > 1023) {
            ESP_LOGE(TAG, "ADC returned out of bounds value: %hu", value);
        } else {
            uint8_t brightness;

            if (last_brightness == -1) {
                brightness = value / 128.0f;
            } else if (value < last_value) {
                brightness = value / 128.0f + HYSTERESIS;
                if (brightness > last_brightness) brightness = last_brightness;
            } else if (value > last_value) {
                brightness = value / 128.0f - HYSTERESIS;
                if (brightness < last_brightness) brightness = last_brightness;
            }

            last_value = value;
            last_brightness = brightness;

            ESP_LOGI(TAG, "ADC voltage: %.3fV, brightness=%d", (float) value / 1023, (int) brightness);

            display_set_brightness(brightness);
        }

        vTaskDelayUntil(&tick, 200 / portTICK_RATE_MS);
    }
}
