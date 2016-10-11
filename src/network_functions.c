/*
 * network_functions.c
 *
 *  Created on: 11/10/2016
 *      Author: Mariano
 */

#include "network_functions.h"
#include "MQTTClient.h"
#include "mqtt_wifi_interface.h"
#include "tcp_transceiver.h"

#include "uart.h"
#include "lcd.h"
#include "main_menu.h"

//--- Externals ----------------------//
extern Client  c;
extern MQTT_vars mqtt_ibm_setup;
extern Network  n;
extern unsigned char MQTT_read_buf[SIZEOF_BUFFTCP_SEND];
extern unsigned char MQTT_write_buf[SIZEOF_BUFFTCP_SEND];
extern MQTTPacket_connectData options;
extern MQTTMessage  MQTT_msg;
extern const char s_blank_line [];

extern uint8_t json_buffer[SIZEOF_BUFFTCP_SEND];
extern void prepare_json_pkt (uint8_t *);

extern unsigned short mqtt_timer;

//--- Globals ------------------------//
mqtt_state_t mqtt_state;


//--- Function Definitions -----------//

void MQTTFunctionResetSM (void)
{
	mqtt_state = mqtt_init;
}


//---------- Prueba Conexiones Prueba MQTT_MEM_ONLY ---------//
unsigned char MQTTFunction (void)
{
	unsigned char resp = RESP_CONTINUE;
	mqtt_state = mqtt_init;
	int dummy_resp = 0;

	Client * pc;
	pc = &c;
	int rc = FAILURE;
	char topicName [80];
	unsigned char s_connack [] = {0x20, 0x02};		//Deserialize_connack no le da bola al length
	unsigned char connack_rc = 255;
	char sessionPresent = 0;


    switch (mqtt_state)
    {
		case mqtt_init:
			Usart2Send((char *) (const char *)"MQTT Memory Only Test...\r\n");

			MQTTtimer_init();
			Config_MQTT_Mosquitto ( &mqtt_ibm_setup);
			/* Initialize network interface for MQTT  */
			NewNetwork(&n);
			/* Initialize MQTT client structure */
			MQTTClient(&c,&n, 4000, MQTT_write_buf, sizeof(MQTT_write_buf), MQTT_read_buf, sizeof(MQTT_read_buf));

			mqtt_state++;
			break;

		case mqtt_sending_connect:
			options.MQTTVersion = 3;
			options.clientID.cstring = (char*)mqtt_ibm_setup.clientid;
			options.username.cstring = (char*)mqtt_ibm_setup.username;
			options.password.cstring = (char*)mqtt_ibm_setup.password;

			dummy_resp = MQTTSerialize_connect(pc->buf, pc->buf_size, &options);
			//if (MQTTConnect(&c, &options) < 0)
			if (dummy_resp <= 0)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "BRKR params error!!");
				mqtt_state = mqtt_connect_failed;
			}
			else
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "CONNECT     ");
				//resp = TCPSendDataSocket (dummy_resp, pc->buf);
				mqtt_state = mqtt_waiting_connack_load;
				mqtt_timer = 3000;
			}
			break;

		case mqtt_waiting_connack_load:

			RDMUtil_StringCopy(pc->readbuf, pc->readbuf_size, s_connack, sizeof(s_connack));
			mqtt_state = mqtt_waiting_connack;
			break;

		case mqtt_waiting_connack:
			//espero CONNACK o  TIMEOUT
			//				unsigned char connack_rc = 255;
			//				char sessionPresent = 0;
			connack_rc = 255;
			sessionPresent = 0;

			if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, pc->readbuf, pc->readbuf_size) == 1)
			{
				pc->isconnected = 1;
				mqtt_state = mqtt_connect;
			}
			else
				mqtt_state = mqtt_connect_failed;

			break;

		case mqtt_connect_failed:
			break;


		case mqtt_connect:

			if (!mqtt_timer)		//cuando agoto el timer, publico
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "PUB new data    ");
				LCD_2DO_RENGLON;
				LCDTransmitStr(s_blank_line);
				mqtt_timer = 5000;

				mqtt_state = mqtt_pub_prepare;
			}

			//reviso nuevos paquetes
			CheckForPubs (pc, 1000);

			break;

		case mqtt_pub_prepare:
			/* Prepare MQTT message */
			prepare_json_pkt(json_buffer);
			MQTT_msg.qos=QOS0;
			MQTT_msg.dup=0;
			MQTT_msg.retained=1;
			MQTT_msg.payload= (char *) json_buffer;
			MQTT_msg.payloadlen=strlen( (char *) json_buffer);
			strcpy(topicName, (const char *)"prueba");
			mqtt_state = mqtt_pub;
			break;

		case mqtt_pub:
			/* Publish MQTT message */
			rc = FAILURE;
			Timer timer;
			MQTTString topic = MQTTString_initializer;
			topic.cstring = (char *)topicName;
			int len = 0;

			InitTimer(&timer);
			countdown_ms(&timer, pc->command_timeout_ms);

			if (!pc->isconnected)
				mqtt_state = mqtt_pub_failed;

			if (MQTT_msg.qos == QOS1 || MQTT_msg.qos == QOS2)
				MQTT_msg.id = getNextPacketId(pc);

			len = MQTTSerialize_publish(pc->buf, pc->buf_size, 0, MQTT_msg.qos, MQTT_msg.retained, MQTT_msg.id,
					topic, (unsigned char*)MQTT_msg.payload, MQTT_msg.payloadlen);

			if (len <= 0)
				mqtt_state = mqtt_pub_failed;
			if ((rc = sendPacket(pc, len, &timer)) != OK) // send the subscribe packet
					mqtt_state = mqtt_pub_failed;

			if (MQTT_msg.qos == QOS1)
			{
				mqtt_state = mqtt_waiting_puback;
			}
			else if (MQTT_msg.qos == QOS2)
			{
				mqtt_state = mqtt_waiting_pubcomp;
			}

			//resultado de las funciones
			//				exit:
			//				    return rc;

			if (rc == FAILURE)
				mqtt_state = mqtt_pub_failed;

			break;

		case mqtt_pub_failed:
			LCD_1ER_RENGLON;
			LCDTransmitStr((const char *) "Failed to publish");
			mqtt_state = mqtt_connect;
			break;

		case mqtt_waiting_puback:
			if (waitfor(pc, PUBACK, &timer) == PUBACK)
			{
				unsigned short mypacketid;
				unsigned char dup, type;
				if (MQTTDeserialize_ack(&type, &dup, &mypacketid, pc->readbuf, pc->readbuf_size) != 1)
					rc = FAILURE;
			}
			else
				rc = FAILURE;
			break;

		case mqtt_waiting_pubcomp:
			if (waitfor(pc, PUBCOMP, &timer) == PUBCOMP)
			{
				unsigned short mypacketid;
				unsigned char dup, type;
				if (MQTTDeserialize_ack(&type, &dup, &mypacketid, pc->readbuf, pc->readbuf_size) != 1)
					rc = FAILURE;
			}
			else
				rc = FAILURE;
			break;

		default:
			mqtt_state = mqtt_init;
			break;
    }
    //Procesos continuos
//    ESP_ATProcess ();
//    TCPProcess();

    return resp;
    //---------- Fin Prueba MQTT_MEM_ONLY ---------//
}

