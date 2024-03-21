#ifndef SAMPLER_TASK_H
#define SAMPLER_TASK_H
//Standard headers
#include <ctime>
#include <stdio.h>
#include <string.h>
//Libraries
#include "Adafruit_HTU21DF.h"
#include <BH1750.h>//Include the library for light sensorsetQuality
#include "MeanFilterLib.h"
//my headers
#include "app_config.h"
#include "data_types.h"
#include "board.h"

#define DELAY_TIME_BETWEEN_ITEMS_MS 60000 /*!< delay time between different test items */

#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)

#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)

void samplerTask ( void* pvParameters );

#endif