/*
 * tcp_transceiver.c
 *
 *  Created on: 13/06/2016
 *      Author: Mariano
 */

#include "tcp_transceiver.h"
#include <string.h>



//--- Externals -----------------------------------//

//extern volatile unsigned char data1[];
//extern volatile unsigned char data[];
//
//#define data512		data1
//#define data256		data


//--- Globals -------------------------------------//




//--- Module Functions ----------------------------//

enum TcpMessages CheckTCPMessage(char * d, unsigned char * new_room_bright, unsigned char * new_lamp_bright)
{
	//reviso que tipo de mensaje tengo en data
	if (strncmp((char *) (const char *) "kAlive;\r", (char *)d, sizeof((char *) (const char *) "kAlive;\r")) == 0)
		return KEEP_ALIVE;

	if (strncmp((char *) (const char *) "geta;\r", (char *)d, sizeof((char *) (const char *) "geta;\r")) == 0)
		return GET_A;

	if ((*d == 'r') && (*(d + 2) == ','))
	{
		*new_room_bright = 20;
		return ROOM_BRIGHT;
	}

	if ((*d == 's') && (*(d + 2) == ','))
	{
		*new_room_bright = 20;
		return LAMP_BRIGHT;
	}

	return NONE_MSG;
}
