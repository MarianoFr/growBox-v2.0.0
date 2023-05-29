#include "rgb_rst_butt.h"

/***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
void RGBalert(void)
{
    static uint8_t green = 0, blue = 0, red = 0;
    if(rgb_state & WIFI_DISC)
    {
        green = 0;
        blue = 0;
        red = 0;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("WIFI_DISC");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & WIFI_CONN)
    {
        green = 255;
        blue = 0;
        red = 0;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("WIFI_CONN");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & NO_WIFI_CRED)
    {
        green = 0;
        blue = 255;
        red = 0;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("NO_WIFI_CRED");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & NO_USER)
    {
        green = 0;
        blue = 0;
        red = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("NO_USER");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & DHT_ERR)
    {
        green = 255;
        blue = 255;
        red = 0;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("DHT_ERR");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & RTC_ERR)
    {
        green = 0;
        blue = 255;
        red = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("RTC_ERR");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & SOIL_ERR)
    {
        green = 255;
        blue = 0;
        red = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("SOIL_ERR");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if(rgb_state & BH_1750_ERR)
    {
        green = 255;
        blue = 255;
        red = 255;
    #if SERIAL_DEBUG && RGB_DEBUG
        Serial.println("BH_1750_ERR");
    #endif
        analogWrite(PIN_GREEN, green);
        analogWrite(PIN_BLUE, blue);
        analogWrite(PIN_RED, red);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    return;
}

/******************************************/
// Debounce routine for WiFi reset button //
/******************************************/
void debounceWiFiReset(void)
{
    /*Switch debounce variables for WiFi reset*/
    static bool buttonState;
    static bool lastButtonState = LOW;
    int reading = digitalRead(WIFI_RESET);
    static unsigned long lastDebounceTime = 0;
    int debounceDelay = 50;
    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;
            if (buttonState == HIGH)
            {
                char aux_1[100];
                char mac_char[100];
                char user_id[100];
                mac.toCharArray(mac_char, mac.length()+1);
                sprintf(aux_1,"users/%s/user/", mac_char);
                Firebase.getString(firebaseData2, aux_1, user_id);
                if(Firebase.setString(firebaseData2, aux_1, "NULL"))
                {
                    sprintf(aux_1,"growboxs/%s/", user_id);
                    Firebase.deleteNode(firebaseData2, aux_1);
                }
                    
                vTaskDelay(pdMS_TO_TICKS(2000));
                WiFi.disconnect();
                EEPROM.write(WIFI_CRED_FLAG, NO_CREDENTIALS); // wifi flag erased, credentials will be reset.
                EEPROM.commit();
                ESP.restart();
            }
        }
    }
    lastButtonState = reading;
}