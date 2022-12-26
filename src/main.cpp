/*******************************************************************************************
  Complete code to implement GrowBox, storing variables in the FireBase real time Data Base
  15/11/2022
  By Mariano Miguel Franceschetti
  NOTE: once finished all serial debug comms wil be errased!!!
* ******************************************************************************************/
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "WebServer.h"
#include <EEPROM.h>
#include "WiFi.h"
#include "FirebaseESP32.h"
#include <OneWire.h>
#include <DHT.h>//DHT ambient humidity and temperature sensor
#include <time.h>//Time library, getLocalTime
#include <BH1750.h>//Include the library for light sensorsetQuality
#include <Wire.h>
#include <ESP32Time.h>
#include <analogWrite.h>

#define USE_RTC 1

#define SERIAL_DEBUG 0
#define DEBUG_RGB 0
#define RTC_DEBUG 0
#define TIME_DEBUG  0
#define FIRE_DEBUG  0
#define ESPtag_DEBUG 0
#define DHT_DEBUG 0
#define BH_DEBUG 0
#define SOIL_DEBUG 0
#define WIFI_DEBUG 0

//RGB errors
#define NO_WIFI_CRED  0
#define WIFI_CONN     1
#define WIFI_DISC     2
#define NO_USER       3
#define DHT_ERR       4
#define RTC_ERR       5
#define SOIL_ERR      6
#define BH_1750_ERR   7
/* All lines corresponding to the use of the DS18B20 
    temperature sensor have been commented. This sensor
    won't be used in the commercial version of the device,
    it could be included in certain custom versions.*/
/*#include <DallasTemperature.h>//DS18B20*/

/* Access Point to configure ESP32 ( WiFi Web Server )
    used to input customer's WiFi credentials
    and give ESP32 access to internet*/
const char* esp32_ssid     = "GrowBox_config";
const char* esp32_password = "green4you";
WebServer server(80);
IPAddress ap_local_IP(192, 168, 1, 1);
IPAddress ap_gateway(192, 168, 1, 254);
IPAddress ap_subnet(255, 255, 255, 0);

TaskHandle_t wiFiHandler;//Task to handle wifi in core 0
QueueHandle_t writeQueue;
QueueHandle_t waterQueue;

/* User's WiFi credentials */
char ssidc[30];//Stores the router name, they must be less than 30 characters in length
char passwordc[30];//Stores the password

/* Flag to detect whether user's WiFi credentials
    are stored in FLASH memory or not and
    adresses where they will be stored */
#define WIFI_CRED_FLAG 0
#define NO_CREDENTIALS 0
#define WITH_CREDENTIALS 1
#define SSID_STORING_ADD 1
#define PASS_STORING_ADD 51
/* Flag activated when WiFi credentials are beign asked by ESP32 */
bool gettingWiFiCredentials = false;
/*FireBase authentification values*/
#define FIREBASE_HOST "growbox-350914-default-rtdb.firebaseio.com" //Do not include https:// in FIREBASE_HOST nor the last slash /
#define FIREBASE_AUTH "ZpsPdhWPAb2BMlsWiP9YmdujC8LINDnrVNfSxWAP"
/*Declare the Firebase Data object in the global scope*/
FirebaseData firebaseData2;
FirebaseData firebaseData1;
char users_uid[100] = "";

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
/*Light sensor*/
BH1750 lightMeter(0x23);
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
  int humiditySet = 100;
  int onHour = 0;
  int offHour = 0;
  int soilMoistureSet = 0;
  int temperatureSet = 100;
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
bool wrtWater = false;
readControl Rx;
writeControl Tx;
int currentHour = 0;

/*Switch debounce variables for WiFi reset*/
bool buttonState;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
int debounceDelay = 50;

float auxTemp=0;
float auxHumidity=0;

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

