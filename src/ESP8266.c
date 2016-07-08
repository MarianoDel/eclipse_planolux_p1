/*
 * ESP8266.c
 *
 *  Created on: 26/04/2016
 *      Author: Mariano
 */

#include "ESP8266.h"
#include "uart.h"
#include "main_menu.h"
#include "stm32f0xx.h"

#include <string.h>
#include <stdio.h>



//--- Externals -----------------------------------//
extern unsigned short esp_timeout;
extern unsigned char esp_mini_timeout;
extern unsigned char esp_answer;
extern unsigned char esp_unsolicited_pckt;

extern volatile unsigned char data1[];
extern volatile unsigned char data[];
extern volatile unsigned char bufftcp[];

#define data512		data1
#define data256		data


//--- Globals -------------------------------------//

void (* pCallBack) (char *);

unsigned char esp_command_state = 0;
unsigned char esp_timeout_cnt = 0;
unsigned char esp_mode = UNKNOW_MODE;
volatile unsigned char * prx;
volatile unsigned char at_start = 0;
volatile unsigned char at_finish = 0;
volatile unsigned char pckt_start = 0;
volatile unsigned char pckt_finish = 0;
enum EspConfigState esp_config_state = CONF_INIT;

//OJO OJO OJO no esta terminada y no se si se necesita
unsigned char ESP_EnableNewConn (unsigned char p)
{
	unsigned char resp = RESP_CONTINUE;

	if (p == CMD_RESET)
	{
		esp_config_state = ENA_INIT;
		return resp;
	}

	switch (esp_config_state)
	{
		case ENA_INIT:
			esp_config_state++;
			SendCommandWaitAnswerResetSM();
			break;

		case ENA_ASK_AT:
			resp = SendCommandWaitAnswer((const char *) "at+reconn=1\r\n");
			break;

		default:
			esp_config_state = ENA_INIT;
			break;
	}
	return resp;
}

//con CMD_RESET hace reset de la maquina
//con CMD_PROC busca primero ir a AT Mode y luego va a transparente
//con CMD_ONLY_CHECK revisa si esta en AT y pasa a transparente o constesta que ya esta en transparente

unsigned char ESP_SendConfig (unsigned char p)
{
	unsigned char resp = RESP_CONTINUE;

	if (p == CMD_RESET)
	{
		esp_config_state = CONF_INIT;
		return resp;
	}

	switch (esp_config_state)
	{
		case CONF_INIT:
			esp_config_state++;
			ESPToATMode(CMD_RESET);
			break;

		case CONF_ASK_AT:
			resp = ESPToATMode(CMD_PROC);

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_0;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_0:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_0B;
			break;

		case CONF_AT_CONFIG_0B:
			resp = SendCommandWaitAnswer((const char *) "AT+CWMODE_CUR=2\r\n");

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_1;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_1:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_1B;
			break;

		case CONF_AT_CONFIG_1B:
			resp = SendCommandWaitAnswer((const char *) "AT+CWSAP_CUR=\"KIRNO_WIFI\",\"12345678\",5,3\r\n");

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_2;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_2:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_2B;
			break;

		case CONF_AT_CONFIG_2B:
			resp = SendCommandWaitAnswer((const char *) "AT+CWDHCP_CUR=0,1\r\n");

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_3;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_3:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_3B;
			break;

		case CONF_AT_CONFIG_3B:
			resp = SendCommandWaitAnswer((const char *) "AT+CIPAP_CUR=\"192.168.1.254\"\r\n");

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_4;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_4:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_4B;
			break;

		case CONF_AT_CONFIG_4B:
			resp = SendCommandWaitAnswer((const char *) "AT+CIPMUX=1\r\n");

			if (resp == RESP_OK)
			{
				esp_config_state = CONF_AT_CONFIG_5;
				resp = RESP_CONTINUE;
			}
			break;

		case CONF_AT_CONFIG_5:
			SendCommandWaitAnswerResetSM();
			esp_config_state = CONF_AT_CONFIG_5B;
			break;

		case CONF_AT_CONFIG_5B:
			resp = SendCommandWaitAnswer((const char *) "AT+CIPSERVER=1,10002\r\n");
			//utilizo esta respuesta como salida de la funcion

			break;

		default:
			esp_config_state = CONF_INIT;
			break;
	}

	return resp;
}

