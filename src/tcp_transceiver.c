/*
 * tcp_transceiver.c
 *
 *  Created on: 13/06/2016
 *      Author: Mariano
 */

#include "tcp_transceiver.h"
#include <string.h>



//--- Externals -----------------------------------//

extern volatile unsigned char data1[];
extern volatile unsigned char data[];

#define data512		data1
#define data256		data


//--- Globals -------------------------------------//




//--- Module Functions ----------------------------//

enum TcpMessages CheckTCPMessage(char * d)
{
	//reviso que tipo de mensaje tengo en data
	if (strncmp((char *) (const char *) "kAlive\r\n", (char *)data256, sizeof((char *) (const char *) "kAlive\r\n")) == 0)
		return KEEP_ALIVE;

	return NONE_MSG;
}
