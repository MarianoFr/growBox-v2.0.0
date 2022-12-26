#include "rgb_rst_butt.h"

/***********************************
 * RTC alerts
 * input ERROR
 * out none
 ***********************************/
void RGBalert(uint8_t error)
{
    static uint8_t green = 0, blue = 0, red = 0;
    switch (error)
    {
    case NO_WIFI_CRED:
        green = 0;
        blue = 0;
        red = 255;
        Serial.println("NO_WIFI_CRED");
        break;
    case WIFI_CONN:
        green = 255;
        blue = 0;
        red = 0;
        Serial.println("WIFI_CONN");
        break;
    case WIFI_DISC:
        green = 0;
        blue = 255;
        red = 0;
        Serial.println("WIFI_DISC");
        break;
    case NO_USER:
        green = 50;
        blue = 127;
        red = 0;
        Serial.println("NO_USER");
        break;
    case DHT_ERR:
        green = 0;
        blue = 0;
        red = 0;
        Serial.println("DHT_ERR");
        break;
    case RTC_ERR:
        green = 0;
        blue = 0;
        red = 0;
        Serial.println("RTC_ERR");
        break;
    case SOIL_ERR:
        green = 0;
        blue = 0;
        red = 0;
        Serial.println("SOIL_ERR");
        break;
    case BH_1750_ERR:
        green = 0;
        blue = 0;
        red = 0;
        Serial.println("BH_1750_ERR");
        break;
    default:
        break;
    }
    analogWrite(PIN_GREEN, green);
    analogWrite(PIN_BLUE, blue);
    analogWrite(PIN_RED, red);
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
                Firebase.setString(firebaseData2, "users/" + mac + "/user/", "");
                delay(10);
                WiFi.disconnect();
                EEPROM.write(WIFI_CRED_FLAG, NO_CREDENTIALS); // wifi flag erased, credentials will be reset.
                EEPROM.commit();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                ESP.restart();
            }
        }
    }
    lastButtonState = reading;
}