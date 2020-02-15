#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "vdd_monitoring.h"

static const char TAG[] = "vdd_mon";

void vdd_monitoring_task(void *pvParameters) {
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing ADC");

    adc_config_t adc_config = {
        .mode = ADC_READ_VDD_MODE,
        .clk_div = 8
    };

    err = adc_init(&adc_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    TickType_t tick = xTaskGetTickCount();

    while (1) {
        uint16_t value;
        err = adc_read(&value);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read ADC: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "VDD voltage: %.3fV", (float) value / 1000);
        }
        vTaskDelayUntil(&tick, 5000 / portTICK_RATE_MS);
    }
}
