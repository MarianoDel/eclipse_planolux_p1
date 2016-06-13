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
	LAMP_BRIGHT
};



//--- Module Functions ----------------------------//
enum TcpMessages CheckTCPMessage(char *);

#endif /* TCP_TRANSCEIVER_H_ */
