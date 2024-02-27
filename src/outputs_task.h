#ifndef OUTPUTS_TASK_H
#define OUTPUTS_TASK_H
//Standard headers
#include <stdio.h>
//FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//esp-idf headers
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
//my headers
#include "app_config.h"
#include "data_types.h"
#include "nvs_storage.h"
#include "gb_firebase.h"
#include "board.h"

#define UPDT_AUTO_WATER     0
#define UPDT_HUM_CTRL_H     1
#define UPDT_HUM_CTRL_ON    2
#define UPDT_HUM_OFF_HR     3
#define UPDT_HUM_ON_HR      4
#define UPDT_HUM_SET        5
#define UPDT_LIGHTS_OFF_HR  6
#define UPDT_LIGHTS_ON_HR   7
#define UPDT_SOIL_SET       8
#define UPDT_TEMP_CTRL_H    9
#define UPDT_TEMP_CTRL_ON   10
#define UPDT_TEMP_OFF_HR    11
#define UPDT_TEMP_ON_HR     12
#define UPDT_TEMP_SET       13
#define UPDT_WATER_ON       14


void outputsTask ( void* pvParameters );

#endif