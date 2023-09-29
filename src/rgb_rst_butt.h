#ifndef RGB_RST_BUTT_H
#define RGB_RST_BUTT_H
#include <Arduino.h>
#include "globals.h"
#include "fb_mgmt.h"
#include "write_mem.h"
//RGB errors
#define WIFI_DISC     0
#define WIFI_CONN     1
#define NO_WIFI_CRED  2
#define NO_USER       3
#define HTU21_ERR     4
#define RTC_ERR       5
#define SOIL_ERR      6
#define BH_1750_ERR   7

/******************************************/
// Debounce routine for WiFi reset button //
/******************************************/
void debounceWiFiReset(void);

#endif