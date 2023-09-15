#include "rgb_rst_butt.h"

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
                vTaskDelay(pdMS_TO_TICKS(5000));
                WiFi.disconnect();
                EEPROM.write(WIFI_CRED_FLAG, NO_CREDENTIALS); // wifi flag erased, credentials will be reset.
                EEPROM.commit();
                ESP.restart();
            }
        }
    }
    lastButtonState = reading;
}