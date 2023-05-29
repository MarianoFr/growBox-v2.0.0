#ifndef RGB_RSST_BUTT_H
#define RGB_RSST_BUTT_H
#include <Arduino.h>
#include "globals.h"
#include "fb_mgmt.h"
#include "write_mem.h"
//RGB errors
#define WIFI_DISC     0b10000000
#define WIFI_CONN     0b00000001
#define NO_WIFI_CRED  0b00000010
#define NO_USER       0b00000100
#define DHT_ERR       0b00001000
#define RTC_ERR       0b00010000
#define SOIL_ERR      0b00100000
#define BH_1750_ERR   0b01000000

/***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
void RGBalert(void);
/******************************************/
// Debounce routine for WiFi reset button //
/******************************************/
void debounceWiFiReset(void);

#endif