//Creating the input form
const char INDEX_HTML[] =
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta content='text/html' charset='UTF-8'"
  " http-equiv=\"content-type\">"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>Internet para su GrowBox</title>"
  "<style>"
  "body { background-color: #78288C; font-family: Arial, Helvetica, Sans-Serif; Color: white; }"
  "h1 { color: yellow; }"
  "h3 { color: yellow; }"
  ".form { margin-left: 100px; margin-top: 25px; }"
  "</style>"
  "</head>"
  "<body>"
  "<FORM action=\"/\" method=\"post\">"
  "<h1>Bienvenido a GrowBox</h1>"
  "<h3>El sistema de automatización de cultivos de interior.</h3>"
  "<ul>"
  "<li>Por favor, introduzca el nombre (SSID) y la contraseña de su red de WiFi luego presione aceptar</li>"
  "<li>Con esto su GrowBox ya estará conectado y podrá usarlo con la aplicación desde su celular.</li>"
  "<li>Si tiene problemas para iniciar su GrowBox, no dude en comunicarse con el centro de atencion al cliente.</li>"
  "<li>Puede contactarse por mail o por instagram</li>"
  "<li>growboxcultivo@gmail.com | IG: growbox1.0</li>"
  "</ul>"
  "<div>"
  "<label>ssid:&nbsp;</label>"
  "<input maxlength=\"30\" name=\"ssid\"><br>"
  "<label>contraseña:&nbsp;</label><input maxlength=\"30\" name=\"Password\"><br>"
  "<INPUT type=\"submit\" value=\"Aceptar\"> <INPUT type=\"Reset\">"
  "</div>"
  "<br>"
  "</FORM>"
  "</body>"
  "</html>";

String childPath[15] = {"/TempCtrlHigh","/HumCtrlHigh","/TemperatureControl","/HumidityControl", "/AutomaticWatering",
                        "/HumidityOffHour", "/HumidityOnHour", "/TemperatureOffHour", "/TemperatureOnHour",
                        "/HumiditySet", "/OffHour", "/OnHour", "/SoilMoistureSet",
                        "/TemperatureSet", "/Water"
                       };

/*****************************************************************************************/
// Once the data's been sent by the user, we display the inputs and write them to memory //
/*****************************************************************************************/
void handleSubmit();

/**************************************************************************************/
// Write data to memory                                                               //
// We prepping the data strings by adding the end of line symbol I decided to use ";".//
// Then we pass it off to the write_EEPROM function to actually write it to memmory   //
/**************************************************************************************/
void write_to_Memory(String s, String p);

 /***********************************
 * wifi core 0 tasks prototype
 * input pvParameters
 * out none
 ***********************************/                       
