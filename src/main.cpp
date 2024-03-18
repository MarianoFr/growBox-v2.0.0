/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <Arduino.h>

#include <iostream>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_utils.h"
#include "outputs_task.h"
#include "sampler_task.h"
#include "nvs_storage.h"
#include "data_types.h"

#include <ESP32Time.h>
#include "FirebaseESP32.h"
#include "WiFi.h"

/*FireBase authentification values*/
#define FIREBASE_HOST "growbox-350914-default-rtdb.firebaseio.com" //Do not include https:// in FIREBASE_HOST nor the last slash /
#define FIREBASE_AUTH "ZpsPdhWPAb2BMlsWiP9YmdujC8LINDnrVNfSxWAP"
#define UID_FETCH_TO  15*1000

/*Declare the Firebase Data object in the global scope*/
FirebaseData firebaseData2;
FirebaseData firebaseData1;

//Paths to firebase for multi stream
String childPath[15] = {"/TempCtrlHigh","/HumCtrlHigh","/TemperatureControl","/HumidityControl", "/AutomaticWatering",
                        "/HumidityOffHour", "/HumidityOnHour", "/TemperatureOffHour", "/TemperatureOnHour",
                        "/HumiditySet", "/OffHour", "/OnHour", "/SoilMoistureSet",
                        "/TemperatureSet", "/Water"
                       };

static const char *TAG = "**MAIN**";

extern TaskHandle_t wiFiHandler;
extern TaskHandle_t outputsHandler;
extern TaskHandle_t samplingHandler;

//Reset button timer
static TimerHandle_t xRstButTimer;

ESP32Time rtc;

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
uint8_t fb_retry = 0;
uint8_t res = 0;

extern char wifi_ssid[];
extern char wifi_pass[];

char gb_connected_path[50];
char gb_mac_user_path[50];
char path_to_dashboard[100];
char user_uid[50];
char gb_control_path[50];
char path_to_gb[50];

