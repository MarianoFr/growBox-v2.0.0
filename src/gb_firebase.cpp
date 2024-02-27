#include "gb_firebase.h"

/*Declare the Firebase Data object in the global scope*/
FirebaseData firebaseData2;
FirebaseData firebaseData1;

extern QueueHandle_t FB2outputs;

//Paths to firebase for multi stream
String childPath[15] = {"/TempCtrlHigh","/HumCtrlHigh","/TemperatureControl","/HumidityControl", "/AutomaticWatering",
                        "/HumidityOffHour", "/HumidityOnHour", "/TemperatureOffHour", "/TemperatureOnHour",
                        "/HumiditySet", "/OffHour", "/OnHour", "/SoilMoistureSet",
                        "/TemperatureSet", "/Water"
                       };
char path_to_dashboard[100];
char user_uid[50];
/*******************************
 Update variables from FireBase
*******************************/
void streamCallback(MultiPathStreamData data) {
    rx_control_update_t control_data_dummy;
    rx_control_data_t rx;
    for (size_t i = 0; i < 15; i++)
    {
        if (data.get(childPath[i])) {
            if (data.dataPath == "/TempCtrlHigh") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif
            if (data.value == "true")
                rx.temperature_control_high = true;
            else
                rx.temperature_control_high = false;
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.temperature_control_high);
        #endif
            } else if (data.dataPath == "/HumCtrlHigh") {
        #if SERIAL_DEBUG && FIRE_DEBUG      
            Serial.println(data.dataPath);
        #endif
            if (data.value == "true")
                rx.humidity_control_high = true;
            else
                rx.humidity_control_high = false;
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.humidity_control_high);
        #endif
            } else if (data.dataPath == "/HumidityControl") {
        #if SERIAL_DEBUG && FIRE_DEBUG      
            Serial.println(data.dataPath);
        #endif
            if (data.value == "true")
                rx.humidity_control = true;
            else
                rx.humidity_control = false;
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.humidity_control);
        #endif
            } else if (data.dataPath == "/TemperatureControl") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif        
            if (data.value == "true")
                rx.temperature_control = true;
            else
                rx.temperature_control = false;
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.temperature_control);
        #endif
            } else if (data.dataPath == "/AutomaticWatering") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif
            if (data.value == "true")
                rx.automatic_watering = true;
            else
                rx.automatic_watering = false;
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.automatic_watering);
        #endif
            } else if (data.dataPath == "/HumidityOffHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
                Serial.println(data.dataPath);
        #endif        
                rx.humidity_off_hour = data.value.toInt();
            #if SERIAL_DEBUG && FIRE_DEBUG
                Serial.println(rx.humidity_off_hour);
            #endif
            } else if (data.dataPath == "/HumidityOnHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG       
            Serial.println(data.dataPath);
        #endif
                rx.humidity_on_hour = data.value.toInt();
            #if SERIAL_DEBUG && FIRE_DEBUG
                Serial.println(rx.humidity_on_hour);
            #endif
            } else if (data.dataPath == "/TemperatureOffHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG      
            Serial.println(data.dataPath);
        #endif
            rx.temperature_off_hour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.temperature_off_hour);
        #endif
            } else if (data.dataPath == "/TemperatureOnHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif
            rx.temperature_on_hour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.temperature_on_hour);
        #endif
            } else if (data.dataPath == "/HumiditySet") {
        #if SERIAL_DEBUG && FIRE_DEBUG     
            Serial.println(data.dataPath);
        #endif        
            rx.humidity_set = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.humidity_set);
        #endif
            } else if (data.dataPath == "/OffHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG       
            Serial.println(data.dataPath);
        #endif
            rx.lights_off_hour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.lights_off_hour);
        #endif
            } else if (data.dataPath == "/OnHour") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif        
            rx.lights_on_hour = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.lights_on_hour);
        #endif
            } else if (data.dataPath == "/SoilMoistureSet") {
        #if SERIAL_DEBUG && FIRE_DEBUG        
            Serial.println(data.dataPath);
        #endif        
            rx.soil_moisture_set = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.soil_moisture_set);
        #endif
            } else if (data.dataPath == "/TemperatureSet") {
        #if SERIAL_DEBUG && FIRE_DEBUG    
            Serial.println(data.dataPath);
        #endif        
            rx.temperature_set = data.value.toInt();
        #if SERIAL_DEBUG && FIRE_DEBUG
            Serial.println(rx.temperature_set);
        #endif
            } else if (data.dataPath == "/Water") {
        #if SERIAL_DEBUG && FIRE_DEBUG      
            Serial.println(data.dataPath);
        #endif        
            if (data.value == "true")
                rx.water = true;
            else
                rx.water = false;
            }
        }
    }
    if(xQueueSend(FB2outputs, &rx, 10/portTICK_PERIOD_MS) != pdTRUE) {
        ESP_LOGI(FBCB_TAG, "Failed to update");
        xQueueReceive(FB2outputs, &control_data_dummy, 10/portTICK_PERIOD_MS);
        xQueueSend(FB2outputs, &rx, 10/portTICK_PERIOD_MS);
    }
  return;

}

