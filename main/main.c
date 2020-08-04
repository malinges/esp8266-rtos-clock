/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "lwip/apps/sntp.h"

#include "wifi_manager.h"

#include "display.h"
#include "brightness.h"
#include "tz.h"

static const char *TAG = "main";

#define SNTP_SERVER CONFIG_SNTP_SERVER
#define TZ_NAME     CONFIG_TZ_NAME

static void initialize_sntp(void *arg)
{
    static uint8_t sntp_initialized = 0;
    if (!sntp_initialized) {
        sntp_initialized = 1;
        ESP_LOGI(TAG, "Initializing SNTP");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, SNTP_SERVER);
        sntp_init();
    }
}

static void heap_stats_task(void *arg)
{
    while (1) {
        ESP_LOGI(TAG, "Free heap size: %d", esp_get_free_heap_size());
        ESP_LOGI(TAG, "Minimum free heap size: %d", esp_get_minimum_free_heap_size());
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

void app_main()
{
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();
    ESP_LOGI(TAG, "%s %s compiled %s %s", app_desc->project_name,
        app_desc->version, app_desc->date, app_desc->time);

    ESP_LOGI(TAG, "setting timezone");
    ESP_ERROR_CHECK(tz_set_tz(TZ_NAME));

    ESP_LOGI(TAG, "creating heap stats task");
    xTaskCreate(&heap_stats_task, "heap_stats", 1024, NULL, 2, NULL);
 
    ESP_LOGI(TAG, "starting wifi manager");
    wifi_manager_start();
    wifi_manager_set_callback(EVENT_STA_GOT_IP, &initialize_sntp);

    ESP_LOGI(TAG, "creating display task");
    xTaskCreate(&display_task, "display", 2048, NULL, 7, NULL);

    ESP_LOGI(TAG, "creating brightness monitoring task");
    xTaskCreate(&brightness_task, "brightness", 2048, NULL, 2, NULL);
}