/*******************************
 Update variables from FireBase
*******************************/
void streamCallback(MultiPathStreamData data) {
    rx_control_update_t control_data_dummy;
    rx_control_update_t control_update_data;
    control_update_data.var_2_update = 0;
    for (size_t i = 0; i < 15; i++) {
        if (data.get(childPath[i])) {
            if (data.dataPath == "/TempCtrlHigh") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());            
                if (data.value == "true")
                    control_update_data.control_data.temperature_control_high = true;
                else
                    control_update_data.control_data.temperature_control_high = false;
                control_update_data.var_2_update |= 1UL << UPDT_TEMP_CTRL_H;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.temperature_control_high);        
            } else if (data.dataPath == "/HumCtrlHigh") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());         
                if (data.value == "true")
                    control_update_data.control_data.humidity_control_high = true;
                else
                    control_update_data.control_data.humidity_control_high = false;
                control_update_data.var_2_update |= 1UL << UPDT_HUM_CTRL_H;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.humidity_control_high);        
            } else if (data.dataPath == "/HumidityControl") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                 
                if (data.value == "true")
                    control_update_data.control_data.humidity_control = true;
                else
                    control_update_data.control_data.humidity_control = false;
                control_update_data.var_2_update |= 1UL << UPDT_HUM_CTRL_ON;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.humidity_control);        
            } else if (data.dataPath == "/TemperatureControl") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                    
                if (data.value == "true")
                    control_update_data.control_data.temperature_control = true;
                else
                    control_update_data.control_data.temperature_control = false;
                control_update_data.var_2_update |= 1UL << UPDT_TEMP_CTRL_ON;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.temperature_control);            
            } else if (data.dataPath == "/AutomaticWatering") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());        
                if (data.value == "true")
                    control_update_data.control_data.automatic_watering = true;
                else
                    control_update_data.control_data.automatic_watering = false;
                control_update_data.var_2_update |= 1UL << UPDT_AUTO_WATER;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.automatic_watering);        
            } else if (data.dataPath == "/HumidityOffHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                
                control_update_data.control_data.humidity_off_hour = data.value.toInt();
                control_update_data.var_2_update |= 1UL << UPDT_HUM_OFF_HR;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.humidity_off_hour);            
            } else if (data.dataPath == "/HumidityOnHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());        
                control_update_data.control_data.humidity_on_hour = data.value.toInt();
                control_update_data.var_2_update |= 1UL << UPDT_HUM_ON_HR;
                ESP_LOGI(TAG, "%d", control_update_data.control_data.humidity_on_hour);            
            } else if (data.dataPath == "/TemperatureOffHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());      
                control_update_data.control_data.temperature_off_hour = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.temperature_off_hour);  
                control_update_data.var_2_update |= 1UL << UPDT_TEMP_OFF_HR;
            } else if (data.dataPath == "/TemperatureOnHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());        
                control_update_data.control_data.temperature_on_hour = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.temperature_on_hour);
                control_update_data.var_2_update |= 1UL << UPDT_TEMP_ON_HR;
            } else if (data.dataPath == "/HumiditySet") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());              
                control_update_data.control_data.humidity_set = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.humidity_set);
                control_update_data.var_2_update |= 1UL << UPDT_HUM_SET;
            } else if (data.dataPath == "/OffHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());        
                control_update_data.control_data.lights_off_hour = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.lights_off_hour);
                control_update_data.var_2_update |= 1UL << UPDT_LIGHTS_OFF_HR;
            } else if (data.dataPath == "/OnHour") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());               
                control_update_data.control_data.lights_on_hour = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.lights_on_hour);
                control_update_data.var_2_update |= 1UL << UPDT_LIGHTS_ON_HR;
            } else if (data.dataPath == "/SoilMoistureSet") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                
                control_update_data.control_data.soil_moisture_set = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.soil_moisture_set);
                control_update_data.var_2_update |= 1UL << UPDT_SOIL_SET;
            } else if (data.dataPath == "/TemperatureSet") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                 
                control_update_data.control_data.temperature_set = data.value.toInt();
                ESP_LOGI(TAG, "%d", control_update_data.control_data.temperature_set);
                control_update_data.var_2_update |= 1UL << UPDT_TEMP_SET;      
            } else if (data.dataPath == "/Water") {
                ESP_LOGI(TAG, "%s",data.dataPath.c_str());                 
                if (data.value == "true")
                    control_update_data.control_data.water = true;
                else
                    control_update_data.control_data.water = false;
                control_update_data.var_2_update |= 1UL << UPDT_WATER_ON;
            }
        }
    }
    if(xQueueSend(FB2outputs, &control_update_data, 10/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGI("**FBCB**", "Failed to update");
        xQueueReceive(FB2outputs, &control_data_dummy, 10/portTICK_PERIOD_MS);
        xQueueSend(FB2outputs, &control_update_data, 10/portTICK_PERIOD_MS);
    }
  return;

}

//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    //Stream timeout occurred
    ESP_LOGI(TAG, "Stream timeout, resume streaming...");  
  }
}

/*******************************
  Update variables in FireBase
*******************************/
void write2FBSensor (tx_sensor_data_t *sensor, FirebaseJson *dashBoard) {
  //FirebaseJson json;
  (*dashBoard).add("ESPtag", String((*sensor).esp_tag));
  (*dashBoard).add("Humidity", (*sensor).humidity);
  (*dashBoard).add("Temperature", (*sensor).temperature);
  (*dashBoard).add("Lux", (*sensor).lux);
  (*dashBoard).add("SoilMoisture", (*sensor).soil_humidity);
  //dashBoard->add(json);
}

/*******************************
  Update variables in FireBase
*******************************/
void write2FBControl (tx_control_data_t *control, FirebaseJson *dashBoard) {

    (*dashBoard).add("HumidityControlOn", (*control).humidity_on);
    (*dashBoard).add("TemperatureControlOn", (*control).temperature_on);
    (*dashBoard).add("Lights", (*control).lights_on);
    (*dashBoard).add("Watering", (*control).water_on);

}

bool fb_update_sensor(tx_sensor_data_t *sensor) {
    
    bool res = false;
    FirebaseJson dashBoard;
    write2FBSensor( sensor, &dashBoard );
    ESP_LOGI(TAG, "%s", dashBoard.raw());
    if(Firebase.updateNode(firebaseData2, path_to_dashboard, dashBoard)) {    
        res = true;
    }
    else {        
        res = false;
    }
    return res;

}

