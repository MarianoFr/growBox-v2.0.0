/*******************************************************************************************
  Complete code to implement GrowBox, storing variables in the FireBase real time Data Base
  15/11/2022
  By Mariano Miguel Franceschetti
  NOTE: once finished all serial debug comms wil be errased!!!
* ******************************************************************************************/
#include <Arduino.h>
#include "debug_flags.h"
// #include <esp_task_wdt.h>
#include "WiFi.h"
#include <OneWire.h>
// #include "DHT.hpp"//DHT ambient humidity and temperature sensor
#include <time.h> //Time library, getLocalTime
#include <Wire.h>
#include <ESP32Time.h>
#include <analogWrite.h>
#include "gb_local_server.h"
#include "write_mem.h"
#include "rgb_rst_butt.h"
#include "globals.h"
#include "fb_mgmt.h"
#include "gb_sensors_outputs.h"
#include "driver/gpio.h"
#include "driver/uart.h"

TaskHandle_t wiFiHandler; // Task to handle wifi in core 0

/* Flag to detect whether user's WiFi credentials
    are stored in FLASH memory or not and
    adresses where they will be stored */
#define WIFI_CRED_FLAG 0
#define NO_CREDENTIALS 0
#define WITH_CREDENTIALS 1
#define SSID_STORING_ADD 1
#define PASS_STORING_ADD 51

#define DHT_SLEEP 0
#define DHT_WAKING 1
#define DHT_SENDING 3
#define DHT_WAIT_HIGH 4
#define DHT_WAIT_LOW 5
#define DHT_END_TX 6

/*
   Mac path. When logging a user to the app for the first time, the movile
   creates children in the DB, named after the MAC address input by the user,
   its value being equal to the users uid. Then we turn on the GB which will look
   for the children named after its own name and gets the user uid stored in this
   children. With this info, the GB loads the dashboard in a directory parent named
   after the user uid.
*/
String mac = WiFi.macAddress();
String path = "";
/*Input variables for getLocalTime.*/
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;
/*Sensors definitions and declarations*/

/*Ambient humidity and temperature sensor*/
#define DHTPIN GPIO_NUM_19
/* #define DHTTYPE   DHT22
/* Iniatiates DHT Unified sensor
  DHT_Unified dht(DHTPIN, DHTTYPE);
  Initiates generic DHT sensor*/
/*DHT dht(DHTPIN, DHTTYPE); */
// DHT dht;
SemaphoreHandle_t xDhtWiFiSemaphore;
TimerHandle_t xRgbTimer;
TimerHandle_t xDhtTimer;
/* RTC configuration and variables */
ESP32Time rtc;
bool rtc_calibrated = false;
struct tm currentTime;
bool wrtWater = false;
readControl Rx;
int currentHour = 0;
QueueHandle_t writeQueue;
QueueHandle_t waterQueue;
// QueueHandle_t dhtQueue;
float auxTemp = 0;
float auxHumidity = 0;
uint8_t rgb_state = 1;
uint8_t nmbr_outputs = 0;
uint32_t current = 0;
uint32_t writePeriod = 3000;
uint32_t espTagPeriod = 5 * 60000;
uint32_t previousEspTag = 0;
uint32_t previousWrite = 0;
uint32_t previousDHT = 0;
static writeControl tx;
uint8_t retry = 0;
uint32_t v0 = 0;
uint32_t v05 = 0;
volatile uint8_t dht_state = DHT_SLEEP;
volatile int64_t dht_times[100];
uint8_t dhtData[5];
uint8_t dht_byteInx = 0;
uint8_t dht_bitInx = 7;
volatile bool dht_ready = false;
volatile uint16_t passed = 0;
/*get DHT level within irq*/
void IRAM_ATTR DHT_ISR()
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  dht_times[passed++] = esp_timer_get_time();
  if (passed == 85)
  {    
    dht_ready = true;
    detachInterrupt((uint8_t)DHTPIN);
    xSemaphoreGiveFromISR(xDhtWiFiSemaphore, &xHigherPriorityTaskWoken);
  }
}
/*Parse dht data*/
int8_t parseDht()
{
  for (int k = 6; k < 85; k = k + 2)
  {
    if (dht_times[k] - dht_times[k - 1] > 40)
      dhtData[dht_byteInx] |= (1 << dht_bitInx);
    if (dht_bitInx == 0)
    {
      dht_bitInx = 7;
      ++dht_byteInx;
    }
    else
      dht_bitInx--;
  }
  // == get hum from Data[0] and Data[1]
  dht_byteInx = 0;
  dht_bitInx = 7;
  auxHumidity = dhtData[0];
  auxHumidity *= 0x100; // >> 8
  auxHumidity += dhtData[1];
  auxHumidity /= 10; // get the decimal

  // == get temp from Data[2] and Data[3]
  auxTemp = dhtData[2] & 0x7F;
  auxTemp *= 0x100; // >> 8
  auxTemp += dhtData[3];
  auxTemp /= 10;
  if (dhtData[2] & 0x80) // negative temp, brrr it's freezing
    auxTemp *= -1;

  // == verify if checksum is ok ===========================================
  // Checksum is the sum of Data 8 bits masked out 0xFF
  if (dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF))
    return DHT_OK;
  else
    return DHT_CHECKSUM_ERROR;
}
/***********************************
 * DHT timer start
 ***********************************/
