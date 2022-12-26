#include "write_mem.h"

/**************************************************************************************/
// Write data to memory                                                               //
// We prepping the data strings by adding the end of line symbol I decided to use ";".//
// Then we pass it off to the write_EEPROM function to actually write it to memmory   //
/**************************************************************************************/
void write_to_Memory(String s, String p) {//(String m,
  if ( !EEPROM.read( WIFI_CRED_FLAG ) ) {
    EEPROM.write(WIFI_CRED_FLAG, WITH_CREDENTIALS); // Flag indicates that WiFi credentials are stored in memory
  }
  s += ";";
  for (int n = SSID_STORING_ADD; n < s.length() + SSID_STORING_ADD; n++) {
    EEPROM.write(n, s[n - SSID_STORING_ADD]);
  }
  p += ";";
  for (int n = PASS_STORING_ADD; n < p.length() + PASS_STORING_ADD; n++) {
    EEPROM.write(n, p[n - PASS_STORING_ADD]);
  }
  EEPROM.commit();
}

/********************************/
// Reads a string out of memory //
/********************************/
String FLASH_read_string(int l, int p) {
  String temp;
  for (int n = p; n < l + p; ++n)
  {
    if (char(EEPROM.read(n)) != ';') {
      temp += String(char(EEPROM.read(n)));
    } else n = l + p;
  }
  return temp;
}