#ifndef GB_FIREBASE_H
#define GB_FIREBASE_H

#include "FirebaseESP32.h"
#include "data_types.h"
/*FireBase authentification values*/
#define FIREBASE_HOST "growbox-350914-default-rtdb.firebaseio.com" //Do not include https:// in FIREBASE_HOST nor the last slash /
#define FIREBASE_AUTH "ZpsPdhWPAb2BMlsWiP9YmdujC8LINDnrVNfSxWAP"
#define UID_FETCH_TO  15*1000  

bool gb_firebase_init( void );
bool fb_update_sensor(tx_sensor_data_t *sensor_data);
bool fb_update_control(tx_control_data_t *control_data);
bool fb_update_water();

#endif