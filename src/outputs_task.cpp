#include "outputs_task.h"

TaskHandle_t outputsHandler;

extern QueueHandle_t FB2outputs;
extern QueueHandle_t sensors2outputs;
extern QueueHandle_t outputs2FB;
extern QueueHandle_t sensors2FB;
extern QueueHandle_t wifiRgbState;
extern QueueHandle_t samplerRgbState;
extern QueueHandle_t outputs2sampler;

static TimerHandle_t xRgbTimer;
static TimerHandle_t xSingleWaterTimer;
static TimerHandle_t xWaterTimer;

rx_control_data_t rx_data;
tx_sensor_data_t sensor_data_2;

tx_control_data_t curr_tx_data;
tx_control_data_t prev_tx_data;

static const char *TAG = "**OUTPUTS_TASK**";

rgb_state_t outputs_rgb_state = 0;
rgb_state_t sampler_rgb_state = 0;
rgb_state_t wifi_rgb_state    = 0;
    
uint8_t current_hour;
uint8_t out_nmbr_outputs      = 0;

bool water_error              = false;
uint8_t water_toggle_cnt      = 0;

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_RED_CHANNEL        LEDC_CHANNEL_0
#define LEDC_GREEN_CHANNEL      LEDC_CHANNEL_1
#define LEDC_BLUE_CHANNEL       LEDC_CHANNEL_2
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT // Set duty resolution to 8 bits
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

/***********************************
 * Init RGB led driver
 ***********************************/
static void rgb_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_red_channel = {
        .gpio_num       = PIN_RED,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_RED_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_green_channel = {
        .gpio_num       = PIN_GREEN,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_GREEN_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_blue_channel = {
        .gpio_num       = PIN_BLUE,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_BLUE_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_red_channel));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_green_channel));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_blue_channel));
}