void wiFiTasks( void * pvParameters );

 /***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
void RGBalert(uint8_t error)
{
  static uint8_t green = 0, blue = 0, red = 0;
  switch(error)
  {    
    case NO_WIFI_CRED:
    green = 0;
    blue  = 0;
    red   = 255;
    Serial.println("NO_WIFI_CRED");
    break;
    case WIFI_CONN:
    green = 255;
    blue  = 0;
    red   = 0;
    Serial.println("WIFI_CONN");
    break;
    case WIFI_DISC:
    green = 0;
    blue  = 255;
    red   = 0;
    Serial.println("WIFI_DISC");
    break;
    case NO_USER:
    green = 50;
    blue  = 127;
    red   = 0;
    Serial.println("NO_USER");
    break;
    case DHT_ERR:
    green = 0;
    blue  = 0;
    red   = 0;
    Serial.println("DHT_ERR");
    break;
    case RTC_ERR:
    green = 0;
    blue  = 0;
    red   = 0;
    Serial.println("RTC_ERR");
    break;
    case SOIL_ERR:
    green = 0;
    blue  = 0;
    red   = 0;
    Serial.println("SOIL_ERR");
    break;
    case BH_1750_ERR:
    green = 0;
    blue  = 0;
    red   = 0;
    Serial.println("BH_1750_ERR");
    break;
    default:
    break;
  }  
  analogWrite(PIN_GREEN, green);
  analogWrite(PIN_BLUE,  blue);
  analogWrite(PIN_RED,   red);
  return;
}
/*******************************
 Update variables from FireBase
*******************************/
void streamCallback(MultiPathStreamData data)
{ 
  //Stream data can be many types which can be determined from function dataType
  for (size_t i = 0; i < 15; i++)
  {
    if (data.get(childPath[i]))
    {
      if (data.dataPath == "/TempCtrlHigh") {
#if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println(data.dataPath);
#endif
        if (data.value == "true")
          Rx.tempCtrlHigh = true;
        else
          Rx.tempCtrlHigh = false;
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.tempCtrlHigh);
#endif
      } else if (data.dataPath == "/HumCtrlHigh") {
#if SERIAL_DEBUG && FIRE_DEBUG      
        Serial.println(data.dataPath);
#endif
        if (data.value == "true")
          Rx.humCtrlHigh = true;
        else
          Rx.humCtrlHigh = false;
#if SERIAL_DEBUG && FIRE_DEBUG
       Serial.println(Rx.humCtrlHigh);
#endif
      } else if (data.dataPath == "/HumidityControl") {
#if SERIAL_DEBUG && FIRE_DEBUG      
        Serial.println(data.dataPath);
#endif
        if (data.value == "true")
          Rx.humidityControl = true;
        else
          Rx.humidityControl = false;
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.humidityControl);
#endif
      } else if (data.dataPath == "/TemperatureControl") {
#if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println(data.dataPath);
#endif        
        if (data.value == "true")
          Rx.temperatureControl = true;
        else
          Rx.temperatureControl = false;
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.temperatureControl);
#endif
      } else if (data.dataPath == "/AutomaticWatering") {
#if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println(data.dataPath);
#endif
        if (data.value == "true")
          Rx.automaticWatering = true;
        else
          Rx.automaticWatering = false;
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.automaticWatering);
#endif
      } else if (data.dataPath == "/HumidityOffHour") {
#if SERIAL_DEBUG && FIRE_DEBUG        
          Serial.println(data.dataPath);
#endif        
          Rx.humidityOffHour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
          Serial.println(Rx.humidityOffHour);
        #endif
      } else if (data.dataPath == "/HumidityOnHour") {
#if SERIAL_DEBUG && FIRE_DEBUG       
        Serial.println(data.dataPath);
#endif
          Rx.humidityOnHour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
          Serial.println(Rx.humidityOnHour);
        #endif
      } else if (data.dataPath == "/TemperatureOffHour") {
#if SERIAL_DEBUG && FIRE_DEBUG      
        Serial.println(data.dataPath);
#endif
        Rx.temperatureOffHour = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.temperatureOffHour);
#endif
      } else if (data.dataPath == "/TemperatureOnHour") {
#if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println(data.dataPath);
#endif
        Rx.temperatureOnHour = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.temperatureOnHour);
#endif
      } else if (data.dataPath == "/HumiditySet") {
#if SERIAL_DEBUG && FIRE_DEBUG     
        Serial.println(data.dataPath);
#endif        
        Rx.humiditySet = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.humiditySet);
#endif
      } else if (data.dataPath == "/OffHour") {
#if SERIAL_DEBUG && FIRE_DEBUG       
        Serial.println(data.dataPath);
#endif
        Rx.offHour = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.offHour);
#endif
      } else if (data.dataPath == "/OnHour") {
#if SERIAL_DEBUG && FIRE_DEBUG        
       Serial.println(data.dataPath);
#endif        
       Rx.onHour = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
       Serial.println(Rx.onHour);
 #endif
      } else if (data.dataPath == "/SoilMoistureSet") {
#if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println(data.dataPath);
#endif        
        Rx.soilMoistureSet = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println(Rx.soilMoistureSet);
#endif
      } else if (data.dataPath == "/TemperatureSet") {
#if SERIAL_DEBUG && FIRE_DEBUG    
        Serial.println(data.dataPath);
#endif        
        Rx.temperatureSet = data.value.toInt();
#if SERIAL_DEBUG && FIRE_DEBUG
      Serial.println(Rx.temperatureSet);
#endif
      } else if (data.dataPath == "/Water") {
#if SERIAL_DEBUG && FIRE_DEBUG      
        Serial.println(data.dataPath);
#endif        
        if (data.value == "true") {
          Rx.water = true;
        }
        else
        {
          Rx.water = false;
        }
      }
    }
  }
  #if SERIAL_DEBUG && FIRE_DEBUG        
        Serial.println("Stream callback end");
  #endif
  return;
}

