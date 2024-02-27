#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "debug_flags.h"
#include "data_types.h"

#define WIFI_SSID_SIZE 30

#define WAIT_VARIABLES_FROM_FB  0
#define VARIABLES_OK            1
#define ERROR_READING_MEM       2
#define ERROR_WRITING_MEM       3

uint8_t nvs_read_control_variables( rx_control_data_t* rx_data );
uint8_t nvs_write_control_variables( rx_control_data_t* rx_data );
bool nvs_write_wifi_creds( char* ssid, char* pass );
bool nvs_read_wifi_creds( char* ssid, char* pass );
bool nvs_erase_wifi_creds( void );
bool nvs_erase_storage( void );

#ifdef __cplusplus
}
#endif

#endif