/***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
static void RGBalert(TimerHandle_t xTimer) {

  static uint8_t green = 100, blue = 0, red = 120;
  char aux_2[100];
  char path_char[100];
  static uint8_t try_out = 0;

  xQueueReceive(wifiRgbState, &wifi_rgb_state, 0);
  xQueueReceive(samplerRgbState, &sampler_rgb_state, 0);
  outputs_rgb_state = outputs_rgb_state | sampler_rgb_state | wifi_rgb_state;
  switch (try_out)
  {
  case WIFI_DISC:
    if ((outputs_rgb_state >> WIFI_DISC) & 1U) {
      green = 0;
      red = 0;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Negro");
      ESP_LOGI(TAG, "WIFI_DISC");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case WIFI_CONN:
    if ((outputs_rgb_state >> WIFI_CONN) & 1U) {
      green = 0;
      red = 35;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Violeta");
      ESP_LOGI(TAG, "WIFI_CONN");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case NO_WIFI_CRED:
    if ((outputs_rgb_state >> NO_WIFI_CRED) & 1U) {
      green = 0;
      red = 255;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Rojo");
      ESP_LOGI(TAG, "NO_WIFI_CRED"); 
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case NO_USER:
    if ((outputs_rgb_state >> NO_USER) & 1U) {
      green = 255;
      red = 0;
      blue = 0;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Verde");
      ESP_LOGI(TAG, "NO_USER"); //
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case HTU21_ERR:
    if ((outputs_rgb_state >> HTU21_ERR) & 1U) {
      green = 0;
      red = 0;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Azul");
      ESP_LOGI(TAG, "HTU21_ERR");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case RTC_ERR:
    if ((outputs_rgb_state >> RTC_ERR) & 1U) {
      green = 255;
      red = 255;
      blue = 50;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Amarillo");
      ESP_LOGI(TAG, "RTC_ERR");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case SOIL_ERR:
    if ((outputs_rgb_state >> SOIL_ERR) & 1U) {
      green = 100;
      red = 120;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Blanco");
      ESP_LOGI(TAG, "SOIL_ERR");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  case BH1750_ERR:
    if ((outputs_rgb_state >> BH1750_ERR) & 1U) {
      green = 0;
      red = 150;
      blue = 255;
#if SERIAL_DEBUG && RGB_DEBUG
      ESP_LOGI(TAG, "Rosa");
      ESP_LOGI(TAG, "BH_1750_ERR");
#endif
      ledc_set_duty(LEDC_MODE, LEDC_RED_CHANNEL, red);
      ledc_set_duty(LEDC_MODE, LEDC_GREEN_CHANNEL, green);
      ledc_set_duty(LEDC_MODE, LEDC_BLUE_CHANNEL, blue);
      ledc_update_duty(LEDC_MODE, LEDC_RED_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_GREEN_CHANNEL);
      ledc_update_duty(LEDC_MODE, LEDC_BLUE_CHANNEL);
    }
    break;
  default:
    break;
  }
  try_out++;
  if (try_out > 7)
    try_out = 0;
  return;
}

static bool in_range(int low, int high, int x) {
    return ((x-high)*(x-low) <= 0);
}

static bool update_control( rx_control_update_t* update_rx_data ) {
    bool res = false;
    ESP_LOGI(TAG, "Update control %d", update_rx_data->var_2_update);  
    if ((update_rx_data->var_2_update >> UPDT_AUTO_WATER) & 1U) {
        if(in_range(0, 1, (int)update_rx_data->control_data.automatic_watering)) {
            rx_data.automatic_watering = update_rx_data->control_data.automatic_watering;
            ESP_LOGI(TAG, "automatic_watering updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_HUM_CTRL_H) & 1U) {
        if(in_range(0, 1, (int)update_rx_data->control_data.humidity_control_high)) {
            rx_data.humidity_control_high = update_rx_data->control_data.humidity_control_high;
            ESP_LOGI(TAG, "humidity_control_high updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_HUM_CTRL_ON) & 1U) {
        if(in_range(0, 1, (int)update_rx_data->control_data.humidity_control)) {
            rx_data.humidity_control = update_rx_data->control_data.humidity_control;
            ESP_LOGI(TAG, "humidity_control updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_HUM_OFF_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.humidity_off_hour)) {
            rx_data.humidity_off_hour = update_rx_data->control_data.humidity_off_hour;
            ESP_LOGI(TAG, "humidity_off_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_HUM_ON_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.humidity_on_hour)) {
            rx_data.humidity_on_hour = update_rx_data->control_data.humidity_on_hour;
            ESP_LOGI(TAG, "humidity_on_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_HUM_SET) & 1U) {
        if(in_range(0, 100, update_rx_data->control_data.humidity_set)) {
            rx_data.humidity_set = update_rx_data->control_data.humidity_set;
            ESP_LOGI(TAG, "humidity_set updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_LIGHTS_OFF_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.lights_off_hour)) {
            rx_data.lights_off_hour = update_rx_data->control_data.lights_off_hour;
            ESP_LOGI(TAG, "lights_off_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_LIGHTS_ON_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.lights_on_hour)) {
            rx_data.lights_on_hour = update_rx_data->control_data.lights_on_hour;
            ESP_LOGI(TAG, "lights_on_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_SOIL_SET) & 1U) {
        if(in_range(0, 50, update_rx_data->control_data.soil_moisture_set)) {
            rx_data.soil_moisture_set = update_rx_data->control_data.soil_moisture_set;
            ESP_LOGI(TAG, "soil_moisture_set updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_TEMP_CTRL_H) & 1U) {
        if(in_range(0, 1, update_rx_data->control_data.temperature_control_high)) {
            rx_data.temperature_control_high = update_rx_data->control_data.temperature_control_high;
            ESP_LOGI(TAG, "temperature_control_high updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_TEMP_CTRL_ON) & 1U) {
        if(in_range(0, 1, update_rx_data->control_data.temperature_control)) {
            rx_data.temperature_control = update_rx_data->control_data.temperature_control;
            ESP_LOGI(TAG, "temperature_control updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_TEMP_OFF_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.temperature_off_hour)) {
            rx_data.temperature_off_hour = update_rx_data->control_data.temperature_off_hour;
            ESP_LOGI(TAG, "temperature_off_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_TEMP_ON_HR) & 1U) {
        if(in_range(0, 24, update_rx_data->control_data.temperature_on_hour)) {
            rx_data.temperature_on_hour = update_rx_data->control_data.temperature_on_hour;
            ESP_LOGI(TAG, "temperature_on_hour updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_TEMP_SET) & 1U) {
        if(in_range(0, 40, update_rx_data->control_data.temperature_set)) {
            rx_data.temperature_set = update_rx_data->control_data.temperature_set;
            ESP_LOGI(TAG, "temperature_set updated");
            res = true;
        }
    }
    if ((update_rx_data->var_2_update >> UPDT_WATER_ON) & 1U) {
        if(in_range(0, 1, update_rx_data->control_data.water)) {
            rx_data.water = update_rx_data->control_data.water;
            ESP_LOGI(TAG, "water updated");
            res = true;
        }
    }
    return res;
}

static void control_lights( ) {

    if ( ( rx_data.lights_off_hour == rx_data.lights_on_hour || 
         ( rx_data.lights_off_hour == 24 && rx_data.lights_on_hour == 0 ) ||
         ( rx_data.lights_off_hour == 0 && rx_data.lights_on_hour == 24 ) ) && curr_tx_data.lights_on ) {
        gpio_set_level((gpio_num_t)LIGHTS, 1);//lights off
        ESP_LOGI(TAG, "Lights OFF");
        curr_tx_data.lights_on = false;
        if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        return;
    }

    else if ( rx_data.lights_off_hour > rx_data.lights_on_hour ) {
        if ( (current_hour >= rx_data.lights_on_hour) && (current_hour < rx_data.lights_off_hour) && !curr_tx_data.lights_on ) {
            gpio_set_level((gpio_num_t)LIGHTS, 0);//lights on
            ESP_LOGI(TAG, "Lights ON");
            curr_tx_data.lights_on = true;
            if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
        }
        else if ( ((current_hour >= rx_data.lights_off_hour) || (current_hour < rx_data.lights_on_hour)) && curr_tx_data.lights_on ) {
            gpio_set_level((gpio_num_t)LIGHTS, 1);//lights off
            ESP_LOGI(TAG, "Lights OFF");
            curr_tx_data.lights_on = false;
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }
        return;
    }
    else if ( rx_data.lights_off_hour < rx_data.lights_on_hour ) {
        if ( ((current_hour >= rx_data.lights_on_hour) || (current_hour < rx_data.lights_off_hour)) && !curr_tx_data.lights_on ) {
            gpio_set_level((gpio_num_t)LIGHTS, 0);//lights on
            ESP_LOGI(TAG, "Lights ON");
            curr_tx_data.lights_on = true;
            if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
        }
        else if ( (current_hour < rx_data.lights_on_hour) && (current_hour >= rx_data.lights_off_hour) && curr_tx_data.lights_on) {
            gpio_set_level((gpio_num_t)LIGHTS, 1);//lights off
            ESP_LOGI(TAG, "Lights OFF");
            curr_tx_data.lights_on = false;
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }
        return;
    }
  
}

static void control_temperature( ) {

    if ( rx_data.temperature_control ) {
        if ((outputs_rgb_state >> HTU21_ERR) & 1U) {
            gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//TEMP_CTRL_OUTPUT off if HTU error
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            return;
        }
        if(rx_data.temperature_control_high) {
            if ( (sensor_data_2.temperature > (rx_data.temperature_set)) && !curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 0);//temperatureControlOn on
                ESP_LOGI(TAG, "Temperature ON");
                curr_tx_data.temperature_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            if ( (sensor_data_2.temperature <= (rx_data.temperature_set - TEMPERATURE_HISTERESIS)) && curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//temperatureControlOn off
                ESP_LOGI(TAG, "Temperature OFF");
                curr_tx_data.temperature_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }
        else {
            if ( (sensor_data_2.temperature < (rx_data.temperature_set)) && !curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 0);//temperatureControlOn on
                ESP_LOGI(TAG, "Temperature ON");
                curr_tx_data.temperature_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            if ( (sensor_data_2.temperature >= (rx_data.temperature_set + TEMPERATURE_HISTERESIS)) && curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//temperatureControlOn off
                ESP_LOGI(TAG, "Temperature OFF");
                curr_tx_data.temperature_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }
    }
    else {
        /*Analize temperature's period*/
        if ( ( rx_data.temperature_off_hour == rx_data.temperature_on_hour ||
             ( rx_data.temperature_off_hour == 24 && rx_data.temperature_on_hour == 0 ) ||
             ( rx_data.temperature_off_hour == 0 && rx_data.temperature_on_hour == 24 ) ) && curr_tx_data.temperature_on) {
            
            gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//temperatureControl off
            ESP_LOGI(TAG, "Temperature OFF");
            curr_tx_data.temperature_on = false;
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }
        else if ( rx_data.temperature_off_hour > rx_data.temperature_on_hour ) {
            if ( (current_hour >= rx_data.temperature_on_hour) && (current_hour < rx_data.temperature_off_hour) && !curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 0);//temperatureControl on
                ESP_LOGI(TAG, "Temperature ON");
                curr_tx_data.temperature_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            else if ( ( (current_hour >= rx_data.temperature_off_hour) || (current_hour < rx_data.temperature_on_hour) ) && curr_tx_data.temperature_on ) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//temperatureControl off
                ESP_LOGI(TAG, "Temperature OFF");
                curr_tx_data.temperature_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }
        else if ( rx_data.temperature_off_hour < rx_data.temperature_on_hour ) {
            if ( ( (current_hour >= rx_data.temperature_on_hour) || (current_hour < rx_data.temperature_off_hour) ) && !curr_tx_data.temperature_on )  {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 0);//temperatureControl on
                ESP_LOGI(TAG, "Temperature ON");
                curr_tx_data.temperature_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            else if ( (current_hour < rx_data.temperature_on_hour) && (current_hour >= rx_data.temperature_off_hour) && curr_tx_data.temperature_on) {
                gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);//temperatureControl off
                ESP_LOGI(TAG, "Temperature OFF");
                curr_tx_data.temperature_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }  
    }    
}

