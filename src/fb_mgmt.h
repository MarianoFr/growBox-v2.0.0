#ifndef FB_MGMT_H
#define FB_MGMT_H
#include "FirebaseESP32.h"
#include "debug_flags.h"
#include "globals.h"
#include <Arduino.h>

/*FireBase authentification values*/
#define FIREBASE_HOST "growbox-350914-default-rtdb.firebaseio.com" //Do not include https:// in FIREBASE_HOST nor the last slash /
#define FIREBASE_AUTH "ZpsPdhWPAb2BMlsWiP9YmdujC8LINDnrVNfSxWAP"

extern String childPath[];
/*******************************
 Update variables from FireBase
*******************************/
void streamCallback(MultiPathStreamData data);
//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout);
/*******************************
  Update variables in FireBase
*******************************/
void write2FireBase (struct writeControl *tx, FirebaseJson *dashBoard);

#endif