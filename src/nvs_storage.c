#include "nvs_storage.h"

static const char *TAG = "**NVS**";

nvs_handle_t my_handle;
size_t size = WIFI_SSID_SIZE;
uint8_t written = 0; // value will default to 0, if not set yet in NVS

extern char wifi_ssid[];
extern char wifi_pass[];

void nvs_open_storage( ) {
    // Open
    esp_err_t err;
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle");    
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS_OPEN() OK");
    }
}

static bool in_range(int low, int high, int x) {

    return ((x-high)*(x-low) <= 0);

}

static bool check_variables( rx_control_data_t* control_data ) {

    bool res = false;
    if(in_range(0, 1, (int)control_data->automatic_watering)) {
        res = true;
    } else
        return false;
    if(in_range(0, 1, (int)control_data->humidity_control_high)) {
        res = true;
    } else
        return false;
    if(in_range(0, 1, (int)control_data->humidity_control)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->humidity_off_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->humidity_on_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 100, control_data->humidity_set)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->lights_off_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->lights_on_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 50, control_data->soil_moisture_set)) {
        res = true;
    } else
        return false;
    if(in_range(0, 1, control_data->temperature_control_high)) {
        res = true;
    } else
        return false;
    if(in_range(0, 1, control_data->temperature_control)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->temperature_off_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 24, control_data->temperature_on_hour)) {
        res = true;
    } else
        return false;
    if(in_range(0, 40, control_data->temperature_set)) {
        res = true;
    } else
        return false;
    if(in_range(0, 1, control_data->water)) {
        res = true;
    } else
        return false;
    return res;
}

uint8_t nvs_read_control_variables( rx_control_data_t* rx_data ) {
    // Read
    esp_err_t err;
    uint8_t res = false;
    ESP_LOGI(TAG, "NVS_READ_CONTROL");

    nvs_open_storage( );
    // Read the size of memory space required for blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, "control_data", NULL, &required_size);
    if (required_size == 0) {
        printf("Nothing saved yet!\n");
        res = ERROR_READING_MEM;
        return res;
    }
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    //Value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, "control_data", rx_data, &required_size);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG,"READ_CONTROL_DATA OK\n");
            if(check_variables(rx_data))
                res = VARIABLES_OK;
            else
                res = ERROR_READING_MEM;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG,"CONTROL_DATA NOT WRITTEN\n");
            res = WAIT_VARIABLES_FROM_FB;
            break;
        default :
            ESP_LOGI(TAG,"ERROR READ_CONTROL_DATA\n");
            res = ERROR_READING_MEM;
            break;
    }
    nvs_close(my_handle);

    return res;
}

uint8_t nvs_write_control_variables( rx_control_data_t* rx_data ) {
    // Read
    esp_err_t err;
    size_t required_size = sizeof(rx_control_data_t);
    uint8_t res = VARIABLES_OK;
    ESP_LOGI(TAG, "Writing GB NVS");

    nvs_open_storage( );
    
    err = nvs_set_blob(my_handle, "control_data", rx_data, required_size);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG,"WRITE_CONTROL_DATA OK\n");
            res = VARIABLES_OK;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG,"CONTROL_DATA NOT WRITTEN\n");
            res = VARIABLES_OK;
            break;
        default :
            ESP_LOGI(TAG,"ERROR WRITE_CONTROL_DATA\n");
            res = ERROR_WRITING_MEM;
            break;
    }
    err = nvs_commit(my_handle);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "Failed to write control variables");
        nvs_close(my_handle);
        return ERROR_WRITING_MEM;
    }
    nvs_close(my_handle);
    if(nvs_read_control_variables(rx_data) == VARIABLES_OK) {
        res = VARIABLES_OK;
    } else {
        res = ERROR_WRITING_MEM;
    }
    return res;
}

bool nvs_write_wifi_creds( char* ssid, char* pass ) {
    //Write wifi creds to NVS
    esp_err_t err;
    nvs_open_storage( );

    err = nvs_set_str(my_handle, "ssid", ssid);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to write WiFi ssid");
        nvs_close(my_handle);
        return false;
    }
    err = nvs_set_str(my_handle, "pass", pass);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "Failed to write WiFi pass");
        nvs_close(my_handle);
        return false;
    }
    written = 1;
    err = nvs_set_u8(my_handle, "written", written);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "Failed to write WiFi pass");
        nvs_close(my_handle);
        return false;
    }
    ESP_LOGI(TAG, "Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    if(err != ESP_OK){
        ESP_LOGI(TAG, "Failed to write WiFi pass");
        nvs_close(my_handle);
        return false;
    }
    nvs_close(my_handle);
   
    return true;
}

bool nvs_read_wifi_creds( char* ssid, char* pass ) {    

    ESP_LOGI(TAG, "Reading WiFi-Creds");
#if DEBUG
    char debug_ssid[30] = DEBUG_SSID;
    char debug_pass[30] = DEBUG_PASS;
    memcpy(ssid, debug_ssid, 30);
    memcpy(pass, debug_pass, 30);
#else
    esp_err_t err;
    
    nvs_open_storage( );

    //Value will default to 0, if not set yet in NVS
    err = nvs_get_u8(my_handle, "written", &written);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Done\n");
            ESP_LOGI(TAG, "Wifi written flag is %d\n", written);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "Wifi written flag not initialized yet!\n");
            nvs_close(my_handle);
            return false;
            break;
        default :
            ESP_LOGI(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
            nvs_close(my_handle);
            return false;
            break;
    }
    if(written == 1) {
        //read ssid
        err = nvs_get_str(my_handle, "ssid", ssid, &size);
        if(err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to read WiFi ssid");
            //TODO: what to do in case of read fail
            //even when written flag is set
            nvs_close(my_handle);
            return false;
        }
        else {
            ESP_LOGI(TAG, "SSID: %s", ssid);
        }
        //read pass
        size = 30;
        err = nvs_get_str(my_handle, "pass", pass, &size);
        if(err != ESP_OK){
            ESP_LOGI(TAG, "Failed to read WiFi pass");
            nvs_close(my_handle);
            return false;
        }
        else {
            ESP_LOGI(TAG, "PASS: %s", pass);
        }
    }
    nvs_close(my_handle);
#endif
    return true;
}

bool nvs_erase_wifi_creds( void ) {

    esp_err_t err;
    bool res = false;

    ESP_LOGI(TAG, "Erasing WiFi-Creds");
    nvs_open_storage( );

    err = nvs_erase_key(my_handle, "written");
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to erase WiFi creds");
        //TODO: what to do in case of read fail
        res = false;
    }
    err = nvs_erase_key(my_handle, "pass");
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to erase WiFi pass");
        res = false;
    }
    err = nvs_erase_key(my_handle, "ssid");
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to erase WiFi ssid");
        res = false;
    }
    else {
        res = true;
    }
    nvs_close(my_handle);
    return res;
}

bool nvs_erase_storage( void ) {
    esp_err_t err;
    nvs_open_storage( );
    err = nvs_erase_all(my_handle);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to erase NVS storage");
        return false;
    }
    ESP_LOGI(TAG, "Erased NVS storage");
    return true;
}