static void control_humity( ) {
    if( rx_data.humidity_control ) {
      if(rx_data.humidity_control_high) {
        if ( (sensor_data_2.humidity > (rx_data.humidity_set + HUMIDITY_HISTERESIS)) && !curr_tx_data.humidity_on ) {
          gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 0);//humidity_control on
          ESP_LOGI(TAG, "Humidity ON");
          curr_tx_data.humidity_on = true;
          if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
        }
        if ( (sensor_data_2.humidity < (rx_data.humidity_set - HUMIDITY_HISTERESIS)) && curr_tx_data.humidity_on ) {
          gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);//humidity_control off
          ESP_LOGI(TAG, "Humidity OFF");
          curr_tx_data.humidity_on = false;
          if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }
      }
      else {
        if ( (sensor_data_2.humidity < (rx_data.humidity_set)) && !curr_tx_data.humidity_on ) {
          gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 0);//humidity_control on
          ESP_LOGI(TAG, "Humidity ON");
          curr_tx_data.humidity_on = true;
          if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
        }
        if ( (sensor_data_2.humidity > (rx_data.humidity_set + 2)) && curr_tx_data.humidity_on ) {
          gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);//humidity_control off
          ESP_LOGI(TAG, "Humidity OFF");
          curr_tx_data.humidity_on = false;
          if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }
      }
    }
    else {
        if ( ( rx_data.humidity_off_hour == rx_data.humidity_on_hour ||
             ( rx_data.humidity_off_hour == 24 && rx_data.humidity_on_hour == 0 ) ||
             ( rx_data.humidity_off_hour == 0 && rx_data.humidity_on_hour == 24 ) ) && curr_tx_data.humidity_on ) {

            gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);//humidity_control off
            ESP_LOGI(TAG, "Humidity OFF");
            curr_tx_data.humidity_on = false;
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        }

        else if ( rx_data.humidity_off_hour > rx_data.humidity_on_hour ) {
            if ( (current_hour >= rx_data.humidity_on_hour) && (current_hour < rx_data.humidity_off_hour) && !curr_tx_data.humidity_on ) {
                gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 0);//humidity_control on
                ESP_LOGI(TAG, "Humidity ON");
                curr_tx_data.humidity_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            else if ( ( (current_hour >= rx_data.humidity_off_hour) || (current_hour < rx_data.humidity_on_hour) ) && curr_tx_data.humidity_on ) {
                gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);//humidity_control off
                ESP_LOGI(TAG, "Humidity OFF");
                curr_tx_data.humidity_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }

        else if ( rx_data.humidity_off_hour < rx_data.humidity_on_hour ) {
            if ( ( (current_hour >= rx_data.humidity_on_hour) || (current_hour < rx_data.humidity_off_hour) ) && !curr_tx_data.humidity_on ) {
                gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 0);//humidity_control on
                ESP_LOGI(TAG, "Humidity ON");
                curr_tx_data.humidity_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
            }
            else if ( (current_hour < rx_data.humidity_on_hour) && (current_hour >= rx_data.humidity_off_hour) && curr_tx_data.humidity_on) {
                gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);//humidity_control off
                ESP_LOGI(TAG, "Humidity OFF");
                curr_tx_data.humidity_on = false;
                if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            }
        }
    }
}


