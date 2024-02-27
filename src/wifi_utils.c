/* WiFi station Example

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/
#include "wifi_utils.h"

const char INDEX_HTML[] =
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta content='text/html' charset='UTF-8'"
  " http-equiv=\"content-type\">"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>Internet para su GrowBox</title>"
  "<style>"
  "body { background-color: #78288C; font-family: Arial, Helvetica, Sans-Serif; Color: white; }"
  "h1 { color: yellow; }"
  "h3 { color: yellow; }"
  ".form { margin-left: 100px; margin-top: 25px; }"
  "</style>"
  "</head>"
  "<body>"  
  "<h1>Bienvenido a GrowBox</h1>"
  "<h3>El sistema de automatización de cultivos de interior.</h3>"
  "<ul>"
  "<li>Por favor, introduzca el nombre(SSID) y la contraseña de su red de WiFi, ambos con menos de 30 caracteres, luego presione aceptar</li>"
  "<li>Con esto su GrowBox ya estará conectado y podrá usarlo con la aplicación desde su celular.</li>"
  "<li>Si tiene problemas para iniciar su GrowBox, no dude en comunicarse con el centro de atencion al cliente.</li>"
  "<li>Puede contactarse por mail o por instagram</li>"
  "<li>growboxcultivo@gmail.com | IG: growbox1.0</li>"
  "</ul>"
  "<FORM action=\"/\" method=\"post\">"
  "<div>"
  "<label>ssid:&nbsp;</label>"
  "<input maxlength=\"30\" name=\"ssid\"><br>"
  "<label>contraseña:&nbsp;</label><input maxlength=\"30\" name=\"pass\"><br>"
  "<INPUT type=\"submit\" value=\"Aceptar\"> <INPUT type=\"Reset\">"
  "</div>"
  "<br>"
  "</FORM>"
  "</body>"
  "</html>";

#define TAG "**WIFI_UTILS**"

static int s_retry_num = 0;

static httpd_handle_t http_server_instance = NULL; // Pntr to server object

EventGroupHandle_t s_wifi_event_group;

extern TaskHandle_t outputsHandler;
extern TaskHandle_t wiFiHandler;
extern TaskHandle_t samplingHandler;

char wifi_ssid[30];
char wifi_pass[30];

/*Get method call-back*/ 
static esp_err_t get_method_handler(httpd_req_t* http_req) {

    ESP_LOGI("HANDLER","This is the handler for the HTTP_GET <%s> URI", http_req->uri);
    httpd_resp_send(http_req, INDEX_HTML, sizeof(INDEX_HTML));
    return ESP_OK;

}

/*Get method call-back*/ 
static esp_err_t post_method_handler(httpd_req_t* http_req) {

    ESP_LOGI(TAG,"This is the handler for the HTTP_POST <%s> URI", http_req->uri);
    char buf[100];
    int ret, remaining = http_req->content_len;    

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(http_req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
        
        char* begin_ssid = strstr( buf, "ssid" );
        char* begin_pass = strstr( buf, "pass" );

        size_t ssid_len = (begin_pass - 1) - (begin_ssid + 5);
        size_t pass_len = buf + ret - (begin_pass + 5);
        ESP_LOGI(TAG, "%u", ssid_len);
        ESP_LOGI(TAG, "%u", pass_len);
        memcpy(wifi_ssid, begin_ssid + 5, ssid_len );
        memcpy(wifi_pass, begin_pass + 5, pass_len ); 
        wifi_pass[pass_len] = '\0';
        wifi_ssid[ssid_len] = '\0';
        ESP_LOGI(TAG, "SSID: %s", wifi_ssid);
        ESP_LOGI(TAG, "PASS: %s", wifi_pass);
        char response[450] =
            "<head>"
            "<meta content='text/html' charset='UTF-8'"
            " http-equiv=\"content-type\">"
            "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
            "<title>Internet para su GrowBox</title>"
            "<style>"
            "body { background-color: #78288C; font-family: Arial, Helvetica, Sans-Serif; Color: white; }"
            "</style>"
            "</head>"
            "<p>El nombre de la red WiFi es ";
        strcat(response, wifi_ssid);
        strcat(response, "<br>");
        strcat(response, "Y su contraseña es ");
        strcat(response, wifi_pass);
        strcat(response, "</P><BR>");
        strcat(response, "<H2><a href=\"/\">go home</a></H2><br>");
        if(nvs_write_wifi_creds( wifi_ssid, wifi_pass )) {
            nvs_read_wifi_creds( wifi_ssid, wifi_ssid );
            httpd_resp_send(http_req, response, sizeof(response));
            ESP_LOGI(TAG, "WiFi creds stored");
            vTaskDelay(100);
            esp_restart( );
        }
        else {
            nvs_erase_wifi_creds( );
            ESP_LOGI(TAG, "Failed to store wifi creds");
            vTaskDelay(100);
            esp_restart( );
        }
    }
    return ESP_OK;

}

/*httpd_uri_t HTTP_GET call-back subscription element*/ 
static httpd_uri_t get_uri = {
    .uri       = URI_STRING,
    .method    = HTTP_GET,
    .handler   = get_method_handler,
    .user_ctx  = NULL,
};

/*httpd_uri_t HTTP_POST call-back subscription element*/ 
static const httpd_uri_t post_uri = {
    .uri       = "/",
    .method    = HTTP_POST,
    .handler   = post_method_handler,
    .user_ctx  = NULL
};

/*Start http server*/ 
static void start_http_server(void) {

    httpd_config_t http_server_configuration = HTTPD_DEFAULT_CONFIG();
    http_server_configuration.server_port = SERVER_PORT;
    if(httpd_start(&http_server_instance, &http_server_configuration) == ESP_OK){
        ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_instance, &get_uri));
        ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_instance, &post_uri));
    }

}
/*Stop http server*/ 
static void stop_http_server(void){
    if(http_server_instance != NULL){
        ESP_ERROR_CHECK(httpd_stop(http_server_instance));
    }
}
/*Wifi event handler*/
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data) {

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        start_http_server();
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
        stop_http_server();
    }

}

void wifi_init_softap(void) {
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);

}