void ESP_SendDataResetSM (void)
{
	esp_config_state = SEND_DATA_INIT;
}

//resetar primero la maquina de estados con ESP_SendDataResetSM()
//contesta RESP_CONTINUE, RESP_TIMEOUT, RESP_NOK o RESP_OK
//el bufftcpsend de transmision es port,lenght,data
unsigned char ESP_SendData (unsigned char port, unsigned char * pbuf)
{
	unsigned char resp = RESP_CONTINUE;
	char a [30];
	unsigned char i;
	char dummy = 0;

	switch (esp_config_state)
	{
		case SEND_DATA_INIT:
			esp_config_state++;
			ESP_SetMode(AT_MODE);		//TODO: ver si todo no puede ser AT
			break;

		case SEND_DATA_RST:
			SendCommandWaitAnswerResetSM();
			esp_config_state++;
			break;

		case SEND_DATA_ASK_CHANNEL:
			sprintf (a, "AT+CIPSEND=%i,%i\r\n", *pbuf, *(pbuf+1));
			resp = SendCommandWaitAnswer(a);

			if (resp == RESP_OK)
			{
				resp = RESP_CONTINUE;
				SendCommandWithAnswer((pbuf + 2));		//blanquea esp_answer
				esp_timeout = TT_AT_3SEG;
				esp_config_state++;
			}
			break;

		case SEND_DATA_WAIT_SEND_OK:
			//me quedo esperando el ok de envio o timeout
			if (esp_answer == RESP_READY)
			{
				//reviso si tengo SEND OK en el buffer
				ESPPreParser((unsigned char *)data256);

				//si me recibe los bytes doy como el paquete enviado
				if (strncmp((char *) (const char *) "Recv ", (char *)data256, (sizeof ((const char *) "Recv ")) - 1) == 0)
					resp = RESP_OK;
				else if (strncmp((char *) (const char *) "SEND OK", (char *)data256, (sizeof ((const char *) "SEND OK")) - 1) == 0)
					resp = RESP_OK;
				else
					resp = RESP_NOK;

//				for (i = 0; i < 249; i++)
//				{
//					if (data256[i] != '\0')
//					{
//						if ((data256[i] == 'S') &&
//								(data256[i+1] == 'E') &&
//								(data256[i+2] == 'N') &&
//								(data256[i+3] == 'D') &&
//								(data256[i+4] == ' ') &&
//								(data256[i+5] == 'O') &&
//								(data256[i+6] == 'K'))
//
//						{
//							i = 250;
//							resp = RESP_OK;
//						}
//					}
//					else
//					{
//						i = 250;
//					}
//				}
//				if (resp != RESP_OK)
//					resp = RESP_NOK;
			}

			if (!esp_timeout)
			{
				//tengo timeout, termino transmision
				resp = RESP_TIMEOUT;
			}
			break;

		default:
			esp_config_state = SEND_DATA_INIT;
			break;
	}

	return resp;
}

//con CMD_RESET hace reset de la maquina, con CMD_PROC recorre la rutina
//contesta RESP_CONTINUE, RESP_TIMEOUT, RESP_NOK o RESP_OK
unsigned char ESPToATMode (unsigned char p)
{
	unsigned char resp = RESP_CONTINUE;

	if (p == CMD_RESET)
	{
		esp_command_state = COMM_INIT;
		return resp;
	}

	//hago polling hasta que este con modo AT
	switch (esp_command_state)
	{
		case COMM_INIT:
			esp_timeout = TT_ESP_AT_MODE;
			esp_timeout_cnt = 0;
			esp_command_state = COMM_TO_AT;
			esp_mode = GOING_AT_MODE;
			break;

		case COMM_TO_AT:
			if (!esp_timeout)
			{
				esp_command_state = COMM_AT_ANSWER;
				SendCommandWithAnswer((const char *) "AT+GMR\r\n");	//blanquea esp_answer
				esp_timeout = 3000;
			}
			break;

		case COMM_AT_ANSWER:
			if ((esp_answer == RESP_TIMEOUT) || (!esp_timeout))
			{
				if (esp_timeout_cnt >= 3)
					resp = RESP_TIMEOUT;
				else
				{
					esp_timeout_cnt++;
					esp_command_state = COMM_TO_AT;
				}
			}

			if (esp_answer == RESP_READY)
			{
				//esp_command_state = COMM_WAIT_PARSER;
				ESPPreParser((unsigned char *)data256);

				resp = ESPVerifyVersion((unsigned char *)data256);

				if (resp == RESP_OK)
					esp_mode = AT_MODE;
				else
					esp_mode = UNKNOW_MODE;
			}
			break;

		default:
			esp_command_state = COMM_INIT;
			break;
	}

	return resp;
}