void dhtCheck(TimerHandle_t xTimer)
{
  static gpio_num_t DHTgpio = DHTPIN;
  esp_err_t ert;
  switch (dht_state)
  {
  case DHT_SLEEP:
    if (!gettingWiFiCredentials && WiFi.status() == WL_CONNECTED)
    {
      WiFi.disconnect();
      rgb_state &= ~(1UL << WIFI_CONN);
      rgb_state |= 1UL << WIFI_DISC;
      //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    attachInterrupt((uint8_t)DHTPIN, DHT_ISR, CHANGE);
    passed = 0;
    for (uint8_t k = 0; k < 100; k++)
    {
      dht_times[k] = 0;
    }
    for (uint8_t k = 0; k < 5; k++)
      dhtData[k] = 0;
    gpio_set_direction(DHTgpio, GPIO_MODE_OUTPUT);
    gpio_set_level(DHTgpio, 0);
    dht_state = DHT_WAKING;
    configASSERT(xTimerChangePeriod(xDhtTimer, DHT_WAKE_ORDER_PERIOD / portTICK_PERIOD_MS, 100 / portTICK_PERIOD_MS));
    break;
  case DHT_WAKING:
    dht_state = DHT_WAIT_HIGH;
    // configASSERT(xTimerStop(xDhtTimer, 100 / portTICK_PERIOD_MS));
    configASSERT(xTimerChangePeriod(xDhtTimer, DHT_SENSE_PERIOD / portTICK_PERIOD_MS, 100 / portTICK_PERIOD_MS));
    gpio_set_level(DHTgpio, 1);
    gpio_set_direction(DHTgpio, GPIO_MODE_INPUT);
    break;
  default:
    break;
  }
}

/***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
static void RGBalert(TimerHandle_t xTimer)
{
  static uint8_t green = 100, blue = 0, red = 120;
#if SERIAL_DEBUG && RGB_DEBUG
  Serial.println(rgb_state);
#endif
  char aux_2[100];
  char path_char[100];
  static uint8_t try_out = 0;
  switch (try_out)
  {
  case WIFI_DISC:
    if ((rgb_state >> WIFI_DISC) & 1U)
    {
      green = 0;
      red = 0;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Negro");
      Serial.println("WIFI_DISC");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case WIFI_CONN:
    if ((rgb_state >> WIFI_CONN) & 1U)
    {
      green = 0;
      red = 35;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Violeta");
      Serial.println("WIFI_CONN");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case NO_WIFI_CRED:
    if ((rgb_state >> NO_WIFI_CRED) & 1U)
    {
      green = 0;
      red = 255;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Rojo");
      Serial.println("NO_WIFI_CRED"); // Verde a tope
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case NO_USER:
    if ((rgb_state >> NO_USER) & 1U)
    {
      green = 255;
      red = 0;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Verde");
      Serial.println("NO_USER"); //
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case DHT_ERR:
    if ((rgb_state >> DHT_ERR) & 1U)
    {
      green = 0;
      red = 0;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Azul");
      Serial.println("DHT_ERR");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case RTC_ERR:
    if ((rgb_state >> RTC_ERR) & 1U)
    {
      green = 255;
      red = 255;
      blue = 50;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Amarillo");
      Serial.println("RTC_ERR");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case SOIL_ERR:
    if ((rgb_state >> SOIL_ERR) & 1U)
    {
      green = 100;
      red = 120;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Blanco");
      Serial.println("SOIL_ERR");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  case BH_1750_ERR:
    if ((rgb_state >> BH_1750_ERR) & 1U)
    {
      green = 0;
      red = 150;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      Serial.println("Rosa");
      Serial.println("BH_1750_ERR");
#endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
    }
    break;
  default:
    break;
  }
  try_out++;
  if (try_out > 7)
    try_out = 0;
  return;
}

/***********************************
 * wifi core 0 tasks prototype
 * input pvParameters
 * out none
 ***********************************/
void wiFiTasks(void *pvParameters);

/*****************************************************************************/
// Set-up config for our ESP32: serial, WiFi, initiates OneWire comms, GPIOs //
/*****************************************************************************/
void setup()
{
  String gbs = "growboxs/";
  char aux_1[100];
  char mac_char[100];
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(SOILPIN, INPUT);
  pinMode(W_VLV_PIN, OUTPUT);
  digitalWrite(W_VLV_PIN, HIGH);
  pinMode(HUM_CTRL_OUTPUT, OUTPUT);
  digitalWrite(HUM_CTRL_OUTPUT, HIGH);
  pinMode(TEMP_CTRL_OUTPUT, OUTPUT);
  digitalWrite(TEMP_CTRL_OUTPUT, HIGH);
  pinMode(LIGHTS, OUTPUT);
  digitalWrite(LIGHTS, HIGH);
  pinMode(WIFI_RESET, INPUT);
  digitalWrite(WIFI_RESET, LOW);
  rgb_state |= 1UL << RTC_ERR;

  xRgbTimer = xTimerCreate("RGB timer", 500 / portTICK_PERIOD_MS, pdTRUE, (void *)0, RGBalert);
  configASSERT(xRgbTimer);
  configASSERT(xTimerStart(xRgbTimer, 10 / portTICK_PERIOD_MS));

  // DHT works only when we have wifi connection, so as to not stop the server qhen receiving ssid and password
  xDhtTimer = xTimerCreate("DHT timer", DHT_SENSE_PERIOD / portTICK_PERIOD_MS, pdTRUE, (void *)0, dhtCheck);
  configASSERT(xDhtTimer);
  configASSERT(xTimerStart(xDhtTimer, 100 / portTICK_PERIOD_MS));

  xDhtWiFiSemaphore = xSemaphoreCreateBinary();
  configASSERT(xDhtWiFiSemaphore);
#if SERIAL_DEBUG
  Serial.begin(115200);
  #else
  uart_driver_delete(UART_NUM_0);
#endif
  // disableCore0WDT();
  // disableLoopWDT();
  EEPROM.begin(512);
  if (!Wire.begin())
  {
    rgb_state |= 1UL << BH_1750_ERR;
  }
  uint8_t bh1750_tries = 0;
  while (!lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE) && (bh1750_tries < 5))
  {
    rgb_state |= 1UL << BH_1750_ERR;
#if SERIAL_DEBUG && BH_DEBUG
    Serial.println(rgb_state);
    Serial.println("Error initialising BH1750");
#endif
    bh1750_tries++;
  }
  if (bh1750_tries < 5)
  {
#if SERIAL_DEBUG && BH_DEBUG
    Serial.println("BH1750 Advanced begin");
#endif
    rgb_state &= ~(1UL << BH_1750_ERR);
  }
  bh1750_tries = 0;

  v0 = EEPROM.readUInt(V0_SOIL);
  v05 = EEPROM.readUInt(V05_SOIL);

  if (checkWiFiCredentials())
  {
    if (connectWifi())
    {
      /*Setup Firebase credentials in setup(), that is c onnect to FireBase.
        Two parameters are necessary: the firebase_url and the firebase_API_key*/
      rgb_state |= 1UL << NO_USER;
      Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
      Firebase.reconnectWiFi(false);
      /*Set database read timeout to 1 minute (max 15 minutes)*/
      Firebase.setReadTimeout(firebaseData2, 1000 * 60);
      /*Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).*/
      Firebase.setwriteSizeLimit(firebaseData2, "medium");
      /*Get from DB the user's uid which corresponds to this GrowBox*/
      mac.toCharArray(mac_char, mac.length() + 1);
      sprintf(aux_1, "users/%s/GBconnected/", mac_char);
      Firebase.setBool(firebaseData2, aux_1, true);
      sprintf(aux_1, "users/%s/user/", mac_char);
      if (!Firebase.getString(firebaseData2, aux_1, users_uid))
      {
        Firebase.setString(firebaseData2, aux_1, "NULL");
        rgb_state |= 1UL << NO_USER;
        sprintf(users_uid, "%s", "NULL");
      }
      while (strcmp(users_uid, "NULL") == 0)
      {
        Firebase.getString(firebaseData2, aux_1, users_uid);
        debounceWiFiReset();
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      rgb_state &= ~(1UL << NO_USER);
      path = gbs + String(users_uid);
      /*Init the NTP library*/
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      while (!getLocalTime(&currentTime) && (retry < NTP_RETRY))
      {
        retry++;
      }
      currentHour = currentTime.tm_hour;
#if USE_RTC
#if SERIAL_DEBUG && RTC_DEBUG
      Serial.println("Adjusting");
#endif
      rtc.setTime(currentTime.tm_sec, currentTime.tm_min, currentTime.tm_hour, currentTime.tm_mday, currentTime.tm_mon + 1, currentTime.tm_year + 1900);
      tm now = rtc.getTimeStruct();
      char buf2[20];
      strftime(buf2, 20, "%Y-%m-%d %X", &now);
#if SERIAL_DEBUG && RTC_DEBUG
      Serial.println(String(buf2));
#endif
      tx.ESPtag = String(buf2);
      Firebase.setString(firebaseData2, path + "/dashboard/FirstConnection", tx.ESPtag);
#endif
      rgb_state &= ~(1UL << RTC_ERR);
      int nmbrResets = 0;
      char aux_2[100];
      char path_char[100];
      path.toCharArray(path_char, path.length() + 1);
      sprintf(aux_2, "%s/dashboard/NmbrResets", path_char);
      Firebase.getInt(firebaseData2, aux_2, &nmbrResets);
      nmbrResets++;
      Firebase.setInt(firebaseData2, path + "/dashboard/NmbrResets", nmbrResets);
    }
    else
    {
      rgb_state |= 1UL << RTC_ERR;
      rgb_state |= 1UL << WIFI_DISC;
    }
  }
  else
  {
#if SERIAL_DEBUG
    Serial.println("Server started");
#endif
    rgb_state |= 1UL << NO_WIFI_CRED;
    rgb_state |= 1UL << RTC_ERR; // RTC must be calibrated with internet
    setupServer();
    gettingWiFiCredentials = true;
  }

  writeQueue = xQueueCreate(1, sizeof(writeControl));
  waterQueue = xQueueCreate(1, sizeof(bool));

  xTaskCreatePinnedToCore(
      wiFiTasks,    /* Function to implement the task */
      "wiFiTasks",  /* Name of the task */
      10000,        /* Stack size in words */
      NULL,         /* Task input parameter */
      0,            /* Priority of the task */
      &wiFiHandler, /* Task handle. */
      0);           /* Core where the task should run */
  vTaskDelay(1 / portTICK_PERIOD_MS);
}

void loop()
{
  static tm now;
  debounceWiFiReset();
  current = millis();
  nmbr_outputs = 0;
  if (tx.lights)
    nmbr_outputs++;
  if (tx.watering)
    nmbr_outputs++;
  if (tx.temperatureControlOn)
    nmbr_outputs++;
  if (tx.humidityControlOn)
    nmbr_outputs++;
#if USE_RTC
  //************* Start If Wifi connected ******************
  if (!gettingWiFiCredentials && (WiFi.status() == WL_CONNECTED))
  { // Start create ESPTAG
    if ((uint32_t)(current - previousEspTag) > espTagPeriod)
    {
      previousEspTag = current;
      now = rtc.getTimeStruct();
      char buf2[20];
      strftime(buf2, 20, "%Y-%m-%d %X", &now);
#if SERIAL_DEBUG && RTC_DEBUG
      Serial.println(String(buf2));
#endif
      tx.ESPtag = String(buf2);
      currentHour = now.tm_hour;
#if SERIAL_DEBUG && TIME_DEBUG
      Serial.println("Current hour:" + String(currentHour));
#endif
    } // End create ESPTAG
    //************* Start Calibrate RTC ******************
    if ((currentHour == 4 || ((rgb_state >> RTC_ERR) & 1U)) & ~(rtc_calibrated))
    {
      rtc_calibrated = true;
      if (!getLocalTime(&now))
      {
#if SERIAL_DEBUG && TIME_DEBUG
        Serial.println("getLocalTime Error");
#endif
        rgb_state |= 1UL << RTC_ERR;
      }
      else
      {
        rtc.setTime(now.tm_sec, now.tm_min, now.tm_hour, now.tm_mday, now.tm_mon + 1, now.tm_year + 1900);
        rgb_state &= ~(1UL << RTC_ERR);
#if SERIAL_DEBUG && RTC_DEBUG
        Serial.println("¡¡¡¡¡RTC adjusted!!!!!");
#endif
      }
    }
    else if (currentHour > 4)
    {
      rtc_calibrated = false;
    }
    //************* End Calibrate RTC ******************
    //************* Start Update FB dashboard ******************
    if ((uint32_t)(current - previousWrite) > writePeriod)
    {
      ReadBH1750(&tx);
      xQueueSend(writeQueue, &tx, 0);
      previousWrite += writePeriod;
    }
    //************* End Update FB dashboard ******************
  } //************* End If Wifi connected ******************
#endif
  // Attend sensors and create ESPtag
  if (!gettingWiFiCredentials && !((rgb_state >> RTC_ERR) & 1U))
  {
    analogSoilRead(&Rx, &tx);
    TemperatureHumidityHandling(&Rx, &tx, currentHour);
    PhotoPeriod(&Rx, &tx, currentHour);

    /* if ((uint32_t)(current - previousDHTRead) > dhtPeriod)
    {
      int8_t err = 0;
      err = dht.readDHT();
      if(err == DHT_OK)
      {
        auxTemp = dht.getTemperature();
        auxHumidity = dht.getHumidity();
      }
      previousDHTRead = current;
      #if SERIAL_DEBUG && DHT_DEBUG
      Serial.println(auxTemp);
      Serial.println(auxHumidity);
      Serial.println(err);
      #endif
    }  */
  }
  if (dht_ready)
  {
    previousDHT = current;
    int8_t err = parseDht();
    if (err == DHT_OK)
    {
      tx.humidity = auxHumidity;
      tx.temperature = auxTemp;
    }
#if SERIAL_DEBUG && DHT_DEBUG
    /* for(uint8_t k = 0; k < 84; k++)
    {
      Serial.print(dht_times[k]);
      Serial.print("-");
    }
    Serial.println(dht_times[84]);
    for(uint8_t k = 0; k < 84; k++)
    {
      Serial.print(dht_times[k+1]-dht_times[k]);
      Serial.print("-");
    } */
    Serial.println(auxTemp);
    Serial.println(auxHumidity);
    Serial.println(err);
    Serial.println(passed);
#endif
    configASSERT(xTimerChangePeriod(xDhtTimer, DHT_SENSE_PERIOD / portTICK_PERIOD_MS, 100 / portTICK_PERIOD_MS));
    dht_ready = false;
    passed = 0;
    dht_state = DHT_SLEEP;
  }
}