static void single_water_tmr_cb(TimerHandle_t xTimer) {
    gpio_set_level((gpio_num_t)W_VLV_PIN, 1);//water off
    ESP_LOGI(TAG, "Water Off");
    rx_data.water = false;
    curr_tx_data.water_on = false;
    if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
}

static void water_tmr_cb(TimerHandle_t xTimer) {

    static bool watering = true;//first timer done it will be watering
    if(watering) {
        gpio_set_level((gpio_num_t)W_VLV_PIN, 1);//water off
        ESP_LOGI(TAG, "Water Off");
        water_toggle_cnt++;
        curr_tx_data.water_on = false;
        if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
        watering = false;
    }
    else {        
        gpio_set_level((gpio_num_t)W_VLV_PIN, 0);//water on
        ESP_LOGI(TAG, "Water On");
        curr_tx_data.water_on = true;
        if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;
        watering = true;
    }  
}

static void control_soil( ) {

    if(water_error && !gpio_get_level((gpio_num_t)W_VLV_PIN)) {//Low level, output is active
        gpio_set_level((gpio_num_t)W_VLV_PIN, 1);//water off
        ESP_LOGI(TAG, "Water error water Off");
        xTimerStop(xWaterTimer,pdMS_TO_TICKS(1));
        xTimerStop(xSingleWaterTimer,pdMS_TO_TICKS(1));
        curr_tx_data.water_on = false;
        return;
    }
    else if(rx_data.automatic_watering) {
        if(sensor_data_2.soil_humidity < rx_data.soil_moisture_set - SOIL_HISTERESIS) {
            if(!xTimerIsTimerActive(xWaterTimer)) {
                gpio_set_level((gpio_num_t)W_VLV_PIN, 0);//water on
                ESP_LOGI(TAG, "Water ON");
                curr_tx_data.water_on = true;
                if (++out_nmbr_outputs > 4) out_nmbr_outputs = 4;             
                configASSERT( xTimerStart(xWaterTimer,pdMS_TO_TICKS(1)) );
            }
        }
        if((sensor_data_2.soil_humidity > rx_data.soil_moisture_set + SOIL_HISTERESIS) && curr_tx_data.water_on) {
            gpio_set_level((gpio_num_t)W_VLV_PIN, 1);//water off
            ESP_LOGI(TAG, "Water Off");            
            if(water_toggle_cnt > MAX_WATER_TOGGLE) {
                water_error = true;
            }
            curr_tx_data.water_on = false;
            water_toggle_cnt = 0; 
            if (--out_nmbr_outputs < 0) out_nmbr_outputs = 0;
            if(xTimerIsTimerActive(xWaterTimer)) {
                configASSERT(xTimerStop(xWaterTimer,pdMS_TO_TICKS(1)));
            }
        }
    }
    else if(rx_data.water && !curr_tx_data.water_on) {
        if(!xTimerIsTimerActive(xSingleWaterTimer)) {
            gpio_set_level((gpio_num_t)W_VLV_PIN, 0);//water on
            ESP_LOGI(TAG, "Water ON");
            curr_tx_data.water_on = true;
            configASSERT( xTimerStart(xSingleWaterTimer,pdMS_TO_TICKS(1)) );
        }
    }
    else if(!rx_data.water && curr_tx_data.water_on) {
        gpio_set_level((gpio_num_t)W_VLV_PIN, 1);//water off
        ESP_LOGI(TAG, "Water OFF");
        curr_tx_data.water_on = false;
        configASSERT( xTimerStart(xSingleWaterTimer,pdMS_TO_TICKS(1)) );
    }
}

