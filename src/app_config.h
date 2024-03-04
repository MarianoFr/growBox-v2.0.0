#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define configWIFI_STACK_SIZE      20*1024
#define configSAMPLER_STACK_SIZE   10*1024
#define configOUTPUTS_STACK_SIZE   20*1024

#define GB_TIME_ZONE               "WST+3"
#define NTP_RETRY 3

#define WIFI_TASK_PRIORITY         6
#define OUTPUTS_TASK_PRIORITY      4
#define SAMPLER_TASK_PRIORITY      4

#define WIFI_RETRY                 5
#define MAX_IN_TEMP                28/*cosidering the vegetative term*/
#define MAX_IN_HUM                 65/*considering the vegetative term*/
#define MIN_IN_TEMP                26
#define MIN_IN_HUM                 63

#define TEMPERATURE_HISTERESIS     3
#define HUMIDITY_HISTERESIS        5
#define SOIL_HISTERESIS            5
#define WATERING_DELAY             3*60000
#define MAX_WATER_TOGGLE           10 

//RGB errors
#define WIFI_DISC     0
#define WIFI_CONN     1
#define NO_WIFI_CRED  2
#define NO_USER       3
#define HTU21_ERR     4
#define RTC_ERR       5
#define SOIL_ERR      6
#define BH1750_ERR    7

#endif