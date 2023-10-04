#ifndef GB_SENSORS_OUTPUTS_H
#define GB_SENSORS_OUTPUTS_H

#include <Arduino.h>
#include "globals.h"
#include <BH1750.h>//Include the library for light sensorsetQuality
#include "rgb_rst_butt.h"

#define MAX_TEMP_STEP 5

extern BH1750 lightMeter;
/***********************************************************************/
// Reads both variables ZXand updates them in the Data Base,//
// also if temperature exceeds 27C turns on the ammbient fans          //
/***********************************************************************/
void TemperatureHumidityHandling ( struct readControl *rx, struct writeControl *tx, int currentHour );
/***************************************************************/
// Reads light intensity and updates it in the Data Base, also //
/***************************************************************/
void ReadBH1750(struct writeControl *tx);
/********************************/
// Check for user's time chages //
/********************************/
void PhotoPeriod( struct readControl *rx, struct writeControl *tx , int currentHour );
/*******************/
/*Analog soil test */
/*******************/
void analogSoilRead(struct readControl *rx, struct writeControl *tx);
/**************************************************/
/*Reads temperature and humidity from SHT31 sensor*/
/**************************************************/
bool read_htu21(void);

#endif