//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout)
{
  if (timeout) {
    //Stream timeout occurred
#if SERIAL_DEBUG && FIRE_DEBUG
    Serial.println("Stream timeout, resume streaming...");
#endif  
  }
}

/*******************************
  Update variables in FireBase
*******************************/
void write2FireBase (struct writeControl *tx, FirebaseJson *dashBoard) {
  FirebaseJson json;
  (*dashBoard).add("ESPtag", String((*tx).ESPtag));
#if SERIAL_DEBUG && ESPtag_DEBUG
  Serial.println(String((*tx).ESPtag));
#endif
  (*dashBoard).add("HumidityControlOn", (*tx).humidityControlOn);
  (*dashBoard).add("TemperatureControlOn", (*tx).temperatureControlOn);
  (*dashBoard).add("Humidity", (*tx).humidity);
  (*dashBoard).add("Temperature", (*tx).temperature);
  (*dashBoard).add("Lights", (*tx).lights);
  (*dashBoard).add("Lux", (*tx).lux);
  (*dashBoard).add("Soil", (*tx).soil);
  (*dashBoard).add("SoilMoisture", (*tx).soilMoisture);
  (*dashBoard).add("Watering", (*tx).watering);
  //dashBoard->add(json);
}
/***********************************************************************/
// Reads both variables from the DHT and updates them in the Data Base,//
// also if temperature exceeds 27C turns on the ammbient fans          //
/***********************************************************************/
void TemperatureHumidityHandling ( struct readControl *rx, struct writeControl *tx, int currentHour ) 
{
  /********************************************************************/
  // This portion of code uses a generic DHT from RandomNerdTutorials //
  /********************************************************************/
  static uint8_t dhtFails = 0;
  if(!isnan(auxTemp) && !isnan(auxHumidity))
  {
    (*tx).humidity = auxHumidity;
    (*tx).temperature = auxTemp;
    dhtFails = 0;
  }
  /*else
  {
    dhtFails++;
    if(dhtFails == 10)
    {
      dht.begin();
    }
  }*/
  /*Humidity and temp, automatic control or periodic control
    The ventilators of the indoor can have a time period, or
    could be turned on and off depending on temperature and humidity*/
  //
#if SERIAL_DEBUG && DHT_DEBUG
  Serial.print("**************Temperature: ");
  Serial.println((*tx).temperature);
#endif 
  if ( (!isnan((*tx).temperature)) && (!isnan((*tx).humidity)) ) 
  { //TODO alarm sensor not working
    if ( (*rx).temperatureControl ) 
    {
      if((*rx).tempCtrlHigh)
      {
        if ( ((*tx).temperature > ((*rx).temperatureSet)) && !(*tx).temperatureControlOn ) 
        {
          digitalWrite(TEMP_CTRL_OUTPUT, LOW);//temperatureControlOn on
          (*tx).temperatureControlOn = true;
        }
        if ( ((*tx).temperature <= ((*rx).temperatureSet - 2)) && (*tx).temperatureControlOn ) 
        {
          digitalWrite(TEMP_CTRL_OUTPUT, HIGH);//temperatureControlOn off
          (*tx).temperatureControlOn = false;
        }
      }
      else
      {
        if ( ((*tx).temperature < ((*rx).temperatureSet)) && !(*tx).temperatureControlOn ) 
        {
          digitalWrite(TEMP_CTRL_OUTPUT, LOW);//temperatureControlOn on
          (*tx).temperatureControlOn = true;
        }
        if ( ((*tx).temperature >= ((*rx).temperatureSet + 2)) && (*tx).temperatureControlOn ) 
        {
          digitalWrite(TEMP_CTRL_OUTPUT, HIGH);//temperatureControlOn off
          (*tx).temperatureControlOn = false;
        }
      }
    }
    if( (*rx).humidityControl ) 
    {
      if((*rx).humCtrlHigh)
      {
        if ( ((*tx).humidity > ((*rx).humiditySet + 5)) && !(*tx).humidityControlOn ) 
        {
          digitalWrite(HUM_CTRL_OUTPUT, LOW);//humidityControl on
          (*tx).humidityControlOn = true;
        }
        if ( ((*tx).humidity < ((*rx).humiditySet - 2)) && (*tx).humidityControlOn ) 
        {
          digitalWrite(HUM_CTRL_OUTPUT, HIGH);//humidityControl off
          (*tx).humidityControlOn = false;
        }
      }
      else
      {
        if ( ((*tx).humidity < ((*rx).humiditySet)) && !(*tx).humidityControlOn ) 
        {
          digitalWrite(HUM_CTRL_OUTPUT, LOW);//humidityControl on
          (*tx).humidityControlOn = true;
        }
        if ( ((*tx).humidity > ((*rx).humiditySet + 2)) && (*tx).humidityControlOn ) 
        {
          digitalWrite(HUM_CTRL_OUTPUT, HIGH);//humidityControl off
          (*tx).humidityControlOn = false;
        }
      }
    }
  }
  
  if ( !(*rx).humidityControl ) 
  {
    /*Analize humidity's period*/
    if ( ( (*rx).humidityOffHour == (*rx).humidityOnHour || ( (*rx).humidityOffHour == 24 && (*rx).humidityOnHour == 0 ) || ( (*rx).humidityOffHour == 0 && (*rx).humidityOnHour == 24 ) ) ) 
    {
      digitalWrite(HUM_CTRL_OUTPUT, HIGH);//humidityControl off
      (*tx).humidityControlOn = false;
    }

    else if ( (*rx).humidityOffHour > (*rx).humidityOnHour )
    {
      if ( (currentHour >= (*rx).humidityOnHour) && (currentHour < (*rx).humidityOffHour) && !(*tx).humidityControlOn ) 
      {
        digitalWrite(HUM_CTRL_OUTPUT, LOW);//humidityControl on
        (*tx).humidityControlOn = true;
      }
      else if ( ( (currentHour >= (*rx).humidityOffHour) || (currentHour < (*rx).humidityOnHour) ) && (*tx).humidityControlOn ) 
      {
        digitalWrite(HUM_CTRL_OUTPUT, HIGH);//humidityControl on
        (*tx).humidityControlOn = false;
      }
    }

    else if ( (*rx).humidityOffHour < (*rx).humidityOnHour ) 
    {
      if ( ( (currentHour >= (*rx).humidityOnHour) || (currentHour < (*rx).humidityOffHour) ) && !(*tx).humidityControlOn ) 
      {
        digitalWrite(HUM_CTRL_OUTPUT, LOW);//humidityControl on
        (*tx).humidityControlOn = true;
      }
      else if ( (currentHour < (*rx).humidityOnHour) && (currentHour >= (*rx).humidityOffHour) && (*tx).humidityControlOn) 
      {
        digitalWrite(HUM_CTRL_OUTPUT, HIGH);//humidityControl on
        (*tx).humidityControlOn = false;
      }
    }
  }
  if ( !(*rx).temperatureControl ) 
  {
    /*Analize temperature's period*/
    if ( ( (*rx).temperatureOffHour == (*rx).temperatureOnHour || ( (*rx).temperatureOffHour == 24 && (*rx).temperatureOnHour == 0 ) || ( (*rx).temperatureOffHour == 0 && (*rx).temperatureOnHour == 24 ) ) ) 
    {
      digitalWrite(TEMP_CTRL_OUTPUT, HIGH);//temperatureControl off
      (*tx).temperatureControlOn = false;
    }

    else if ( (*rx).temperatureOffHour > (*rx).temperatureOnHour )
    {
      if ( (currentHour >= (*rx).temperatureOnHour) && (currentHour < (*rx).temperatureOffHour) && !(*tx).temperatureControlOn ) 
      {
        digitalWrite(TEMP_CTRL_OUTPUT, LOW);//temperatureControl on
        (*tx).temperatureControlOn = true;
      }
      else if ( ( (currentHour >= (*rx).temperatureOffHour) || (currentHour < (*rx).temperatureOnHour) ) && (*tx).temperatureControlOn ) 
      {
        digitalWrite(TEMP_CTRL_OUTPUT, HIGH);//temperatureControl on
        (*tx).temperatureControlOn = false;
      }
    }

    else if ( (*rx).temperatureOffHour < (*rx).temperatureOnHour ) 
    {
      if ( ( (currentHour >= (*rx).temperatureOnHour) || (currentHour < (*rx).temperatureOffHour) ) && !(*tx).temperatureControlOn ) 
      {
        digitalWrite(TEMP_CTRL_OUTPUT, LOW);//temperatureControl on
        (*tx).temperatureControlOn = true;
      }
      else if ( (currentHour < (*rx).temperatureOnHour) && (currentHour >= (*rx).temperatureOffHour) && (*tx).temperatureControlOn) 
      {
        digitalWrite(TEMP_CTRL_OUTPUT, HIGH);//temperatureControl on
        (*tx).temperatureControlOn = false;
      }
    }
  }
}

