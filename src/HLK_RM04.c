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
			hlk_mode = UNKNOW_MODE;
			break;

		case COMM_TO_AT:
			if (!hlk_timeout)
			{
				HLK_PIN_OFF;
				hlk_command_state = COMM_AT_ANSWER;
				SendCommandWithAnswer((const char *) "at+ver=?", CheckVersion);	//blanquea hlk_answer
			}
			break;

		case COMM_AT_ANSWER:
			if (hlk_answer == RESP_TIMEOUT)
			{
				if (hlk_timeout_cnt >= 3)
					resp = RESP_TIMEOUT;
				else
				{
					hlk_timeout_cnt++;
					hlk_command_state = COMM_TO_AT;
				}
			}

			if (hlk_answer == RESP_OK)
			{
				hlk_mode = AT_MODE;
				resp = RESP_OK;
			}
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
	USARTSend((char *) (const char *) "END");
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
		hlk_answer = RESP_OK;
	}
}

//me llaman desde usart rx si estoy en modo AT
void HLK_ATModeRx (unsigned char d)
{
	//tengo que ver en que parte del AT estoy
	if (!at_start)
	{
		if (d == 'A')
		{
			prx = (unsigned char *) data256;
			*prx = d;
			prx++;
			at_start = 1;
			hlk_mini_timeout = TT_HLK_AT_MINI;
		}
	}
	else if (at_start)
	{
		if (d == '\n')
			at_finish = 1;

		*prx = d;
		if (prx < &data256[SIZEOF_DATA256])
			prx++;
		else
		{
			//salgo por error
			hlk_answer = RESP_NOK;
		}
	}
}

void HLK_TransparentModeRx (unsigned char d)
{

}

