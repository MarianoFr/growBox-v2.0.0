#include "wifi_task.h"

TaskHandle_t wiFiHandler;
extern TaskHandle_t outputsHandler;
extern TaskHandle_t samplingHandler;

static const char *TAG = "**WIFI_TASK**";

extern EventGroupHandle_t s_wifi_event_group;

extern char wifi_ssid[];
extern char wifi_pass[];

extern QueueHandle_t sensors2FB;
extern QueueHandle_t FB2outputs;
extern QueueHandle_t outputs2FB;
extern QueueHandle_t wifiRgbState;

rgb_state_t local_wifi_rgb_state = (1UL << WIFI_DISC);

tx_sensor_data_t sensor_data_1 = {
    .temperature    = 0,
    .humidity      = 0,
    .soil_humidity = 0,
    .lux            = 0,
};

tx_control_data_t control_data = {
    .lights_on = false,
	.temperature_on = false,
    .humidity_on = false,
    .water_on = false,
};

void wifiTask(void* pvParameters) {

    uint8_t fb_retry = 0;
    uint8_t res = 0;
    bool fb_status = false;

    ESP_LOGI(TAG, "wifiTask initialized");

    local_wifi_rgb_state &= ~(1UL << NO_WIFI_CRED);

    res = WiFi.begin("casita", "TINTAYBELLA");
    ESP_LOGI(TAG, "Began WiFi with ssid: %s and pass: %s", wifi_ssid, wifi_pass);
    while(WiFi.status() != WL_CONNECTED) {
        vTaskDelay(10/portTICK_RATE_MS);
        
    }
    ESP_LOGI(TAG, "WiFi connected");
    vTaskDelay(2000/portTICK_RATE_MS);
    xTaskNotifyGive( outputsHandler );
    xTaskNotifyGive( samplingHandler );

    local_wifi_rgb_state |= (1UL << WIFI_CONN);
    local_wifi_rgb_state &= ~(1UL << WIFI_DISC);

    fb_status = gb_firebase_init();
    while(!fb_status && fb_retry < 3) {
        fb_retry++;
        fb_status = gb_firebase_init();
    }
    fb_retry = 0;
    if(fb_status) {
        ESP_LOGI(TAG, "FireBase initialized");
        local_wifi_rgb_state &= ~(1UL << NO_USER);
    }
    else {
        ESP_LOGI(TAG, "FireBase init failed");    
        local_wifi_rgb_state |= (1UL << NO_USER);
    }    
    
    for ( ; ; ) {
        
        static UBaseType_t task_stack_free_saved = -1;
		UBaseType_t task_stack_free = uxTaskGetStackHighWaterMark( NULL );
		if( task_stack_free_saved != task_stack_free ) { 
			task_stack_free_saved = task_stack_free;
			ESP_LOGI(TAG, "Task has %u bytes availables", task_stack_free_saved * sizeof(size_t) );
		}        
        if(WiFi.status() == WL_CONNECTED) {
            local_wifi_rgb_state |= (1UL << WIFI_CONN);
            local_wifi_rgb_state &= ~(1UL << WIFI_DISC);
            if(fb_status) {
                if(xQueueReceive(sensors2FB, &sensor_data_1, 10/portTICK_PERIOD_MS) == pdPASS) {
                    fb_status = fb_update_sensor(&sensor_data_1);
                }
                if(xQueueReceive(outputs2FB, &control_data, 10/portTICK_PERIOD_MS) == pdPASS) {
                    fb_status = fb_update_control(&control_data);
                    if(fb_status)//Notify control update, water control involved
                        xTaskNotify(outputsHandler,1,eNoAction);
                    else
                        xTaskNotify(outputsHandler,2,eNoAction);//Failed to update
                }
            }
        }
        else {
            //TODO: is it is necessary to reconnect wifi? or is it is already solved automatically by api
            local_wifi_rgb_state |= (1UL << WIFI_DISC);
            local_wifi_rgb_state &= ~(1UL << WIFI_CONN);
        }
        xQueueSend(wifiRgbState, &local_wifi_rgb_state, 10/portTICK_PERIOD_MS);
        vTaskDelay(1);
    }

}

void serverTask(void* pvParameters) {

    ESP_LOGI(TAG, "serverTask initialized");
    local_wifi_rgb_state |= (1UL << NO_WIFI_CRED);
    nvs_erase_wifi_creds( );
    wifi_init_softap( );
    for ( ; ; ) {
        vTaskDelay(1);
    }

}