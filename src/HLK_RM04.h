/*
 * HLK_RM04.h
 *
 *  Created on: 26/04/2016
 *      Author: Mariano
 */

#ifndef HLK_RM04_H_
#define HLK_RM04_H_

#include "hard.h"

#define HLK_RM04_PRESENT

//--- Timeouts para el HLK -----------------------------------//
#define TT_HLK_AT_MODE		300
#define TT_HLK_RESET		6000
#define TT_HLK_AT_MINI		5		//timeout de esopera de chars luego de una comienzo

//--- Estados para el HLK -----------------------------------//
#define COMM_INIT		0
#define COMM_TO_AT		1
#define COMM_AT_ANSWER	2

#define CMD_RESET	0
#define CMD_PROC	1

#define HLK_PIN_ON	SW_RX
#define HLK_PIN_OFF	SW_TX

//--- Modos del HLK -----------------------------------------//
#define UNKNOW_MODE				0
#define AT_MODE					1
#define TRANSPARENT_MODE		2
#define GOING_AT_MODE			3

#define USART_CALLER	0
#define PROCESS_CALLER	1


//--- Funciones del Modulo -----------------------------------//
void CheckVersion (char *);
void SendCommandWithAnswer(const char *, void (*pCall) (char *));

unsigned char HLK_Mode(void);
void HLK_ATModeRx (unsigned char);
void HLK_TransparentModeRx (unsigned char);
void HLK_ATProcess (void);
unsigned char HLKToATMode (unsigned char);


#endif /* HLK_RM04_H_ */

