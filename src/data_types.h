#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief
 * Typedef for rgb state
 */
typedef uint8_t rgb_state_t;

/**
 * @brief
 * Struct with sensor sampled data to send to FireBase
 */
typedef struct { 
    float temperature;
	float humidity;
    uint8_t soil_humidity;
    uint32_t lux;
    char esp_tag[20];
} tx_sensor_data_t;

/**
 * @brief
 * Struct with control data received from FireBase
 */
typedef struct { 
    bool    automatic_watering;
    bool    humidity_control_high;
    bool    humidity_control;
    uint8_t humidity_off_hour;
    uint8_t humidity_on_hour;    
    uint8_t humidity_set;
	uint8_t lights_off_hour;
    uint8_t lights_on_hour;
    uint8_t soil_moisture_set;
    bool    temperature_control_high;   
    bool    temperature_control;
    uint8_t temperature_off_hour;
    uint8_t temperature_on_hour;
    uint8_t temperature_set;
    bool    water;
} rx_control_data_t;

/**
 * @brief
 * Struct with control data received from FireBase
 * and which vaiable to update
 */
typedef struct { 
    rx_control_data_t control_data;
    uint16_t var_2_update;
} rx_control_update_t;

/**
 * @brief
 * Struct with control data to send to FireBase
 */
typedef struct { 
    bool lights_on;
	bool temperature_on;
    bool humidity_on;
    bool water_on;
} tx_control_data_t;

/**
 * @brief
 * Struct with wifi credentials
 */
typedef struct wifi_credentials_t {
    char ssid[30];
    char pass[30];
} wifi_credentials_t;

#ifdef __cplusplus
}
#endif

#endif