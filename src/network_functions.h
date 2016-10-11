/*
 * network_functions.h
 *
 *  Created on: 11/10/2016
 *      Author: Mariano
 */

#ifndef NETWORK_FUNCTIONS_H_
#define NETWORK_FUNCTIONS_H_


//--- MQTT States Functions ---//

typedef enum {
	mqtt_init = 0,
	mqtt_sending_connect,
	mqtt_waiting_connack_load,
	mqtt_waiting_connack,
	mqtt_connect_failed,
	mqtt_connect,
	mqtt_pub_prepare,
	mqtt_pub,
	mqtt_pub_failed,
	mqtt_waiting_pubcomp,
	mqtt_waiting_puback,
	mqtt_sub,

	mqtt_device_control,
	wifi_undefine_state       = 0xFF,
} mqtt_state_t;


void MQTTFunctionResetSM (void);
unsigned char MQTTFunction (void);

#endif /* NETWORK_FUNCTIONS_H_ */