/***************************************************************/
// Reads light intensity and updates it in the Data Base, also //
/***************************************************************/
void ReadBH1750(struct writeControl *tx) {
  static uint8_t tries = 0;
  if (lightMeter.measurementReady())
  {
    float lux = lightMeter.readLightLevel();
    if(lux>=0){
      (*tx).lux = lux;
      tries=0;
    } else
      tries++;   
  }
  else
  {    
    tries++;
    if(tries>4)
    {
      lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE);
      tries=0;
    }
  }
#if SERIAL_DEBUG && BH_DEBUG
  Serial.print("**************Lux: ");
  Serial.println((*tx).lux);
#endif 
  vTaskDelay(500/portTICK_PERIOD_MS);
}
/********************************/
// Check for user's time chages //
/********************************/
void PhotoPeriod( struct readControl *rx, struct writeControl *tx , int currentHour ) {

  if ( ( (*rx).offHour == (*rx).onHour || ( (*rx).offHour == 24 && (*rx).onHour == 0 ) || ( (*rx).offHour == 0 && (*rx).onHour == 24 ) ) ) {
    digitalWrite(LIGHTS, HIGH);//lights off
    (*tx).lights = false;
    return;
  }

  else if ( (*rx).offHour > (*rx).onHour ) {
    if ( (currentHour >= (*rx).onHour) && (currentHour < (*rx).offHour) && !(*tx).lights ) {
      digitalWrite(LIGHTS, LOW);//lights on
      (*tx).lights = true;
    }
    else if ( ((currentHour >= (*rx).offHour) || (currentHour < (*rx).onHour)) && (*tx).lights ) {
      digitalWrite(LIGHTS, HIGH);//lights off
      (*tx).lights = false;
    }
    return;
  }

  else if ( (*rx).offHour < (*rx).onHour ) {
    if ( ((currentHour >= (*rx).onHour) || (currentHour < (*rx).offHour)) && !(*tx).lights ) {
      digitalWrite(LIGHTS, LOW);//lights on
      (*tx).lights = true;
    }
    else if ( (currentHour < (*rx).onHour) && (currentHour >= (*rx).offHour) && (*tx).lights) {
      digitalWrite(LIGHTS, HIGH);//lights off
      (*tx).lights = false;
    }
    return;
  }

}
/*******************/
/*Analog soil test */
/*******************/
void analogSoilRead(struct readControl *rx, struct writeControl *tx) {//bool

  bool stopWater1 = false;
  static unsigned long wateringDelay = 3*60000; //En Milisegundos
  static unsigned long manualWateringStart = 0;

  /*get soil moisture reading*/
  int reading = analogRead(SOILPIN);//the bigger the reading, the drier the ground
  for(int i = 0; i<99; i++){
    reading+=analogRead(SOILPIN);
  }
  reading/=100;
  (*tx).soilMoisture = -0.067 * reading + 216.230;
  (*tx).soilMoisture = constrain((*tx).soilMoisture, 0, 100);

#if SERIAL_DEBUG && SOIL_DEBUG
  Serial.print("**************Soil moisture: ");
  Serial.println((*tx).soilMoisture);
#endif 

  if ( (*rx).automaticWatering ) {
    /*Compare with set point*/
    if ( (*tx).soilMoisture < (*rx).soilMoistureSet - 5) {
      if (!(*tx).watering) {
        digitalWrite(W_VLV_PIN, LOW);//abre válvula
        (*tx).watering = true;
        (*tx).soil = false;
      }
    }
    if ( (*tx).soilMoisture > ((*rx).soilMoistureSet + 5) ) {
      if ((*tx).watering) {
        digitalWrite(W_VLV_PIN, HIGH);//cierra válvula
        (*tx).watering = false;
        (*tx).soil = true;
      }
    }
  }
  /*Manual operation*/
  else if ( (*rx).water ) {
    if ( !(*tx).watering ) {
      digitalWrite(W_VLV_PIN, LOW);//abre valvula
      (*tx).watering = true;
      manualWateringStart = millis();
#if SERIAL_DEBUG && WATERING_DEBUG
      Serial.println("Watering");
#endif
    }
    else {
      if ( (unsigned long)(millis() - manualWateringStart) > wateringDelay ) {//cambiar el limete de contador
        digitalWrite(W_VLV_PIN, HIGH);//cierra valvula
        (*tx).watering = false;
        (*rx).water = false;
#if SERIAL_DEBUG && WATERING_DEBUG
        Serial.println("Stoped watering");
#endif
        stopWater1 = true;
        xQueueSend(waterQueue, &stopWater1, 0);
      }
    }
  }
  else if((*tx).watering || !digitalRead(W_VLV_PIN))
  {
    digitalWrite(W_VLV_PIN, HIGH);//cierra valvula
    (*tx).watering = false;
    (*rx).water = false;
#if SERIAL_DEBUG && WATERING_DEBUG
    Serial.println("Stoped watering");
#endif
  }
}