//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout) {
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
void write2FBSensor (tx_sensor_data_t *sensor, FirebaseJson *dashBoard) {
  FirebaseJson json;
  (*dashBoard).add("ESPtag", String((*sensor).esp_tag));
#if SERIAL_DEBUG && ESPtag_DEBUG
  Serial.println(String((*tx).ESPtag));
#endif
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
  FirebaseJson json;
#if SERIAL_DEBUG && ESPtag_DEBUG
  Serial.println(String((*tx).ESPtag));
#endif
  (*dashBoard).add("HumidityControlOn", (*control).humidity_on);
  (*dashBoard).add("TemperatureControlOn", (*control).temperature_on);
  (*dashBoard).add("Lights", (*control).lights_on);
  (*dashBoard).add("Watering", (*control).water_on);
  //dashBoard->add(json);
}

bool fb_update_sensor(tx_sensor_data_t *sensor) {
    
    bool res = false;
    FirebaseJson dashBoard;
    write2FBSensor( sensor, &dashBoard );
        
    if(Firebase.updateNode(firebaseData2, path_to_dashboard, dashBoard)) {
    #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Successfully updated dashboard!!");
    #endif
        res = true;
    }
    else {
        #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Failed to update dashboard!!");
        #endif
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
    write2FBControl( control_data, &dashBoard );
        
    if(Firebase.updateNode(firebaseData2, path_to_dashboard, dashBoard)) {
    #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Successfully updated dashboard!!");
    #endif
        res = true;
    }
    else {
        #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Failed to update dashboard!!");
        #endif
        res = false;
    }
    return res;

}

bool gb_firebase_init( void ) {

    unsigned char mac_base[6] = {0};
    char gb_connected_path[50];    
    unsigned long start_uid_fetch = 0;
    time_t now = 0;
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

    sprintf(gb_connected_path, "/users/%02X:%02X:%02X:%02X:%02X:%02X/GBconnected/",mac_base[0]
            ,mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    Firebase.setBool(firebaseData2, gb_connected_path, true);
    sprintf(gb_connected_path, "/users/%02X:%02X:%02X:%02X:%02X:%02X/user/",mac_base[0]
            ,mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
    start_uid_fetch = millis();
    if (!Firebase.getString(firebaseData2, gb_connected_path, user_uid)) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        if (!Firebase.getString(firebaseData2, gb_connected_path, user_uid)) {
            vTaskDelay(1000/portTICK_PERIOD_MS);
            if (!Firebase.getString(firebaseData2, gb_connected_path, user_uid)) {
                Firebase.setString(firebaseData2, gb_connected_path, "NULL");
                //rgb_state |= 1UL << NO_USER;//TODO: specify these by returning false?
                sprintf(user_uid, "%s", "NULL");
            }
        }
    }
    while (strcmp(user_uid, "NULL") == 0 || (millis() - start_uid_fetch < UID_FETCH_TO)) {
        Firebase.getString(firebaseData2, gb_connected_path, user_uid);
        //debounceWiFiReset();//TODO: constantly run debounce on task? timer on main task?
        vTaskDelay(pdMS_TO_TICKS(1000/portTICK_PERIOD_MS));
    }

    if(strcmp(user_uid, "NULL") == 0)
        return false;

    sprintf(path_to_dashboard, "/growboxs/%s/dashboard", user_uid);
    sprintf(gb_connected_path, "/users/%s/control", user_uid);

    Firebase.setMultiPathStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);
    
    if(!Firebase.beginMultiPathStream(firebaseData1, gb_connected_path, childPath, sizeof(childPath)))
        return false;

    //Get number of resets and increase
    sprintf(nmbr_rst_path, "/growboxs/%s/dashboard/NmbrResets",user_uid);
    Firebase.getInt(firebaseData2, nmbr_rst_path, &nmbr_rst);
    nmbr_rst++;
    Firebase.setInt(firebaseData2, nmbr_rst_path, nmbr_rst);

    //Set first connection
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(esp_frst_conn, 50, "%Y-%m-%d %X", &timeinfo);
    sprintf(esp_frst_conn_path, "/growboxs/%s/dashboard/FirstConnection", user_uid);
    Firebase.setString(firebaseData2, esp_frst_conn_path, esp_frst_conn);

//  Firebase.setStreamTaskStackSize(20000);    
    return true;

}