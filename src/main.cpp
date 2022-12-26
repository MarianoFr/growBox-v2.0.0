/*******************************************************************************************
  Complete code to implement GrowBox, storing variables in the FireBase real time Data Base
  15/11/2022
  By Mariano Miguel Franceschetti
  NOTE: once finished all serial debug comms wil be errased!!!
* ******************************************************************************************/
#include <Arduino.h>
#include "debug_flags.h"
#include <esp_task_wdt.h>
#include "WiFi.h"
#include <OneWire.h>
#include <DHT.h>//DHT ambient humidity and temperature sensor
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

//RGB errors
#define NO_WIFI_CRED  0
#define WIFI_CONN     1
#define WIFI_DISC     2
#define NO_USER       3
#define DHT_ERR       4
#define RTC_ERR       5
#define SOIL_ERR      6
#define BH_1750_ERR   7

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
#define DHTTYPE   DHT22
/* Iniatiates DHT Unified sensor
  DHT_Unified dht(DHTPIN, DHTTYPE);
  Initiates generic DHT sensor*/
DHT dht(DHTPIN, DHTTYPE);

/* RTC configuration and variables */
ESP32Time rtc;
struct tm currentTime;
bool rtc_adjusted = false;
bool rtc_began = false;
bool wrtWater = false;
readControl Rx;
writeControl Tx;
int currentHour = 0;
QueueHandle_t writeQueue;
QueueHandle_t waterQueue;
float auxTemp=0;
float auxHumidity=0;

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

  String users = "users/";
  String GBconnected = "/GBconnected/";
  String use = "/user/";
  String gbs = "growboxs/";

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
  
#if SERIAL_DEBUG        
  Serial.begin(115200);
#endif
  //disableCore0WDT();
  //disableLoopWDT();
  vTaskDelay(100/portTICK_PERIOD_MS);
  EEPROM.begin(512);
  vTaskDelay(100/portTICK_PERIOD_MS);
  
  // Start up the library for DHT11
  dht.begin();
  vTaskDelay(100/portTICK_PERIOD_MS);
  // Start up the library for BH1750
  if(!Wire.begin())
  {
    RGBalert(BH_1750_ERR);
  }
  /*
  Full mode list:
      BH1750_CONTINUOUS_LOW_RES_MODE
      BH1750_CONTINUOUS_HIGH_RES_MODE (default)
      BH1750_CONTINUOUS_HIGH_RES_MODE_2
      BH1750_ONE_TIME_LOW_RES_MODE
      BH1750_ONE_TIME_HIGH_RES_MODE
      BH1750_ONE_TIME_HIGH_RES_MODE_2
  */
  vTaskDelay(100/portTICK_PERIOD_MS);
  if (lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE)) {
    #if SERIAL_DEBUG && BH_DEBUG
    Serial.println(F("BH1750 Advanced begin"));
    #endif
  } else {
    #if SERIAL_DEBUG && BH_DEBUG
    Serial.println(F("Error initialising BH1750"));
    #endif
  }
  vTaskDelay(100/portTICK_PERIOD_MS);
    
  if (checkWiFiCredentials())
  {
    if (connectWifi())
    {
      /*Setup Firebase credentials in setup(), that is c onnect to FireBase.
        Two parameters are necessary: the firebase_url and the firebase_API_key*/
      Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
      Firebase.reconnectWiFi(true);
      /*Set database read timeout to 1 minute (max 15 minutes)*/
      Firebase.setReadTimeout(firebaseData2, 1000 * 60 );
      /*Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).*/
      Firebase.setwriteSizeLimit(firebaseData2, "medium ");
      /*Get from DB the user's uid which corresponds to this GrowBox*/
      Firebase.setBool(firebaseData2, users + mac + GBconnected, true);
      char aux_1[100];
      char mac_char[100];
      mac.toCharArray(mac_char, mac.length()+1);
      sprintf(aux_1,"users/%s/user/", mac_char);    
      if (!Firebase.getString(firebaseData2, aux_1, users_uid))
      {
        Firebase.setString(firebaseData2, users + mac + users, "");
      }
      while ( users_uid == "" )
      {
        RGBalert(NO_USER);
        Firebase.getString(firebaseData2, users + mac + use, users_uid);
        debounceWiFiReset();
      }
      path = gbs + String(users_uid);
      
      /*Init the NTP library*/
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      if (getLocalTime(&currentTime))
      {
        currentHour = currentTime.tm_hour;
#if USE_RTC
#if SERIAL_DEBUG && RTC_DEBUG
        Serial.println("Adjusting");
#endif
        rtc.setTime(currentTime.tm_sec, currentTime.tm_min, currentTime.tm_hour, currentTime.tm_mday, currentTime.tm_mon + 1, currentTime.tm_year + 1900);
        vTaskDelay(100/portTICK_PERIOD_MS);
        tm now = rtc.getTimeStruct();
        char buf2[20];
        strftime(buf2, 20,"%Y-%m-%d %X",&now); 
#if SERIAL_DEBUG && RTC_DEBUG
        Serial.println(String(buf2));
#endif
        Tx.ESPtag = String(buf2);
        Firebase.setString(firebaseData2, path + "/dashboard/FirstConnection", Tx.ESPtag);
#endif
      }
      int nmbrResets = 0;
      char aux_2[100];
      char path_char[100];
      path.toCharArray(path_char, path.length()+1);
      sprintf(aux_2, "%s/dashboard/NmbrResets", path_char);
      Firebase.getInt(firebaseData2, aux_2, &nmbrResets);
      nmbrResets++;
      Firebase.setInt(firebaseData2, path + "/dashboard/NmbrResets", nmbrResets);
    }
  }
  else {
#if SERIAL_DEBUG
    Serial.println("Server started");
#endif
    RGBalert(NO_WIFI_CRED);
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
  static writeControl tx;
  debounceWiFiReset();
  //Adjust rtc with NTP
  if (!gettingWiFiCredentials && (WiFi.status() == WL_CONNECTED))
  {
    struct tm t;
#if USE_RTC    
    if(currentHour == 4)
    {
      if (!getLocalTime(&t))
      {
#if SERIAL_DEBUG && TIME_DEBUG
        Serial.println("getLocalTime Error");
#endif  
        //NTP error RGBalert
      }
      else
      {
        rtc.setTime(t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
        vTaskDelay(100/portTICK_PERIOD_MS);
      #if SERIAL_DEBUG && RTC_DEBUG
        Serial.println("¡¡¡¡¡RTC adjusted!!!!!");
      #endif
        rtc_adjusted = true;
      }
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
#endif
  }
  //Attend sensors and create ESPtag
  if(!gettingWiFiCredentials)
  {
#if USE_RTC
    vTaskDelay(250/portTICK_PERIOD_MS);
    tm now = rtc.getTimeStruct();
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
#endif

    analogSoilRead ( &Rx, &tx );    
    TemperatureHumidityHandling ( &Rx, &tx, currentHour );    
    PhotoPeriod ( &Rx, &tx, currentHour );    
    
    static unsigned long writePeriod = 3000;
    static unsigned long previousWrite = 0;
    if ((unsigned long)(millis() - previousWrite) > writePeriod)
    {
      ReadBH1750(&tx);
      auxTemp = dht.readTemperature();
      auxHumidity = dht.readHumidity();
      xQueueSend(writeQueue, &tx, 0);
      previousWrite += writePeriod;
    }
  }
  
}