/******************************************/
// Debounce routine for WiFi reset button //
/******************************************/
void debounceWiFiReset(void) {
  int reading = digitalRead(WIFI_RESET);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        Firebase.setString(firebaseData2, "users/" + mac + "/user/", "");
        delay(10);
        WiFi.disconnect();
        EEPROM.write(WIFI_CRED_FLAG, NO_CREDENTIALS);//wifi flag erased, credentials will be reset.
        EEPROM.commit();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ESP.restart();
      }
    }
  }
  lastButtonState = reading;
}
/****************************************************/
// Dealing with the call to root on the WiFi server //
/****************************************************/
void handleRoot() {
  if ( server.hasArg("ssid") && server.hasArg("Password") ) { //( server.hasArg("ssid") && server.hasArg("Password") && server.hasArg("mail") )
    handleSubmit();
  }
  else {//Redisplay the form
    server.send(200, "text/html", INDEX_HTML);
  }
}
/*****************************************************************************************/
// Once the data's been sent by the user, we display the inputs and write them to memory //
/*****************************************************************************************/
void handleSubmit() { //dispaly values and write to memmory
  String response =
    "<head>"
    "<meta content='text/html' charset='UTF-8'"
    " http-equiv=\"content-type\">"
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
    "<title>Internet para su GrowBox</title>"
    "<style>"
    "body { background-color: #78288C; font-family: Arial, Helvetica, Sans-Serif; Color: white; }"
    "</style>"
    "</head>"
    "<p>El nombre de la red WiFi es ";
  response += server.arg("ssid");
  response += "<br>";
  response += "Y su contraseña es ";
  response += server.arg("Password");
  response += "</P><BR>";
  response += "<H2><a href=\"/\">go home</a></H2><br>";

  server.send(200, "text/html", response);
  //calling function that writes data to memory
  write_to_Memory(String(server.arg("ssid")), String(server.arg("Password")));
  ESP.restart();
}

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

