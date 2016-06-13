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
		ReadPcktR(d, 1, new_room_bright);
		return ROOM_BRIGHT;
	}

	if ((*d == 's') && (*(d + 2) == ','))
	{
		*new_room_bright = 20;
		return LAMP_BRIGHT;
	}

	return NONE_MSG;
}

void ReadPcktR(unsigned char * p, unsigned short own_addr, unsigned char * new_r)
{
	unsigned char new_shine;

	if (*(p+2) != ',')
		return;

	if (GetValue(p + 3) == 0xffff)
		return;

	new_shine = GetValue(p + 3);

	switch (*(p+1))
	{
		case '0':
			if ((own_addr >= 1) && (own_addr < 31))
			{
				*new_r = new_shine;
			}
			break;

		case '1':
			if ((own_addr >= 31) && (own_addr < 61))
			{
				*new_r = new_shine;
			}
			break;

		case '2':
			if ((own_addr >= 61) && (own_addr < 91))
			{
				*new_r = new_shine;
			}
			break;

		case '3':
			if ((own_addr >= 91) && (own_addr < 121))
			{
				*new_r = new_shine;
			}
			break;

		default:
			break;
	}
}

//en S me llega un parametro particular
//del brillo de esa lampara pero indicando cada habitacion
//s0,0,255;\r\n
void ReadPcktS(unsigned char * p)
{
	/*
	unsigned char room;
	unsigned char slider;
	unsigned char new_shine;
	unsigned short ii;

	if (*(p+2) != ',')
		return;

	room = *(p+1) - '0';

	if ((room < 0) || (room > 3))
		return;

	ii = GetValue(p + 3);
	if (ii == 0xffff)
		return;
	else
		slider = (unsigned char) ii;

	if (slider < 10)
	{
		ii = GetValue(p + 5);
		if (ii == 0xffff)
			return;
		else
			new_shine = (unsigned char) ii;
	}
	else if (slider < 30)
	{
		ii = GetValue(p + 6);
		if (ii == 0xffff)
			return;
		else
			new_shine = (unsigned char) ii;
	}
	else
		return;

	ii = (room * 30) + slider + 1;
	orig_shine_slider[ii] = new_shine;
	data1[ii] = ((new_shine + 1) * orig_shine_room[room]) >> 8;
	*/
}

unsigned short GetValue (unsigned char * pn)
{
	unsigned char i;
	unsigned char colon = 0;
	unsigned short new_val = 0xffff;

	//me fijo la posiciones de la , o ;
	for (i = 0; i < 6; i++)
	{
		if ((*(pn + i) == ',') || ((*(pn + i) == ';')))
		{
			colon = i;
			i = 6;
		}
	}

	if ((colon == 0) || (colon >= 5))
		return 0;

	switch (colon)
	{
		case 1:
			new_val = *pn - '0';
			break;

		case 2:
			new_val = (*pn - '0') * 10 + (*(pn + 1) - '0');
			break;

		case 3:
			new_val = (*pn - '0') * 100 + (*(pn + 1) - '0') * 10 + (*(pn + 2) - '0');
			break;

		case 4:
			new_val = (*pn - '0') * 1000 + (*(pn + 1) - '0') * 100 + (*(pn + 2) - '0') * 10 + (*(pn + 2) - '0');
			break;

	}
	return new_val;
}
