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
#define SIZEOF_BUFFTCP	128
#define TT_TCP_SEND		1000

//--- ESTADOS DEL TCP SEND ---------//
#define TCP_TX_IDLE					0
#define TCP_TX_READY_TO_SEND		1
#define TCP_TX_WAIT_THE_SIGN		2
#define TCP_TX_SENDING				3
#define TCP_TX_WAIT_SEND_OK			4
#define TCP_TX_END_TRANSMISSION		5

//--- Module Functions ----------------------------//
enum TcpMessages CheckTCPMessage(char *, unsigned char *, unsigned char *);
unsigned char TCPPreProcess(unsigned char *, unsigned char *);
void ReadPcktR(unsigned char *, unsigned short, unsigned char *);
void ReadPcktS(unsigned char *);
unsigned short GetValue (unsigned char *);
unsigned char TCPSendData (unsigned char , char *);
void TCPProcess (void);

#endif /* TCP_TRANSCEIVER_H_ */
