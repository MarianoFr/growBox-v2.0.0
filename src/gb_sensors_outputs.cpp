#include "gb_sensors_outputs.h"

/*Light sensor*/
BH1750 lightMeter(0x23);

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
  }
  /* else
  {
    rgb_state |= DHT_ERR;
    dhtFails++;
    if(dhtFails >= DHT_MAX_ERR)
    {
      dhtFails = 0;
      dht.begin();
    }
  } */
  /*Humidity and temp, automatic control or periodic control
    The ventilators of the indoor can have a time period, or
    could be turned on and off depending on temperature and humidity*/
#if SERIAL_DEBUG && DHT_DEBUG
  Serial.print("**************Temperature: ");
  Serial.println((*tx).temperature);
  Serial.print("**************Humidity: ");
  Serial.println((*tx).humidity);
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
void analogSoilRead(struct readControl *rx, struct writeControl *tx)
{
  bool stopWater1 = false;
  static unsigned long wateringDelay = 3*60000; //En Milisegundos
  static unsigned long manualWateringStart = 0;
  /*get soil moisture reading*/
  uint32_t voltage_mv = 0;//the bigger the reading, the drier the ground
  float voltage_v = 0;
  float slope = 2.48, intercept = -0.72;
  for(int i = 0; i<10; i++){
    voltage_mv+=analogReadMilliVolts(SOILPIN);
  }
  voltage_mv/=10;
  float aux = voltage_mv;
  voltage_v = aux/1000;
  //(*tx).soilMoisture = -0.067 * reading + 216.230;
  (*tx).soilMoisture = ((1/voltage_v)*slope + intercept)*100;
#if SERIAL_DEBUG && SOIL_DEBUG
  Serial.print("Voltage in mV:" );
  Serial.println(voltage_mv);
  Serial.print("Voltage in mV:" );
  Serial.println(aux);
  Serial.print("Voltage in V:" );
  Serial.println(voltage_v);
  Serial.print("**************Soil moisture: "); 
  Serial.println((*tx).soilMoisture);
  vTaskDelay(30000);
#endif
  (*tx).soilMoisture = constrain((*tx).soilMoisture, 0, 100);

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