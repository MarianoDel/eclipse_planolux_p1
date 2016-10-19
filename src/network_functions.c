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
#include "ESP8266.h"
#include "rdm_util.h"

#include "uart.h"
#include "lcd.h"
#include "main_menu.h"
#include "stm32f0xx.h"

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

extern volatile unsigned short mqtt_func_timer;
extern volatile unsigned short wifi_func_timer;

extern unsigned char esp_mini_timeout;
extern volatile unsigned char rx2buff[];
extern unsigned char bufftcprecv [MAX_BUFF_INDEX] [SIZEOF_BUFFTCP_SEND];
extern unsigned char bport_index_receiv [MAX_BUFF_INDEX];

//--- Globals ------------------------//
mqtt_state_t mqtt_state;
wifi_state_t wifi_state;


//--- Function Definitions -----------//

void MQTTFunctionResetSM (void)
{
	mqtt_state = mqtt_init;
}


//---------- Prueba Conexiones Prueba MQTT_MEM_ONLY ---------//
unsigned char MQTTFunction (void)
{
	unsigned char resp = RESP_CONTINUE;
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
#ifdef MQTT_MEM_ONLY
			Usart2Send((char *) (const char *)"MQTT Memory Only Test...\r\n");
#endif
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

#ifdef MQTT_MEM_ONLY
				mqtt_state = mqtt_waiting_connack_load;
#else
				mqtt_state = mqtt_waiting_connack;
				resp = TCPSendDataSocket (dummy_resp, pc->buf);
#endif
				mqtt_func_timer = 10000;	//espero 30 segundos para ver conexion
			}
			break;

#ifdef MQTT_MEM_ONLY
		case mqtt_waiting_connack_load:
			RDMUtil_StringCopy(pc->readbuf, pc->readbuf_size, s_connack, sizeof(s_connack));
			mqtt_state = mqtt_waiting_connack;
			break;
#endif

		case mqtt_waiting_connack:
			//espero CONNACK o  TIMEOUT
			//				unsigned char connack_rc = 255;
			//				char sessionPresent = 0;

			//reviso que no se este transmitiendo nada, ni recibiendo bytes
			if ((CheckTxEmptyBuffer() == 0) && (!esp_mini_timeout))
			{
				unsigned char bindex, lookplus, rxlen = 0;

				//parseo el rx2buff buscando respuestas
				bindex = FirstRxEmptyBuffer();

				if (bindex < MAX_BUFF_INDEX)	//tengo lugar
				{
					//busco en el buffer un +IPD
					for (lookplus = 0; lookplus < (SIZEOF_BUFFTCP_SEND - 1); lookplus++)
					{
						if ((rx2buff[lookplus] == '+') && (rx2buff[lookplus + 1] == 'I'))
						{
							if (TCPReadDataSocket((unsigned char *)&rx2buff[lookplus], &bufftcprecv[bindex] [0], &rxlen) != 0xFF)
							{
								if (rxlen > 0)
									bport_index_receiv[bindex] = rxlen;
							}

							//ya lo procese, lo marco
							rx2buff[lookplus] = '-';
							lookplus = SIZEOF_BUFFTCP_SEND;
						}
					}
				}
			}

			if (ReadSocket(pc->readbuf, pc->readbuf_size) != 0)
			{
				connack_rc = 255;
				sessionPresent = 0;

				if (MQTTDeserialize_connack((unsigned char*)&sessionPresent, &connack_rc, pc->readbuf, pc->readbuf_size) == 1)
				{
					pc->isconnected = 1;
					mqtt_state = mqtt_connect;
					mqtt_func_timer = 2;		//publico al toque
				}
				else
					mqtt_state = mqtt_connect_failed;
			}

			if (!mqtt_func_timer)
			{
				mqtt_state = mqtt_connect_failed;
			}

			break;

		case mqtt_connect_failed:
			resp = RESP_NOK;
			break;


		case mqtt_connect:

			if (!mqtt_func_timer)		//cuando agoto el timer, publico
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "PUB new data    ");
				LCD_2DO_RENGLON;
				LCDTransmitStr(s_blank_line);
				mqtt_func_timer = 2000;

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
			else
			{
				mqtt_state = mqtt_connect;
				mqtt_func_timer = 2000;
			}
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


void WIFIFunctionResetSM (void)
{
	wifi_state = mqtt_init;
}