void SendCommandWaitAnswerResetSM (void)
{
	esp_command_state = COMM_INIT;
}

unsigned char SendCommandWaitAnswer (const char * comm)	//blanquea esp_answer
{
	unsigned char i, length = 0;
	unsigned char resp = RESP_CONTINUE;
	char s_comm [80];

	switch (esp_command_state)
	{
		case COMM_INIT:
			esp_command_state = COMM_WAIT_ANSWER;
			SendCommandWithAnswer(comm);	//blanquea esp_answer
			esp_timeout = TT_AT_1SEG;
			break;

		case COMM_WAIT_ANSWER:
			if ((esp_answer == RESP_TIMEOUT) || (!esp_timeout))
			{
				resp = RESP_TIMEOUT;
			}

			if (esp_answer == RESP_READY)
				esp_command_state = COMM_VERIFY_ANSWER;

			break;

		case COMM_VERIFY_ANSWER:
			for (i = 0; i < sizeof(s_comm); i++)	//copio hasta el primer \r
			{
				if (comm[i] != '\r')
					s_comm[i] = comm[i];
				else
				{
					length = i;
					s_comm[i] = '\0';
					i = sizeof(s_comm);
				}
			}

			ESPPreParser((unsigned char *)data256);
			if (strncmp(s_comm, (char *)data256, length) == 0)
			{
				if ((*(data256 + length) == 'O') && (*(data256 + length + 1) == 'K'))
					resp = RESP_OK;

				if (strncmp((char *) (const char *) "no change OK", (data256 + length), (sizeof ((const char *) "no change OK")) - 1) == 0)
					resp = RESP_OK;

				if (*(data256 + length) == '>')
					resp = RESP_OK;
			}
			else
				resp = RESP_NOK;
			break;

		default:
			esp_command_state = COMM_INIT;
			break;
	}

	return resp;
}

//Manda un comando al ESP sin esperar la respuesta
//blanquea esp_answer
void SendCommandWithAnswer(const char * str)
{
	esp_answer = RESP_NO_ANSWER;
	USARTSend((char *) str);
}

unsigned char ESP_AskMode(void)
{
	return esp_mode;	//TRANSPARENT_MODE, AT_MODE, UNKNOW_MODE
}

void ESP_SetMode(unsigned char m)
{
	esp_mode = m;	//TRANSPARENT_MODE, AT_MODE, UNKNOW_MODE
}

//me llaman desde el proceso principal para update
void ESP_ATProcess (void)
{
	//reviso timeouts para dar aviso del fin de mensajes at
	if ((at_start) && (at_finish) && (!esp_mini_timeout))
	{
		at_start = 0;
		at_finish = 0;
		esp_answer = RESP_READY;	//aviso que tengo una respuesta para revisar
	}

	if ((pckt_start) && (pckt_finish) && (!esp_mini_timeout))
	{
		pckt_start = 0;
		pckt_finish = 0;
		esp_unsolicited_pckt = RESP_READY;	//aviso que tengo una respuesta para revisar
	}
}

