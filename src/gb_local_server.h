#ifndef GB_LOCAL_SERVER_H
#define GB_LOCAL_SERVER_H

#include <Arduino.h>
#include "WebServer.h"
#include "write_mem.h"
#include "rgb_rst_butt.h"
#include "globals.h"

/*****************************************************************************************/
// Once the data's been sent by the user, we display the inputs and write them to memory //
/*****************************************************************************************/
void handleSubmit();
/****************************************************/
// Dealing with the call to root on the WiFi server //
/****************************************************/
void handleRoot();
/********************************************************************/
// Shows when we get a misformt or wrong request for the web server //
/********************************************************************/
void handleNotFound();
/************************************/
// Starts WiFi server to get user's //
// WiFi credentials.                //
/************************************/
void setupServer();
/******************************/
// Checks for WiFi credential //
/******************************/
bool checkWiFiCredentials();
/**************************************************************/
// In charge of connecting to WiFi LAN and FireBase Data Base //
/**************************************************************/
bool connectWifi();
#endif