#include "sampler_task.h"

extern QueueHandle_t sensors2FB;
extern QueueHandle_t samplerRgbState;
extern QueueHandle_t sensors2outputs;
extern QueueHandle_t outputs2sampler;

/*Sensors definitions and declarations*/
Adafruit_HTU21DF htu = Adafruit_HTU21DF();//HTU21 sensor object

/*Light sensor*/
#define LUX_MEAN_WINDOW_SIZE   20
BH1750 lightMeter(0x23);
MeanFilter<float> luxMeanFilter(LUX_MEAN_WINDOW_SIZE);

TaskHandle_t samplingHandler;

static const char *TAG = "**SAMPLER_TASK**";

rgb_state_t local_sampler_rgb_state = 0;

SemaphoreHandle_t print_mux = NULL;

uint8_t nmbr_outputs = 0;
float auxTemp = 0;
float auxHumidity = 0;
float auxLux = 0;

/***************************************************************/
// Reads light intensity and updates it in the Data Base, also //
/***************************************************************/
bool read_BH1750( void ) {

    float reading = 0;
    static uint8_t tries = 0;
    if (lightMeter.measurementReady()) {
        reading = lightMeter.readLightLevel();
        if( reading < 0 ) {
            tries++;
            if(tries>4) {
                ESP_LOGI(TAG, "****RetryingLux****");
                lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE);              
                tries=0;                
            }
            return false;
        }            
    }
    else
        return false;
    auxLux = luxMeanFilter.AddValue(reading);
    return true;
}

bool read_htu21( void ) {

   auxTemp = htu.readTemperature();
   auxHumidity = htu.readHumidity();
   if(!isnan(auxTemp) && !isnan(auxHumidity)) { 
        return true;
    }
    else {
        uint8_t htu_tries = 0;
        while (!htu.begin() && htu_tries < 5) {
            ESP_LOGI(TAG, "Error initialising HTU21");
            htu_tries++;
        }
        if (htu_tries < 5) {
            ESP_LOGI(TAG, "HTU21 began");
        }
        htu_tries = 0;
        return false;
    }

}

static uint16_t get_mean_soil_moisture() {

    static uint16_t mean = 0;
    uint32_t measure = 0;
    float v0 = 3129, v05 = 2200, slope = 0, intercept = 0, alpha = 0.25, voltage_f;
    //Calculate Slope and Intercept as a function of 0% moisture voltage (v0) and 50% moisture voltage (v05)
    //Dependant with number of active relays, source voltage drops
    slope = -0.5*(((v0/1000)-0.02*nmbr_outputs)/(1-((v0/1000)-0.02*nmbr_outputs)/((v05/1000)-0.02*nmbr_outputs)));
    intercept = 0.5*(1/(1-((v0/1000)-0.02*nmbr_outputs)/((v05/1000)-0.02*nmbr_outputs)));
    ESP_LOGD(TAG, "Soil moisture slope=%.2f and intercept=%.2f", slope, intercept);
    measure = analogReadMilliVolts(SOILPIN);    
    voltage_f = (float)measure/1000;
    mean = (1-alpha)*mean + alpha*((1/voltage_f)*slope + intercept)*100;
    if(mean > 100) mean = 100;
    if(mean < 0) mean = 0;
    
    return mean;
}

void bh1750_init() {

    uint8_t bh1750_tries = 0;
    while (!lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE) && (bh1750_tries < 5)) {
        local_sampler_rgb_state |= 1UL << BH1750_ERR;
        ESP_LOGI(TAG, "Error initialising BH1750");
        vTaskDelay(10/portTICK_PERIOD_MS);
        bh1750_tries++;
    }
    if (bh1750_tries < 5) {
        ESP_LOGI(TAG, "BH1750 Advanced begin");
        local_sampler_rgb_state &= ~(1UL << BH1750_ERR);
    }
    bh1750_tries = 0;

}

void htu21_init() {
    
    uint8_t htu_tries = 0;
    while (!htu.begin() && htu_tries < 5) {
        ESP_LOGI(TAG, "Error initialising HTU21");
        vTaskDelay(10/portTICK_PERIOD_MS);
        htu_tries++;
        local_sampler_rgb_state |= 1UL << HTU21_ERR;
    }
    if (htu_tries < 5) {
        ESP_LOGI(TAG,"HTU21 began");
        local_sampler_rgb_state &= ~(1UL << HTU21_ERR);
    }
    htu_tries = 0;

}

void samplerTask ( void* pvParameters ) {
    
    tx_sensor_data_t sensor_data;
    time_t now = 0;
    struct tm timeinfo = { 0 };      

    ESP_LOGD(TAG, "samplerTask initialized");

    htu21_init();
    bh1750_init();     


    /* Block to wait for wifi_task to synch sntp time to notify this task. */
    xTaskNotifyWait( 0x01, 0x01, NULL, portMAX_DELAY );
    ESP_LOGD(TAG, "SamplerTask notified by wifi utils");

    for ( ; ; ) {
        
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(sensor_data.esp_tag, 20, "%Y-%m-%d %X", &timeinfo);
        ESP_LOGD(TAG, "ESP tag: %s", sensor_data.esp_tag);

        if(read_BH1750()) {
            sensor_data.lux = auxLux;
            local_sampler_rgb_state &= ~(1UL << BH1750_ERR);
        } else
            local_sampler_rgb_state |= (1UL << BH1750_ERR);

        if(read_htu21()) {
            sensor_data.temperature = auxTemp;
            sensor_data.humidity = auxHumidity;
            local_sampler_rgb_state &= ~(1UL << HTU21_ERR);
        } else            
            local_sampler_rgb_state |= 1UL << HTU21_ERR;

        sensor_data.soil_humidity = get_mean_soil_moisture();
        ESP_LOGI(TAG, "Temperature: %.02f°C\nHumidity: %.02f%%\nLux: %.02fSoil moisture: %d%%\n", 
            sensor_data.temperature, sensor_data.humidity, sensor_data.lux, sensor_data.soil_humidity);

        xQueueSend(sensors2FB, &sensor_data, 1/portTICK_PERIOD_MS);
        xQueueSend(sensors2outputs, &sensor_data, 1/portTICK_PERIOD_MS);
        xQueueSend(samplerRgbState, &local_sampler_rgb_state, 1/portTICK_PERIOD_MS);
        xQueueReceive(outputs2sampler, &nmbr_outputs, 1/portTICK_PERIOD_MS);
        vTaskDelay((DELAY_TIME_BETWEEN_ITEMS_MS) / portTICK_PERIOD_MS);
        
    }
    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}