/********************************************************************/
// Shows when we get a misformt or wrong request for the web server //
/********************************************************************/
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  message += "<H2><a href=\"/\">go home</a></H2><br>";
  server.send(404, "text/plain", message);
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
/************************************/
// Starts WiFi server to get user's //
// WiFi credentials.                //
/************************************/
void setupServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(esp32_ssid, esp32_password, 1);
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
}
/******************************/
// Checks for WiFi credential //
/******************************/
bool checkWiFiCredentials() {
  if ( EEPROM.read( WIFI_CRED_FLAG ) ) {
    //reading the ssid and password out of memory
#if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println("WiFi credentials retrieved");
#endif
    String string_Ssid = "";
    String string_Password = "";
    string_Ssid = FLASH_read_string(30, SSID_STORING_ADD);
    string_Password = FLASH_read_string(30, PASS_STORING_ADD);
    string_Password.toCharArray(passwordc, 30);
    string_Ssid.toCharArray(ssidc, 30);
#if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println(ssidc);
    Serial.println(passwordc);
#endif
    return true;
  }
  else {
    #if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println("No wifi credentials");
#endif
    return false;
  }
}
/**************************************************************/
// In charge of connecting to WiFi LAN and FireBase Data Base //
/**************************************************************/
bool connectWifi() {
  // Let us connect to WiFi and FireBase
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssidc, passwordc);
  while (WiFi.status() != WL_CONNECTED)
  {
#if SERIAL_DEBUG && WIFI_DEBUG        
        Serial.println("connecting wifi");
#endif
    delay(300);
  }
  RGBalert(WIFI_CONN);  
  return true;
}

