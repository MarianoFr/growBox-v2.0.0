#ifndef WIFI_TASK_H
#define WIFI_TASK_H
//Standard headers
#include <stdio.h>
//FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//esp-idf headers
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "driver/gpio.h"
//Arduino headers
#include "WiFi.h"
//my headers
#include "app_config.h"
#include "data_types.h"
#include "nvs_storage.h"
#include "wifi_utils.h"
#include "board.h"

void wifiTask(void* pvParameters);
void serverTask(void* pvParameters);

#endif