static esp_err_t gpio_start() {
    esp_err_t res = ESP_OK;
    gpio_config_t io_conf = {};

    gpio_set_level((gpio_num_t)PIN_RED, 1);
    gpio_set_level((gpio_num_t)PIN_GREEN, 1);
    gpio_set_level((gpio_num_t)PIN_BLUE, 1);
    gpio_set_level((gpio_num_t)HUM_CTRL_OUTPUT, 1);
    gpio_set_level((gpio_num_t)TEMP_CTRL_OUTPUT, 1);
    gpio_set_level((gpio_num_t)W_VLV_PIN, 1);
    gpio_set_level((gpio_num_t)LIGHTS, 1);

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ((1ULL<<PIN_RED) | (1ULL<<PIN_GREEN) |
                            (1ULL<<PIN_BLUE) | (1ULL<<HUM_CTRL_OUTPUT) |
                            (1ULL<<TEMP_CTRL_OUTPUT) | (1ULL<<W_VLV_PIN) |
                            (1ULL<<LIGHTS));
    //disable pull-down mode
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    //disable pull-up mode
    io_conf.pull_up_en = (gpio_pullup_t)0;
    //configure GPIO with the given settings
    res = gpio_config(&io_conf);
    return res;
}

void outputsTask ( void* pvParameters ) {

    rx_control_update_t rx_data_update;
    uint8_t control_variables_status = WAIT_VARIABLES_FROM_FB;
    time_t now = 0;
    struct tm timeinfo = { 0 };

    ESP_ERROR_CHECK(gpio_start());

    rgb_ledc_init( );
    outputs_rgb_state |= 1UL << RTC_ERR;
    xRgbTimer = xTimerCreate("RGB timer", 500 / portTICK_PERIOD_MS, pdTRUE, (void *)0, RGBalert);
    configASSERT(xRgbTimer);
    configASSERT(xTimerStart(xRgbTimer, 10 / portTICK_PERIOD_MS));

    xSingleWaterTimer = xTimerCreate( "Single water timer", pdMS_TO_TICKS(WATERING_DELAY), pdFALSE, ( void * ) 0, single_water_tmr_cb );
    configASSERT( xSingleWaterTimer );

    xWaterTimer = xTimerCreate( "Water timer", pdMS_TO_TICKS(WATERING_DELAY), pdTRUE, ( void * ) 0, water_tmr_cb );
    configASSERT( xWaterTimer );

#if !DEBUG_CTRL_VARS_MEM
    control_variables_status = nvs_read_control_variables(&rx_data);
#endif
    switch(control_variables_status) {
        case WAIT_VARIABLES_FROM_FB:
            ESP_LOGI(TAG, "WAIT_VARIABLES_FROM_FB");
            break;
        case VARIABLES_OK:
            ESP_LOGI(TAG, "VARIABLES_OK");
            break;
        case ERROR_READING_MEM:
            //TODO: solve error nvs mem
            ESP_LOGI(TAG, "ERROR_READING_MEM");
            break;
        default:
            break;
    }

    ESP_LOGI(TAG, "outputsTask initialized");

    /* Block to wait for wifi_task to synch sntp time to notify this task. */
    xTaskNotifyWait( 0x01, 0x01, NULL, portMAX_DELAY );
    ESP_LOGI(TAG, "OutputsTask notified by wifi utils");
    outputs_rgb_state &= ~(1UL << RTC_ERR);

    for ( ; ; ) {

        time(&now);
        localtime_r(&now, &timeinfo);
        current_hour = timeinfo.tm_hour;

        if(xQueueReceive(FB2outputs, &rx_data_update, 10/portTICK_PERIOD_MS) == pdTRUE) {
            ESP_LOGD(TAG, "New cotrol data received from FB");
            if(update_control(&rx_data_update)) {
                ESP_LOGD(TAG, "New cotrol data updated from FB");
            }
            if(nvs_write_control_variables(&rx_data_update.control_data)) {
                ESP_LOGD(TAG, "New cotrol data saved");
            }
        }
        if(xQueueReceive(sensors2outputs, &sensor_data_2, 10/portTICK_PERIOD_MS) == pdTRUE) {
            ESP_LOGD(TAG, "New sensor data received from sampler");
        }
        if(control_variables_status != WAIT_VARIABLES_FROM_FB) {
            control_lights( );
            control_temperature( );
            control_humity( );
            control_soil( );
        }        
        if(curr_tx_data.lights_on != prev_tx_data.lights_on
            || curr_tx_data.temperature_on != prev_tx_data.temperature_on
            || curr_tx_data.humidity_on != prev_tx_data.humidity_on
            || curr_tx_data.water_on != prev_tx_data.water_on) {
            prev_tx_data = curr_tx_data;
            if(xQueueSend(outputs2FB, &curr_tx_data, 1/portTICK_PERIOD_MS) != pdTRUE) {
                tx_control_data_t dummy_tx_data;
                xQueueReceive(outputs2FB, &dummy_tx_data, 1);
            }
            if(ulTaskNotifyTake (pdTRUE, portMAX_DELAY) == 2) { //Failed to update control
                water_error = true;
            }
            else {
                water_error = false;
            }
        }

        if(xQueueSend(outputs2sampler, &out_nmbr_outputs,1/portTICK_PERIOD_MS) != pdTRUE) {
            uint8_t dummy;
            xQueueReceive(outputs2sampler, &dummy, 1);
        }
        
        vTaskDelay(1);
    }
}