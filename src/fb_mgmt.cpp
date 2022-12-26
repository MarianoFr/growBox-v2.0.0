#include "fb_mgmt.h"

/*Declare the Firebase Data object in the global scope*/
FirebaseData firebaseData2;
FirebaseData firebaseData1;
//Paths to firebase for multi stream
String childPath[15] = {"/TempCtrlHigh","/HumCtrlHigh","/TemperatureControl","/HumidityControl", "/AutomaticWatering",
                        "/HumidityOffHour", "/HumidityOnHour", "/TemperatureOffHour", "/TemperatureOnHour",
                        "/HumiditySet", "/OffHour", "/OnHour", "/SoilMoistureSet",
                        "/TemperatureSet", "/Water"
                       };

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