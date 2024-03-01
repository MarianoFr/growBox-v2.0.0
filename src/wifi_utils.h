#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_sntp.h"
#include "esp_attr.h"

#include "nvs_storage.h"
#include "app_config.h"
#include "data_types.h"

#define SERVER_PORT 80
#define URI_STRING "/"

#define ESP_WIFI_SSID      "growbox"
#define ESP_WIFI_PASS      "greenforyou"
#define ESP_WIFI_CHANNEL   1
#define MAX_STA_CONN       4

#define CONFIG_SNTP_TIME_SERVER "time.windows.com"
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

/* Init softap when no wifi creds are stored*/
void wifi_init_softap ( void );


#ifdef __cplusplus
}
#endif
#endif