#ifndef WRITE_MEM_H
#define WRITE_MEM_H
#include <Arduino.h>
#include <EEPROM.h>

/* Flag to detect whether user's WiFi credentials
    are stored in FLASH memory or not and
    adresses where they will be stored */
#define WIFI_CRED_FLAG 0
#define NO_CREDENTIALS 0
#define WITH_CREDENTIALS 1
#define SSID_STORING_ADD 1
#define PASS_STORING_ADD 51
/**************************************************************************************/
// Write data to memory                                                               //
// We prepping the data strings by adding the end of line symbol I decided to use ";".//
// Then we pass it off to the write_EEPROM function to actually write it to memmory   //
/**************************************************************************************/
void write_to_Memory(String s, String p);
/********************************/
// Reads a string out of memory //
/********************************/
String FLASH_read_string(int l, int p);

#endif