bool fb_update_water() {

    char path_to_water[70];
    bool res = false;
    sprintf(path_to_water, "/growboxs/%s/control/Water", user_uid);
    if(Firebase.setBool(firebaseData2, path_to_water, false))
        res = true;

    return res;    
}

bool fb_update_control(tx_control_data_t *control_data) {
    
    bool res = false;
    FirebaseJson dashBoard;
    char path_to_water[100];

    sprintf(path_to_water, "/growboxs/%s/control/Water/", user_uid); 

    write2FBControl( control_data, &dashBoard );
        
    if(Firebase.updateNode(firebaseData2, path_to_dashboard, dashBoard)) {
        res = true;
    }
    else {
        res = false;
    }
    if(!(*control_data).water_on)
        Firebase.setBool(firebaseData2, path_to_water, false);

    return res;

}

bool gb_firebase_init( void ) {

    unsigned char mac_base[6] = {0};        
    unsigned long start_uid_fetch = 0;
    tm now;
    struct tm timeinfo = { 0 };
    char esp_frst_conn_path[100];
    int nmbr_rst = 0;    
    char nmbr_rst_char[50];
    char esp_frst_conn[50];
    char nmbr_rst_path[100];
    
    esp_efuse_mac_get_default(mac_base);
    
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
    Firebase.setReadTimeout(firebaseData2, 1000 * 60);
    /*Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).*/
    Firebase.setwriteSizeLimit(firebaseData2, "medium");

    sprintf(gb_connected_path, "users/%02X:%02X:%02X:%02X:%02X:%02X/GBconnected/",mac_base[0]
            ,mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    ESP_LOGI("**FB**", "Path to user connected %s", gb_connected_path);
    Firebase.setBool(firebaseData2, gb_connected_path, true);
    sprintf(gb_mac_user_path, "users/%02X:%02X:%02X:%02X:%02X:%02X/user/",mac_base[0]
            ,mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    
    start_uid_fetch = millis();
    if (!Firebase.getString(firebaseData2, gb_mac_user_path, user_uid)) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        if (!Firebase.getString(firebaseData2, gb_mac_user_path, user_uid)) {
            vTaskDelay(1000/portTICK_PERIOD_MS);
            if (!Firebase.getString(firebaseData2, gb_mac_user_path, user_uid)) {
                Firebase.setString(firebaseData2, gb_mac_user_path, "NULL");
                //rgb_state |= 1UL << NO_USER;//TODO: specify these by returning false?
                sprintf(user_uid, "%s", "NULL");
            }
        }
    }
    while (strcmp(user_uid, "NULL") == 0 || (millis() - start_uid_fetch < UID_FETCH_TO)) {
        Firebase.getString(firebaseData2, gb_mac_user_path, user_uid);
        //debounceWiFiReset();//TODO: constantly run debounce on task? timer on main task?
        vTaskDelay(pdMS_TO_TICKS(1000/portTICK_PERIOD_MS));
    }

    if(strcmp(user_uid, "NULL") == 0) {
        ESP_LOGI("**FB**", "User is NULL");
        return false;
    }
       

    //Get number of resets and increase
    sprintf(nmbr_rst_path, "/growboxs/%s/dashboard/NmbrResets",user_uid);
    Firebase.getInt(firebaseData2, nmbr_rst_path, &nmbr_rst);
    nmbr_rst++;
    Firebase.setInt(firebaseData2, nmbr_rst_path, nmbr_rst);

    //Set first connection
    now = rtc.getTimeStruct();
    strftime(esp_frst_conn, 20, "%Y-%m-%d %X", &now);
    sprintf(esp_frst_conn_path, "/growboxs/%s/dashboard/FirstConnection", user_uid);
    Firebase.setString(firebaseData2, esp_frst_conn_path, esp_frst_conn);

    sprintf(path_to_gb, "/growboxs/%s", user_uid);
    sprintf(path_to_dashboard, "/growboxs/%s/dashboard", user_uid);
    sprintf(gb_control_path, "/growboxs/%s/control", user_uid);    
    ESP_LOGI("**FB**", "Path to control %s", gb_control_path);
    if(!Firebase.beginMultiPathStream(firebaseData1, gb_control_path, childPath, sizeof(childPath))) {
        ESP_LOGI("**FB**", "Couldnt begin stream");
        return false;
    }
//  Firebase.setStreamTaskStackSize(20000);
    Firebase.setMultiPathStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);

    return true;

}

