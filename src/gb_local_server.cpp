#include "gb_local_server.h"

/* Access Point to configure ESP32 ( WiFi Web Server )
    used to input customer's WiFi credentials
    and give ESP32 access to internet*/
const char* esp32_ssid     = "GrowBox_config";
const char* esp32_password = "green4you";
WebServer server(80);
IPAddress ap_local_IP(192, 168, 1, 1);
IPAddress ap_gateway(192, 168, 1, 254);
IPAddress ap_subnet(255, 255, 255, 0);
/* User's WiFi credentials */
char ssidc[30];//Stores the router name, they must be less than 30 characters in length
char passwordc[30];//Stores the password
/* Flag activated when WiFi credentials are beign asked by ESP32 */
bool gettingWiFiCredentials = false;
char users_uid[100] = "NULL";
//Creating the input form
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
  "<FORM action=\"/\" method=\"post\">"
  "<h1>Bienvenido a GrowBox</h1>"
  "<h3>El sistema de automatización de cultivos de interior.</h3>"
  "<ul>"
  "<li>Por favor, introduzca el nombre (SSID) y la contraseña de su red de WiFi luego presione aceptar</li>"
  "<li>Con esto su GrowBox ya estará conectado y podrá usarlo con la aplicación desde su celular.</li>"
  "<li>Si tiene problemas para iniciar su GrowBox, no dude en comunicarse con el centro de atencion al cliente.</li>"
  "<li>Puede contactarse por mail o por instagram</li>"
  "<li>growboxcultivo@gmail.com | IG: growbox1.0</li>"
  "</ul>"
  "<div>"
  "<label>ssid:&nbsp;</label>"
  "<input maxlength=\"30\" name=\"ssid\"><br>"
  "<label>contraseña:&nbsp;</label><input maxlength=\"30\" name=\"Password\"><br>"
  "<INPUT type=\"submit\" value=\"Aceptar\"> <INPUT type=\"Reset\">"
  "</div>"
  "<br>"
  "</FORM>"
  "</body>"
  "</html>";

/****************************************************/
// Dealing with the call to root on the WiFi server //
/****************************************************/
void handleRoot() {
  if ( server.hasArg("ssid") && server.hasArg("Password") ) { //( server.hasArg("ssid") && server.hasArg("Password") && server.hasArg("mail") )
    handleSubmit();
  }
  else {//Redisplay the form
    server.send(200, "text/html", INDEX_HTML);
  }
}

/*****************************************************************************************/
// Once the data's been sent by the user, we display the inputs and write them to memory //
/*****************************************************************************************/
void handleSubmit() { //dispaly values and write to memmory
  String response =
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
  response += server.arg("ssid");
  response += "<br>";
  response += "Y su contraseña es ";
  response += server.arg("Password");
  response += "</P><BR>";
  response += "<H2><a href=\"/\">go home</a></H2><br>";

  server.send(200, "text/html", response);
  //calling function that writes data to memory
  write_to_Memory(String(server.arg("ssid")), String(server.arg("Password")));
  ESP.restart();
}

/********************************************************************/
// Shows when we get a misformt or wrong request for the web server //
/********************************************************************/
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  message += "<H2><a href=\"/\">go home</a></H2><br>";
  server.send(404, "text/plain", message);
}

/************************************/
// Starts WiFi server to get user's //
// WiFi credentials.                //
/************************************/
void setupServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(esp32_ssid, esp32_password, 1);
  WiFi.softAPConfig(ap_local_IP, ap_gateway, ap_subnet);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
}

/******************************/
// Checks for WiFi credential //
/******************************/
bool checkWiFiCredentials() {
  if ( EEPROM.read( WIFI_CRED_FLAG ) ) {
    //reading the ssid and password out of memory
#if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println("WiFi credentials retrieved");
#endif
    String string_Ssid = "";
    String string_Password = "";
    string_Ssid = FLASH_read_string(30, SSID_STORING_ADD);
    string_Password = FLASH_read_string(30, PASS_STORING_ADD);
    string_Password.toCharArray(passwordc, 30);
    string_Ssid.toCharArray(ssidc, 30);
#if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println(ssidc);
    Serial.println(passwordc);
#endif
    return true;
  }
  else {
    #if WIFI_DEBUG && SERIAL_DEBUG
    Serial.println("No wifi credentials");
#endif
    return false;
  }
}

/**************************************************************/
// In charge of connecting to WiFi LAN and FireBase Data Base //
/**************************************************************/
bool connectWifi() {
  uint8_t t = 0;
  WiFi.mode(WIFI_STA);
  uint8_t res = 0;
  while(t<3)
  {
    WiFi.begin(ssidc, passwordc);
    res = WiFi.waitForConnectResult(10000);
    if(res == WL_CONNECTED)
      t=3;
    t++;
  }
  if(res != WL_CONNECTED)
  {
   rgb_state |= (1UL << WIFI_DISC);
   rgb_state &= ~(1UL << WIFI_CONN);
   return false;
  }
  rgb_state &= ~(1UL << WIFI_DISC);
  rgb_state |= 1UL << WIFI_CONN;
  return true;
}

void wiFiTasks( void * pvParameters ) {

  Firebase.beginMultiPathStream(firebaseData1, path + "/control", childPath, 12);

//  Firebase.setStreamTaskStackSize(20000);

  Firebase.setMultiPathStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);

  writeControl tx;

  for (;;) {

    /*Loop to obtain user's WiFi credentials via
      ESP32's WiFi server */
    while ( gettingWiFiCredentials )
    {
      server.handleClient();//Checks for web server activity
    }

    if ( (!gettingWiFiCredentials) && (WiFi.status() == WL_CONNECTED) ) 
    {
      /*As long as we have WiFi and not getting WiFi cred through ESP32 server*/
      /*Perform all tasks requiring internet*/
      static bool stopWater2 = false;
      bool state1 = xQueueReceive(waterQueue, &stopWater2, 0);
      rgb_state &= ~(1UL << WIFI_DISC);
      rgb_state |= 1UL << WIFI_CONN;
      if (state1)
      {
      #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Stop water!!!!");
      #endif
        Firebase.setBool(firebaseData2, path + "/control/Water", false);
      }
      bool state2 = xQueueReceive(writeQueue, &tx, 0);
      if(state2)
      {
      #if SERIAL_DEBUG && FIRE_DEBUG
        Serial.println("Updating dashboard!!");
      #endif
        FirebaseJson dashBoard;
        write2FireBase( &tx, &dashBoard );
        Firebase.updateNode(firebaseData2, path + "/dashboard/", dashBoard);
        if(!((rgb_state >> NO_USER) & 1U))
        {
          Firebase.setInt(firebaseData2, path + "/control/RGBState", rgb_state);
        }    
      }
    }

    if (WiFi.status() != WL_CONNECTED && (!gettingWiFiCredentials))
    {
      if(xSemaphoreTake(xDhtWiFiSemaphore, portMAX_DELAY)==pdTRUE)
      {
        rgb_state |= 1UL << WIFI_DISC;
        connectWifi();
      }
    }
    yield();
  }
}