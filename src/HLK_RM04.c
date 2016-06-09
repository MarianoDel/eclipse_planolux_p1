/*
 * HLK_RM04.c
 *
 *  Created on: 26/04/2016
 *      Author: Mariano
 */

#include "HLK_RM04.h"
#include "uart.h"
#include "main_menu.h"
#include "stm32f0xx.h"

#include <string.h>



//--- Externals -----------------------------------//
extern unsigned short hlk_timeout;
extern unsigned char hlk_mini_timeout;
extern unsigned char hlk_answer;

extern volatile unsigned char data1[];
extern volatile unsigned char data[];

#define data512		data1
#define data256		data


//--- Globals -------------------------------------//

void (* pCallBack) (char *);

unsigned char hlk_command_state = 0;
unsigned char hlk_timeout_cnt = 0;
unsigned char hlk_mode = UNKNOW_MODE;
volatile unsigned char * prx;
volatile unsigned char at_start = 0;
volatile unsigned char at_finish = 0;


unsigned char HLKToATMode (unsigned char p)
{
	unsigned char resp = RESP_CONTINUE;

	if (p == CMD_RESET)
	{
		hlk_command_state = COMM_INIT;
		return resp;
	}

	//hago polling hasta que este con modo AT
	switch (hlk_command_state)
	{
		case COMM_INIT:
			HLK_PIN_ON;
			hlk_timeout = TT_HLK_AT_MODE;
			hlk_timeout_cnt = 0;
			hlk_command_state = COMM_TO_AT;
			//hlk_mode = UNKNOW_MODE;
			hlk_mode = GOING_AT_MODE;
			break;

		case COMM_TO_AT:
			if (!hlk_timeout)
			{
				HLK_PIN_OFF;
				hlk_command_state = COMM_AT_ANSWER;
				SendCommandWithAnswer((const char *) "at+ver=?\r\n", CheckVersion);	//blanquea hlk_answer
				hlk_timeout = 3000;
			}
			break;

		case COMM_AT_ANSWER:
			if ((hlk_answer == RESP_TIMEOUT) || (!hlk_timeout))
			{
				if (hlk_timeout_cnt >= 3)
					resp = RESP_TIMEOUT;
				else
				{
					hlk_timeout_cnt++;
					hlk_command_state = COMM_TO_AT;
				}
			}

			if (hlk_answer == RESP_READY)
			{
				//hlk_command_state = COMM_WAIT_PARSER;
				HLKPreParser((unsigned char *)data256);

				resp = HLKVerifyVersion((unsigned char *)data256);

				if (resp == RESP_OK)
					hlk_mode = AT_MODE;
				else
					hlk_mode = UNKNOW_MODE;
			}
			break;

		default:
			hlk_command_state = COMM_INIT;
			break;
	}

	return resp;
}

unsigned char SendCommandWaitAnswer (const char * comm, unsigned char p)	//blanquea hlk_answer
{
	unsigned char i, length = 0;
	unsigned char resp = RESP_CONTINUE;
	char s_comm [80];

	if (p == CMD_RESET)
	{
		hlk_command_state = COMM_INIT;
		return resp;
	}

	switch (hlk_command_state)
	{
		case COMM_INIT:
			hlk_command_state = COMM_WAIT_ANSWER;
			SendCommandWithAnswer(comm, CheckVersion);	//blanquea hlk_answer
			hlk_timeout = TT_AT_1SEG;
			break;

		case COMM_WAIT_ANSWER:
			if ((hlk_answer == RESP_TIMEOUT) || (!hlk_timeout))
			{
				resp = RESP_TIMEOUT;
			}

			if (hlk_answer == RESP_READY)
				hlk_command_state = COMM_VERIFY_ANSWER;

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

			HLKPreParser((unsigned char *)data256);
			if (strncmp(s_comm, (char *)data256, length) == 0)
			{
				if ((*(data256 + length) == 'o') && (*(data256 + length + 1) == 'k'))
					resp = RESP_OK;
			}
			else
				resp = RESP_NOK;
			break;

		default:
			hlk_command_state = COMM_INIT;
			break;
	}

	return resp;
}

//Manda un comando al HLK y espera respuesta
//avisa de la respuesta al callback elegido con *pCall
//blanquea hlk_answer
//
void SendCommandWithAnswer(const char * str, void (*pCall) (char * answer))
{
	hlk_answer = RESP_NO_ANSWER;

	pCallBack = pCall;

	USARTSend((char *) str);
}

unsigned char HLK_Mode(void)
{
	return hlk_mode;	//TRANSPARENT_MODE, AT_MODE, UNKNOW_MODE
}


//me llaman desde el proceso principal para update
void HLK_ATProcess (void)
{
	//reviso timeouts para dar aviso del fin de mensajes at
	if ((at_start) && (at_finish) && (!hlk_mini_timeout))
	{
		at_start = 0;
		at_finish = 0;
		hlk_answer = RESP_READY;	//aviso que tengo una respuesta para revisar
	}
}

//me llaman desde usart rx si estoy en modo AT
void HLK_ATModeRx (unsigned char d)
{
	//tengo que ver en que parte del AT estoy
	if (!at_start)
	{
		if ((d == 'A') || (d == 'a'))
		{
			prx = (unsigned char *) data256;
			*prx = d;
			prx++;
			at_start = 1;
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
			hlk_answer = RESP_NOK;
		}
	}

	//mientras reciba bytes hago update del timer
	hlk_mini_timeout = TT_HLK_AT_MINI;
}

void HLK_TransparentModeRx (unsigned char d)
{

}

void HLKPreParser(unsigned char * d)
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

unsigned char HLKVerifyVersion(unsigned char * d)
{
	char comp;

	//primero reviso el echo del at
	comp = strncmp ((char *) d, (const char *) "at+ver=?", (sizeof ((const char *) "at+ver=?")) - 1);
	if (comp == 0)
	{
		//ahora reviso solo algunos valores
		if ((*(d+8) == 'V') && (*(d+10) == '.') && (*(d+13) == '('))
			return RESP_OK;
	}
	return RESP_NOK;
}

void CheckVersion (char * answer)
{
	unsigned char comp = 0;

	comp = strncmp (answer, (const char *) "VER 1.8", (sizeof ((const char *) "VER 1.8")) - 1);

	if (comp == 0)
		hlk_answer = RESP_OK;
	else
		hlk_answer = RESP_NOK;
}