//TODO: ver que pasa si llega AT y nada mas como salgo por timeout EN ESTA O LA DE ARRIBA
//me llaman desde usart rx si estoy en modo AT
void ESP_ATModeRx (unsigned char d)
{
	//tengo que ver en que parte del AT estoy
	if ((!at_start) && (!pckt_start))
	{
		if ((d == 'A') || (d == 'R') || (d == 'S'))		//AT o Recv o SEND OK
		{
			prx = (unsigned char *) data256;
			*prx = d;
			prx++;
			at_start = 1;
		}
		else if (((d >= '0') && (d <= '4'))	|| (d == '+'))	//pueden ser nuevas conexiones o paquete TCP
		{
			prx = (unsigned char *) bufftcp;
			*prx = d;
			prx++;
			pckt_start = 1;
		}
	}
	else if (at_start)
	{
		if (d == '\n')		//no se cuantos finales de linea voy a tener en la misma respuesta
			at_finish = 1;

		*prx = d;
		if (prx < &data256[SIZEOF_DATA256])
			prx++;
		else
		{
			//recibi demasiados bytes juntos
			//salgo por error
			prx = (unsigned char *) data256;
			esp_answer = RESP_NOK;
		}
	}
	else if (pckt_start)
	{
		if (d == '\n')		//no se cuantos finales de linea voy a tener en la misma respuesta
			pckt_finish = 1;

		*prx = d;
		if (prx < &bufftcp[SIZEOF_BUFFTCP])
			prx++;
		else
		{
			//recibi demasiados bytes juntos
			//salgo por error
			prx = (unsigned char *) bufftcp;
			esp_unsolicited_pckt = RESP_NOK;
		}
	}

	//mientras reciba bytes hago update del timer
	esp_mini_timeout = TT_ESP_AT_MINI;
}

////me llaman desde usart rx si estoy en modo AT para transmitir
//void ESP_ATModeTx (unsigned char d)
//{
//	//tengo que ver en que parte del AT estoy o si esta listo a enviar
//	if (d == '>')
//	{
//		pckttx_start = 0;
//		pckttx_finish = 0;
//		esp_answer = RESP_READY;	//aviso que tengo una respuesta para revisar
//	}
//	else if (!pckttx_start)
//	{
//		if (d == 'R')				//cantidad de bytes recibidos
//		{
//			prx = (unsigned char *) bufftcp;
//			*prx = d;
//			prx++;
//			pckttx_start = 1;
//		}
//	}
//	else if (pckttx_start)
//	{
//		if (d == '\n')		//no se cuantos finales de linea voy a tener en la misma respuesta
//			pckttx_finish = 1;
//
//		*prx = d;
//		if (prx < &bufftcp[SIZEOF_BUFFTCP])
//			prx++;
//		else
//		{
//			//recibi demasiados bytes juntos
//			//salgo por error
//			prx = (unsigned char *) bufftcp;
//			esp_answer = RESP_NOK;
//		}
//	}
//
//	//mientras reciba bytes hago update del timer
//	esp_mini_timeout = TT_ESP_AT_MINI;
//}


void ESPPreParser(unsigned char * d)
{
	unsigned char i;
	unsigned char * l;

	l = d;
	for (i = 0; i < SIZEOF_DATA256; i++)
	{
		if (*d != '\0')
		{
			if ((*d > 31) && (*d < 127))		//todos los codigos numeros y letras
			{
				*l = *d;
				l++;
			}
			d++;
		}
		else
		{
			*l = '\0';
			break;
		}
	}
}

unsigned char ESPVerifyVersion(unsigned char * d)
{
	char comp;

	//primero reviso el echo del at
	comp = strncmp ((char *) d, (const char *) "AT+GMR", (sizeof ((const char *) "AT+GMR")) - 1);
	if (comp == 0)
	{
		//ahora reviso solo algunos valores
		if ((*(d+6) == 'A') && (*(d+9) == 'v') && (*(d+16) == ':'))
			return RESP_OK;
	}
	return RESP_NOK;
}

void CheckVersion (char * answer)
{
	unsigned char comp = 0;

	comp = strncmp (answer, (const char *) "VER 1.8", (sizeof ((const char *) "VER 1.8")) - 1);

	if (comp == 0)
		esp_answer = RESP_OK;
	else
		esp_answer = RESP_NOK;
}