/*****************************************************************************/
// Set-up config for our ESP32: serial, WiFi, initiates OneWire comms, GPIOs //
/*****************************************************************************/
void setup() {

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
      Firebase.setBool(firebaseData2, "users/" + mac + "/GBconnected/", true);
      char aux_1[100];
      char mac_char[100];
      mac.toCharArray(mac_char, mac.length()+1);
      sprintf(aux_1,"users/%s/user/", mac_char);    
      if (!Firebase.getString(firebaseData2, aux_1, users_uid))
      {
        Firebase.setString(firebaseData2, "users/" + mac + "/user/", "");
      }
      while ( users_uid == "" )
      {
        RGBalert(NO_USER);
        Firebase.getString(firebaseData2, "users/" + mac + "/user/", users_uid);
        debounceWiFiReset();
      }
      path = "growboxs/" + String(users_uid);
      
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

void wiFiTasks( void * pvParameters ) {

  Firebase.beginMultiPathStream(firebaseData1, path + "/control", childPath, 12);

//  Firebase.setStreamTaskStackSize(20000);

  Firebase.setMultiPathStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);

  writeControl tx;

  for (;;) {

    /*Loop to obtain user's WiFi credentials via
      ESP32's WiFi server */
    while ( gettingWiFiCredentials )
    {
      server.handleClient();//Checks for web server activity
      if ( EEPROM.read( WIFI_CRED_FLAG ) == WITH_CREDENTIALS)
      {
        WiFi.softAPdisconnect (true);
        delay(5);
        checkWiFiCredentials();
        delay(5);
        gettingWiFiCredentials = false;
        connectWifi();
      }
    }

    if ( (!gettingWiFiCredentials) && (WiFi.status() == WL_CONNECTED) ) 
    {
      /*As long as we have WiFi and not getting WiFi cred through ESP32 server*/
      /*Perform all tasks requiring internet*/
      static bool stopWater2 = false;
      bool state1 = xQueueReceive(waterQueue, &stopWater2, 0);
      if (state1)
      {
      #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Stop water!!!!");
      #endif
        Firebase.setBool(firebaseData2, path + "/control/Water", false);
      }
      bool state2 = xQueueReceive(writeQueue, &tx, 0);
      if(state2)
      {
      #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Updating dashboard!!");
      #endif
        FirebaseJson dashBoard;
        write2FireBase( &tx, &dashBoard );
        Firebase.updateNode(firebaseData2, path + "/dashboard/", dashBoard);
      }
    }

    if (WiFi.status() != WL_CONNECTED && (!gettingWiFiCredentials))
    {
      connectWifi();
    }
    vTaskDelay(1);
  }
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