class Button {

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

    bool debounce() {
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
        return false;
    }
    else
        return true;
}

bool synch_time() {

    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = -10800;
    const int daylightOffset_sec = 0;
    struct tm currentTime;
    uint8_t ntp_retry = 0;
    /*Init the NTP library*/
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    while (!getLocalTime(&currentTime) && (ntp_retry < NTP_RETRY)) {
        ntp_retry++;
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    if(ntp_retry >= NTP_RETRY)
        return false;
    rtc.setTime(currentTime.tm_sec, currentTime.tm_min, currentTime.tm_hour, currentTime.tm_mday, currentTime.tm_mon + 1, currentTime.tm_year + 1900);
    return true;

}

static void RstButTimerCB(TimerHandle_t xTimer) {

    if(!no_credentials) {

        if( wifi_res_butt.debounce( ) ) {
            if(fb_status) {
                Firebase.setBool(firebaseData2, gb_connected_path, false);
                Firebase.deleteNode(firebaseData2, path_to_gb);
                Firebase.setString(firebaseData2, gb_mac_user_path,"NULL");
            }            
            nvs_erase_wifi_creds( );
            ESP_LOGI(TAG, "WiFi errased, reset ESP32");
            vTaskDelay(10);
            esp_restart( );
        }

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

    if(queues_init()) {
        ESP_LOGI(TAG, "Queues created");
    } else {
        ESP_LOGI(TAG, "Failed to create queues");
    }

    //create tasks and check for wifi credentials
    no_credentials = create_tasks();

    xRstButTimer = xTimerCreate("RGB timer", 50 / portTICK_PERIOD_MS, pdTRUE, (void *)0, RstButTimerCB);
    configASSERT(xRstButTimer);
    configASSERT(xTimerStart(xRstButTimer, 10 / portTICK_PERIOD_MS));    
    
    if(!no_credentials) {
        local_wifi_rgb_state &= ~(1UL << NO_WIFI_CRED);
        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        res = WiFi.begin(wifi_ssid, wifi_pass);
        ESP_LOGI(TAG, "Began WiFi with ssid: %s and pass: %s", wifi_ssid, wifi_pass);
        while(WiFi.status() != WL_CONNECTED) {
            vTaskDelay(10/portTICK_RATE_MS);            
        }
        ESP_LOGI(TAG, "WiFi connected");
        vTaskDelay(10/portTICK_RATE_MS);        
        while(!synch_time());
        local_wifi_rgb_state |= (1UL << WIFI_CONN);
        local_wifi_rgb_state &= ~(1UL << WIFI_DISC);
        
        fb_status = gb_firebase_init();
        while(!fb_status && fb_retry < 3) {
            fb_retry++;
            fb_status = gb_firebase_init();
        }
        fb_retry = 0;
        if(fb_status) {
            xTaskNotifyGive( outputsHandler );
            xTaskNotifyGive( samplingHandler );
            ESP_LOGI(TAG, "FireBase initialized");
            local_wifi_rgb_state &= ~(1UL << NO_USER);
        }
        else {
            ESP_LOGI(TAG, "FireBase init failed");
            local_wifi_rgb_state |= (1UL << NO_USER);
        }
    }

}

void loop() {

    if(!no_credentials) {        
        
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
                        ESP_LOGI(TAG, "Received sensor data to push");
                        ESP_LOGI(TAG, "Temperature: %.02f°C\nHumidity: %.02f%%\nLux: %u\nSoil moisture: %d%%\n", 
                            sensor_data_1.temperature, sensor_data_1.humidity, sensor_data_1.lux, sensor_data_1.soil_humidity);
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
    else {
        ESP_LOGI(TAG, "serverTask initialized");
        local_wifi_rgb_state |= (1UL << NO_WIFI_CRED);
        nvs_erase_wifi_creds( );
        wifi_init_softap( );
        for ( ; ; ) {
            vTaskDelay(1);
        }
    }
}
