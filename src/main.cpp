/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_utils.h"
#include "wifi_task.h"
#include "outputs_task.h"
#include "sampler_task.h"
#include "nvs_storage.h"
#include "data_types.h"

static const char *TAG = "**MAIN**";

extern TaskHandle_t wiFiHandler;
extern TaskHandle_t outputsHandler;
extern TaskHandle_t samplingHandler;

QueueHandle_t sensors2FB;
QueueHandle_t sensors2outputs;
QueueHandle_t FB2outputs;
QueueHandle_t outputs2FB;
QueueHandle_t wifiRgbState;
QueueHandle_t samplerRgbState;
QueueHandle_t outputs2sampler;

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

rgb_state_t local_wifi_rgb_state = (1UL << WIFI_DISC);
bool fb_status = false;
bool no_credentials = false;

extern char wifi_ssid[];
extern char wifi_pass[];

class Button
{
  private:
    uint8_t btn;
    uint16_t state;
    uint32_t started;
  public:
    Button(uint8_t button) {
      btn = button;
      state = 0;    
      gpio_config_t io_conf = {};
      io_conf.intr_type = GPIO_INTR_DISABLE;
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pin_bit_mask = 1ULL<<btn;
      //disable pull-down mode
      io_conf.pull_down_en = (gpio_pulldown_t)0;
      //disable pull-up mode
      io_conf.pull_up_en = (gpio_pullup_t)0;
      //configure GPIO with the given settings
      gpio_config(&io_conf);
    }

    bool debounce() 
    {
      state = (state<<1) | gpio_get_level((gpio_num_t)btn) | 0xfe00;
    
      if(state == 0xff00)
      {
        ESP_LOGI(TAG, "Button pressed");       
      }   
      return (state == 0xff00);
    }
};

Button wifi_res_butt( WIFI_RESET_PIN );

bool queues_init( void ) {
    
	sensors2FB = xQueueCreate(1, sizeof(tx_sensor_data_t));
	if( sensors2FB == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "sensors2FB queue created");
	}

    sensors2outputs = xQueueCreate(1, sizeof(tx_sensor_data_t));
	if( sensors2outputs == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "sensors2outputs queue created");
	}

    outputs2sampler = xQueueCreate(1, sizeof(uint8_t));
	if( outputs2sampler == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "outputs2sampler queue created");
	}

    FB2outputs = xQueueCreate(1, sizeof(rx_control_update_t));
	if( FB2outputs == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "FB2outputs queue created");
	}

	outputs2FB = xQueueCreate(1, sizeof(tx_control_data_t));
	if( outputs2FB == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "outputs2FB queue created");
	}

    wifiRgbState = xQueueCreate(3, sizeof(rgb_state_t));
	if( outputs2FB == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "wifiRgbState queue created");
	}

    samplerRgbState = xQueueCreate(3, sizeof(rgb_state_t));
	if( outputs2FB == NULL ) { 
		return false;
	} else {
		ESP_LOGI(TAG, "samplerRgbState queue created");
	}

    return true;
}

bool create_tasks() {
    if(nvs_read_wifi_creds(wifi_ssid, wifi_pass)) {
        ESP_LOGI(TAG, "Retrieved WiFi credentials");
        //TODO: after performance-check, decide if task should run on different cores
        //Should wifi run on a different core than the rest of the tasks?
        xTaskCreatePinnedToCore(
            wifiTask,                               /* Function to implement the task */
            "wifiTask",                             /* Name of the task */
            configWIFI_STACK_SIZE/sizeof(size_t),   /* Stack size in words */
            NULL,                                   /* Task input parameter */
            WIFI_TASK_PRIORITY,                     /* Priority of the task */
            &wiFiHandler,                           /* Task handle. */
            1                                       /* Core where the task should run */
        );
        xTaskCreatePinnedToCore(
            outputsTask,                            /* Function to implement the task */
            "outputsTask",                          /* Name of the task */
            configOUTPUTS_STACK_SIZE/sizeof(size_t),/* Stack size in words */
            NULL,                                   /* Task input parameter */
            OUTPUTS_TASK_PRIORITY,                  /* Priority of the task */
            &outputsHandler,                        /* Task handle. */
            1                                       /* Core where the task should run */
        );
        xTaskCreatePinnedToCore(
            samplerTask,                            /* Function to implement the task */
            "samplerTask",                          /* Name of the task */
            configSAMPLER_STACK_SIZE/sizeof(size_t),/* Stack size in words */
            NULL,                                   /* Task input parameter */
            SAMPLER_TASK_PRIORITY,                  /* Priority of the task */
            &samplingHandler,                       /* Task handle. */
            1                                       /* Core where the task should run */
        );
        return true;
    }
    else {
        ESP_LOGI(TAG, "Creating serverTask");
        xTaskCreatePinnedToCore(
            serverTask,    /* Function to implement the task */
            "serverTask",  /* Name of the task */
            configWIFI_STACK_SIZE/sizeof(size_t),        /* Stack size in words */
            NULL,         /* Task input parameter */
            WIFI_TASK_PRIORITY,            /* Priority of the task */
            &wiFiHandler, /* Task handle. */
            1              /* Core where the task should run */
        );
        return false;
    }
}

void setup() {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if DEBUG_ERASE
    ESP_LOGI(TAG, "Debug errasing nvs storage");
    nvs_erase_storage();
    while(1) {
        vTaskDelay(1);
    }
#endif

    // Set timezone to China Standard Time
    setenv("TZ", GB_TIME_ZONE, 1);
    tzset();

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    if(queues_init()) {
        ESP_LOGI(TAG, "Queues created");
    } else {
        ESP_LOGI(TAG, "Failed to create queues");
    }

    no_credentials = create_tasks();  
     
}

void loop() {

    if(no_credentials) {

        if( wifi_res_butt.debounce( ) ) {
            nvs_erase_wifi_creds( );
            ESP_LOGI(TAG, "WiFi errased, reset ESP32");
            vTaskDelay(10);
            esp_restart( );
        }
        
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
