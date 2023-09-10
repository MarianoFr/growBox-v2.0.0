/*******************************************************************************************
  Complete code to implement GrowBox, storing variables in the FireBase real time Data Base
  15/11/2022
  By Mariano Miguel Franceschetti
  NOTE: once finished all serial debug comms wil be errased!!!
* ******************************************************************************************/
#include <Arduino.h>
#include "debug_flags.h"
//#include <esp_task_wdt.h>
#include "WiFi.h"
#include <OneWire.h>
#include "DHT.hpp"//DHT ambient humidity and temperature sensor
#include <time.h>//Time library, getLocalTime
#include <Wire.h>
#include <ESP32Time.h>
#include <analogWrite.h>
#include "gb_local_server.h"
#include "write_mem.h"
#include "rgb_rst_butt.h"
#include "globals.h"
#include "fb_mgmt.h"
#include "gb_sensors_outputs.h"

TaskHandle_t wiFiHandler;//Task to handle wifi in core 0

/* Flag to detect whether user's WiFi credentials
    are stored in FLASH memory or not and
    adresses where they will be stored */
#define WIFI_CRED_FLAG 0
#define NO_CREDENTIALS 0
#define WITH_CREDENTIALS 1
#define SSID_STORING_ADD 1
#define PASS_STORING_ADD 51

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
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = 0;
/*Sensors definitions and declarations*/

/*Ambient humidity and temperature sensor*/
#define DHTPIN    19
/* #define DHTTYPE   DHT22
/* Iniatiates DHT Unified sensor
  DHT_Unified dht(DHTPIN, DHTTYPE);
  Initiates generic DHT sensor*/
/*DHT dht(DHTPIN, DHTTYPE); */
DHT dht;
TimerHandle_t xRgbTimer;
/* RTC configuration and variables */
ESP32Time rtc;
bool rtc_calibrated = false;
struct tm currentTime;
bool wrtWater = false;
readControl Rx;
int currentHour = 0;
QueueHandle_t writeQueue;
QueueHandle_t waterQueue;
float auxTemp=0;
float auxHumidity=0;
uint8_t rgb_state = 1;
uint8_t nmbr_outputs = 0;
uint32_t current = 0;
uint32_t writePeriod = 3000;
uint32_t espTagPeriod = 5*60000;
uint32_t previousEspTag = 0;
uint32_t previousWrite = 0;
uint32_t previousDHTRead = 0;
uint32_t dhtPeriod = 2000;
static writeControl tx;
uint8_t retry = 0;
/***********************************
 * Dht timed read
 ***********************************/
