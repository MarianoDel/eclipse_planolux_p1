/*
 * tcp_transceiver.h
 *
 *  Created on: 13/06/2016
 *      Author: Mariano
 */

#ifndef TCP_TRANSCEIVER_H_
#define TCP_TRANSCEIVER_H_


//-------- Module Defines -------------------------//
enum TcpMessages
{
	NONE_MSG = 0,
	KEEP_ALIVE,
	ROOM_BRIGHT,
	LAMP_BRIGHT,
	GET_A,
	LIGHTS_OFF,
	LIGHTS_ON
};

#define TT_KALIVE	8000	//8 segundos de keep alive para el tcp

//--- Module Functions ----------------------------//
enum TcpMessages CheckTCPMessage(char *, unsigned char *, unsigned char *);
void ReadPcktR(unsigned char *, unsigned short, unsigned char *);
void ReadPcktS(unsigned char *);
unsigned short GetValue (unsigned char *);

#endif /* TCP_TRANSCEIVER_H_ */
