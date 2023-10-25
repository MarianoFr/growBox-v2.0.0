#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include "fb_mgmt.h"
#include "Adafruit_HTU21DF.h"
#define V0_SOIL 101
#define V05_SOIL 151

/*PinOut definition*/
#define PIN_RED   18
#define PIN_GREEN 5
#define PIN_BLUE  17
#define WIFI_RESET  12
#define SOILPIN     32
#define HUM_CTRL_OUTPUT      25/*FOR NEW VERSION*/
#define TEMP_CTRL_OUTPUT     33    /* PCB*/
#define W_VLV_PIN   26
#define LIGHTS      27
#define MAX_IN_TEMP 28/*cosidering the vegetative term*/
#define MAX_IN_HUM  65/*considering the vegetative term*/
#define MIN_IN_TEMP 26
#define MIN_IN_HUM  63
#define HTU21_SENSE_PERIOD       30000
/*Global RGB state timer*/
extern TimerHandle_t xRgbTimer;
/*HTU21 object*/
extern Adafruit_HTU21DF htu;//HTU21 sensor object    
/*Auto soil calib points*/
extern uint32_t v0;
extern uint32_t v05;
/*Declare the Firebase Data object in the global scope*/
extern FirebaseData firebaseData2;
extern FirebaseData firebaseData1;
extern String mac;
/* Flag activated when WiFi credentials are beign asked by ESP32 */
extern bool gettingWiFiCredentials;
extern char users_uid[100];
extern String path;
extern QueueHandle_t writeQueue;
extern QueueHandle_t waterQueue;
struct readControl {
  bool tempCtrlHigh = false;
  bool humCtrlHigh = false;
  bool temperatureControl = false;
  bool humidityControl = false;
  bool automaticWatering = false;
  int humidityOnHour = 0;
  int humidityOffHour = 0;
  int temperatureOnHour = 0;
  int temperatureOffHour = 0;
  int humiditySet = 30;
  int onHour = 0;
  int offHour = 0;
  int soilMoistureSet = 0;
  int temperatureSet = 28;
  bool water = false;
};
struct writeControl {
  String ESPtag;/*where current time is loaded*/
  bool humidityControlOn = false;
  bool temperatureControlOn = false;
  float humidity  = 25;
  bool lights = false;
  float lux = 0;
  bool soil = false;/*true means wet ground*/
  float soilMoisture = 100;
  float temperature = 100;
  bool watering = false;
};
extern float auxTemp;
extern float auxHumidity;
extern struct tm currentTime;
extern bool wrtWater;
extern readControl Rx;
extern int currentHour;
extern uint8_t rgb_state;
extern uint8_t nmbr_outputs;
extern uint32_t current;
#endif