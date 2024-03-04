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

#define DELAY_TIME_BETWEEN_ITEMS_MS 10000 /*!< delay time between different test items */

#define ADC_READ_LEN         256
#define ADC_CONV_MODE        ADC_CONV_SINGLE_UNIT_1
#define ADC_OUTPUT_TYPE      ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ADC_ATTEN            ADC_ATTEN_DB_11
#define ADC_BIT_WIDTH        SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_UNIT             ADC_UNIT_1

#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)

#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)

void samplerTask ( void* pvParameters );

#endif