unsigned char WIFIFunction (void)
{
	unsigned char resp = RESP_CONTINUE;
	char s_lcd [20];
//	int dummy_resp = 0;

//	Client * pc;
//	pc = &c;

    switch (wifi_state)
    {
		case wifi_state_reset:
			//USARTSend("ESP8266 Test...\r\n");
			Usart2Send("ESP8266 Test...\r\n");
			WRST_OFF;
			Wait_ms(2);
			WRST_ON;

			LCD_1ER_RENGLON;
			LCDTransmitStr((const char *) "Reseting WiFi...");

			TCPProcessInit ();
			wifi_func_timer = 5000;	//espero 5 seg despues del reset
			wifi_state++;
			break;

		case wifi_state_ready:
			if (!wifi_func_timer)
			{
				wifi_state++;

				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Send to ESP Conf");
				ESP_SendConfigResetSM ();
			}
			break;

		case wifi_state_sending_conf:
			resp = ESP_SendConfigClient ();

			if ((resp == RESP_TIMEOUT) || (resp == RESP_NOK))
			{
				LCD_2DO_RENGLON;
				if (resp == RESP_TIMEOUT)
					LCDTransmitStr((const char *) "ESP: Timeout    ");
				else
					LCDTransmitStr((const char *) "ESP: Error      ");
				wifi_state = wifi_state_error;
				wifi_func_timer = 20000;	//20 segundos de error
			}

			if (resp == RESP_OK)
			{
				LCD_2DO_RENGLON;
				LCDTransmitStr((const char *) "ESP: Configured ");
				wifi_func_timer = 1000;
				wifi_state = wifi_state_wait_ip;
			}
			break;

		case wifi_state_wait_ip:
			if (!wifi_func_timer)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Getting DHCP IP ");
				LCD_2DO_RENGLON;
				LCDTransmitStr(s_blank_line);
				wifi_func_timer = 5000;

				wifi_state = wifi_state_wait_ip1;
			}
			break;

		case wifi_state_wait_ip1:
			resp = ESP_GetIP (s_lcd);

			if ((resp == RESP_TIMEOUT) || (resp == RESP_NOK))
			{
				LCD_2DO_RENGLON;
				if (resp == RESP_TIMEOUT)
					LCDTransmitStr((const char *) "ESP: Timeout    ");
				else
					LCDTransmitStr((const char *) "ESP: Err no IP  ");
				wifi_state = wifi_state_error;
				wifi_func_timer = 20000;	//20 segundos de error
			}

			if (resp == RESP_OK)
			{
				if (IpIsValid(s_lcd) == RESP_OK)
				{
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "IP valid on:    ");
					wifi_func_timer = 1000;
					wifi_state = wifi_state_idle;
				}
				else
				{
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "IP is not valid!!");
					wifi_state = wifi_state_error;
					wifi_func_timer = 20000;	//20 segundos de error
				}
				LCD_2DO_RENGLON;
				LCDTransmitStr(s_lcd);
			}
			break;

		case wifi_state_idle:
//			//estoy conectado al wifi
//			Config_MQTT_Mosquitto ( &mqtt_ibm_setup);
//			/* Initialize network interface for MQTT  */
//			NewNetwork(&n);
//			/* Initialize MQTT client structure */
//			MQTTClient(&c,&n, 4000, MQTT_write_buf, sizeof(MQTT_write_buf), MQTT_read_buf, sizeof(MQTT_read_buf));

			if (!wifi_func_timer)
			{
				ESP_OpenSocketResetSM();
				wifi_state = wifi_state_connecting;
			}
			break;

		case wifi_state_connecting:
			resp = ESP_OpenSocket();

			if (resp == RESP_OK)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Socket Open     ");
				wifi_state = wifi_state_connected;
				//wifi_func_timer = 2000;

				MQTTFunctionResetSM();
			}

			if (resp == RESP_NOK)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Cant open a socket");
				wifi_state = wifi_state_idle;
				wifi_func_timer = 500;			//espero 500 msegs antes de intenta reconectar
			}
			break;

		case wifi_state_connected:
			resp = MQTTFunction();

			if (resp == RESP_NOK)
			{
				ESP_CloseSocketResetSM();
				wifi_state = wifi_state_disconnected;
			}

//			if (!wifi_func_timer)
//			{
//				ESP_CloseSocketResetSM();
//				wifi_state = wifi_state_disconnected;
//			}
			break;

		case wifi_state_disconnected:
			resp = ESP_CloseSocket();

			if (resp == RESP_OK)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Socket Close    ");
				wifi_state = wifi_state_idle;
				wifi_func_timer = 10000;

			}

			if (resp == RESP_NOK)
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "Cant close socket");
				wifi_state = wifi_state_idle;
				wifi_func_timer = 10000;
			}
			break;

		case wifi_state_error:
			if (!wifi_func_timer)
				wifi_state = wifi_state_reset;
			break;

		default:
			wifi_state = wifi_state_reset;
			break;
   	}
}