/* static void ReadDht(TimerHandle_t xTimer) 
{
  dht.readDHT();
  auxTemp = dht.getTemperature();
  auxHumidity = dht.getHumidity();
  #if SERIAL_DEBUG && DHT_DEBUG
  Serial.println(auxTemp);
  Serial.println(auxHumidity);
  #endif 
}   */
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
    if((rgb_state >> WIFI_DISC) & 1U)
    {
        green = 0;red = 0; blue = 0;        
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Negro");
        Serial.println("WIFI_DISC");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> WIFI_CONN) & 1U)
    {
        green = 0;red = 35; blue = 255; 
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Violeta");
        Serial.println("WIFI_CONN");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> NO_WIFI_CRED) & 1U)
    {
        green = 0;red = 255; blue = 0;
    #if SERIAL_DEBUG && RGB_DEBUG    
        Serial.println("Rojo");
        Serial.println("NO_WIFI_CRED");//Verde a tope
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> NO_USER) & 1U)
    {
        green = 255;red = 0; blue = 0;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Verde");
        Serial.println("NO_USER");//
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> DHT_ERR) & 1U)
    {
        green = 0;red = 0; blue = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Azul");
        Serial.println("DHT_ERR");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> RTC_ERR) & 1U)
    {
        green = 255;red = 255; blue = 50;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Amarillo");
        Serial.println("RTC_ERR");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> SOIL_ERR) & 1U)
    {        
        green = 100;red = 120; blue = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Blanco");
        Serial.println("SOIL_ERR");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if((rgb_state >> BH_1750_ERR) & 1U)
    {
        green = 0;red = 150; blue = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("Rosa");
        Serial.println("BH_1750_ERR");
    #endif
      analogWrite(PIN_GREEN, green);
      analogWrite(PIN_BLUE, blue);
      analogWrite(PIN_RED, red);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    return;
}

 /***********************************
 * wifi core 0 tasks prototype
 * input pvParameters
 * out none
 ***********************************/                       
void wiFiTasks( void * pvParameters );

/*****************************************************************************/
// Set-up config for our ESP32: serial, WiFi, initiates OneWire comms, GPIOs //
/*****************************************************************************/
void setup() {

  String gbs = "growboxs/";
  char aux_1[100];
  char mac_char[100]; 
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode( SOILPIN, INPUT );
  pinMode( W_VLV_PIN, OUTPUT );
  digitalWrite( W_VLV_PIN, HIGH );
  pinMode( HUM_CTRL_OUTPUT, OUTPUT );
  digitalWrite( HUM_CTRL_OUTPUT, HIGH );
  pinMode( TEMP_CTRL_OUTPUT, OUTPUT );
  digitalWrite( TEMP_CTRL_OUTPUT, HIGH );
  pinMode( LIGHTS, OUTPUT );
  digitalWrite( LIGHTS, HIGH );
  pinMode(WIFI_RESET, INPUT);
  digitalWrite(WIFI_RESET, LOW);

  rgb_state |= 1UL << RTC_ERR;

  xRgbTimer = xTimerCreate( "Scan timer", pdMS_TO_TICKS(1000), pdTRUE, ( void * ) 0, RGBalert );
  configASSERT( xRgbTimer );
	configASSERT( xTimerStart(xRgbTimer,pdMS_TO_TICKS(100)) );

  /* xDhtTimer = xTimerCreate( "Dht timer", pdMS_TO_TICKS(2000), pdTRUE, ( void * ) 0, ReadDht );
  configASSERT( xDhtTimer );
	configASSERT( xTimerStart(xDhtTimer,pdMS_TO_TICKS(100)) ); */
 
  
#if SERIAL_DEBUG        
  Serial.begin(115200);
#endif
  //disableCore0WDT();
  //disableLoopWDT();
  vTaskDelay(100/portTICK_PERIOD_MS);
  EEPROM.begin(512);
  vTaskDelay(100/portTICK_PERIOD_MS);
  dht.setDHTgpio((gpio_num_t)DHTPIN);
  vTaskDelay(100/portTICK_PERIOD_MS);
  if(!Wire.begin())
  {
    rgb_state |= 1UL << BH_1750_ERR;
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
  uint8_t bh1750_tries = 0;
  while (!lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE) && (bh1750_tries < 5)) {
    rgb_state |= 1UL << BH_1750_ERR;    
    #if SERIAL_DEBUG && BH_DEBUG
    Serial.println(rgb_state);
    Serial.println("Error initialising BH1750");
    #endif
    bh1750_tries++;
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
  if(bh1750_tries < 5) {
    #if SERIAL_DEBUG && BH_DEBUG
    Serial.println("BH1750 Advanced begin");
    #endif
    rgb_state &= ~(1UL << BH_1750_ERR);
  }
  bh1750_tries=0;
  if (checkWiFiCredentials())
  {
    if (connectWifi())
    {
      /*Setup Firebase credentials in setup(), that is c onnect to FireBase.
        Two parameters are necessary: the firebase_url and the firebase_API_key*/
      rgb_state |= 1UL << NO_USER;
      Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
      Firebase.reconnectWiFi(true);
      /*Set database read timeout to 1 minute (max 15 minutes)*/
      Firebase.setReadTimeout(firebaseData2, 1000 * 60 );
      /*Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).*/
      Firebase.setwriteSizeLimit(firebaseData2, "medium");
      /*Get from DB the user's uid which corresponds to this GrowBox*/
      mac.toCharArray(mac_char, mac.length()+1);
      sprintf(aux_1,"users/%s/GBconnected/", mac_char);   
      Firebase.setBool(firebaseData2, aux_1, true);
      sprintf(aux_1,"users/%s/user/", mac_char);
      if (!Firebase.getString(firebaseData2, aux_1, users_uid))
      {
        Firebase.setString(firebaseData2, aux_1, "NULL");
        rgb_state |= 1UL << NO_USER;
        sprintf(users_uid,"%s","NULL");
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
      vTaskDelay(10/portTICK_PERIOD_MS);
      tm now = rtc.getTimeStruct();
      char buf2[20];
      strftime(buf2, 20,"%Y-%m-%d %X",&now); 
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
      path.toCharArray(path_char, path.length()+1);
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
  else {
#if SERIAL_DEBUG
    Serial.println("Server started");
#endif
    rgb_state |= 1UL << NO_WIFI_CRED;
    rgb_state |= 1UL << RTC_ERR;//RTC must be calibrated with internet
    setupServer();
    gettingWiFiCredentials = true;
  }

  writeQueue = xQueueCreate(1, sizeof(writeControl));
  waterQueue = xQueueCreate(1, sizeof(bool));

  xTaskCreatePinnedToCore(
    wiFiTasks, /* Function to implement the task */
    "wiFiTasks", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    0,  /* Priority of the task */
    &wiFiHandler,  /* Task handle. */
    0); /* Core where the task should run */
    
}

void loop()
{
  static tm now;
  debounceWiFiReset();
  current = millis();
  nmbr_outputs = 0;
  if(tx.lights)
    nmbr_outputs++;
  if(tx.watering)
    nmbr_outputs++;
  if(tx.temperatureControlOn)
    nmbr_outputs++;
  if(tx.humidityControlOn)
    nmbr_outputs++;
#if USE_RTC
  //************* Start If Wifi connected ******************
  if (!gettingWiFiCredentials && (WiFi.status() == WL_CONNECTED))
  {  //Start create ESPTAG
    if ( (uint32_t)(current - previousEspTag) > espTagPeriod )
    { 
      previousEspTag = current;
      now = rtc.getTimeStruct();
      char buf2[20];
      strftime(buf2, 20,"%Y-%m-%d %X",&now);
    #if SERIAL_DEBUG && RTC_DEBUG
      Serial.println(String(buf2));
    #endif
      tx.ESPtag = String(buf2);
      currentHour = now.tm_hour;
    #if SERIAL_DEBUG && TIME_DEBUG
      Serial.println("Current hour:" + String(currentHour));
    #endif
    }//End create ESPTAG
    //************* Start Calibrate RTC ******************        
    if((currentHour == 4 || ((rgb_state >> RTC_ERR) & 1U)) & ~(rtc_calibrated))
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
        vTaskDelay(100/portTICK_PERIOD_MS);
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
  }//************* End If Wifi connected ******************
#endif
  //Attend sensors and create ESPtag
  if(!gettingWiFiCredentials && !((rgb_state >> RTC_ERR) & 1U))
  {
    analogSoilRead ( &Rx, &tx );    
    TemperatureHumidityHandling ( &Rx, &tx, currentHour );    
    PhotoPeriod ( &Rx, &tx, currentHour );

    if ((uint32_t)(current - previousDHTRead) > dhtPeriod)
    {
      dht.readDHT();
      auxTemp = dht.getTemperature();
      auxHumidity = dht.getHumidity(),
      previousDHTRead = current;
      #if SERIAL_DEBUG && DHT_DEBUG
      Serial.println(auxTemp);
      Serial.println(auxHumidity);
      #endif
    }    
  }  
}