/*
 * standalone.h
 *
 *  Created on: 11/02/2016
 *      Author: Mariano
 */

#ifndef STANDALONE_H_
#define STANDALONE_H_

//estructura del modo StandAlone
typedef struct StandAlone
{
	unsigned char move_sensor_enable;
	unsigned char ldr_enable;
	unsigned char ldr_value;
	unsigned char max_dimmer_value_percent;
	unsigned char max_dimmer_value_dmx;
	unsigned char min_dimmer_value_percent;
	unsigned char min_dimmer_value_dmx;
	unsigned short power_up_timer_value;	//msecs
	unsigned short dimming_up_timer_value;	//msecs
	unsigned char dummy;

} StandAlone_Typedef;	//12 bytes alingned

//estados del modo STAND_ALONE
#define STAND_ALONE_INIT				0
#define STAND_ALONE_SHOW_CONF			1
#define STAND_ALONE_OFF					2
#define STAND_ALONE_OFF_1				3
#define STAND_ALONE_OFF_2				4
#define STAND_ALONE_RISING				5
#define STAND_ALONE_ON					6
#define STAND_ALONE_ON_1				7
#define STAND_ALONE_FALLING				8
#define STAND_ALONE_DIMMING_LAST		9
#define STAND_ALONE_DIMMING_UP			10
#define STAND_ALONE_DIMMING_DOWN		11

#define STAND_ALONE_UPDATE				20

//estados del menu STAND_ALONE
#define STAND_ALONE_MENU_INIT					0
#define STAND_ALONE_MENU_MOV_SENS				1
#define STAND_ALONE_MENU_LDR					2
#define STAND_ALONE_MENU_MAX_DIMMING			3
#define STAND_ALONE_MENU_MIN_DIMMING			4
#define STAND_ALONE_MENU_RAMP_ON_START			5
#define STAND_ALONE_MENU_RAMP_ON_DIMMING		6

#define STAND_ALONE_MENU_MOV_SENS_SELECTED			10
#define STAND_ALONE_MENU_MOV_SENS_SELECTED_1		11
#define STAND_ALONE_MENU_MOV_SENS_SELECTED_2		12
#define STAND_ALONE_MENU_LDR_SELECTED				13
#define STAND_ALONE_MENU_LDR_SELECTED_1				14
#define STAND_ALONE_MENU_LDR_SELECTED_2				15
#define STAND_ALONE_MENU_LDR_SELECTED_3				16
#define STAND_ALONE_MENU_LDR_SELECTED_4				17
#define STAND_ALONE_MENU_LDR_SELECTED_5				18
#define STAND_ALONE_MENU_MAX_DIMMING_SELECTED		19
#define STAND_ALONE_MENU_MAX_DIMMING_SELECTED_1		20
#define STAND_ALONE_MENU_MIN_DIMMING_SELECTED		21
#define STAND_ALONE_MENU_MIN_DIMMING_SELECTED_1		22
#define STAND_ALONE_MENU_RAMP_ON_START_SELECTED		23
#define STAND_ALONE_MENU_RAMP_ON_START_SELECTED_1	24
#define STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED	25
#define STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED_1	26

#define STAND_ALONE_MENU_UPDATE					30
#define STAND_ALONE_MENU_UPDATE_1				31

//estados de Menu Selections
#define MENU_ON			0
#define MENU_SELECTED	1
#define MENU_OFF		2
#define MENU_WAIT_FREE	3

//estados de ShowConf
#define STAND_ALONE_SHOW_CONF_0		0
#define STAND_ALONE_SHOW_CONF_1		1
#define STAND_ALONE_SHOW_CONF_2		2
#define STAND_ALONE_SHOW_CONF_3		3
#define STAND_ALONE_SHOW_CONF_4		4
#define STAND_ALONE_SHOW_CONF_5		5
#define STAND_ALONE_SHOW_CONF_6		6
#define STAND_ALONE_SHOW_CONF_7		7

#define STAND_ALONE_SHOW_CONF_RESET_END		STAND_ALONE_SHOW_CONF_7

#define DIM_UP		0
#define DIM_DOWN	1

//estados del menu STAND_ALONE CERT
#define STAND_ALONE_MENU_CERT_INIT_0			0
#define STAND_ALONE_MENU_CERT_INIT_1			1
#define STAND_ALONE_MENU_CERT_INIT_UP			2
#define STAND_ALONE_MENU_CERT_INIT_DOWN			3
#define STAND_ALONE_MENU_CERT_TEMP_0			4
#define STAND_ALONE_MENU_CERT_TEMP_1			5
#define STAND_ALONE_MENU_CERT_TEMP_UP			6
#define STAND_ALONE_MENU_CERT_TEMP_DOWN			7
#define STAND_ALONE_MENU_CERT_CURRENT_0			8
#define STAND_ALONE_MENU_CERT_CURRENT_1			9
#define STAND_ALONE_MENU_CERT_CURRENT_UP		10
#define STAND_ALONE_MENU_CERT_CURRENT_DOWN		11
#define STAND_ALONE_MENU_CERT_UPTIME_0			12
#define STAND_ALONE_MENU_CERT_UPTIME_1			13
#define STAND_ALONE_MENU_CERT_UPTIME_UP			14
#define STAND_ALONE_MENU_CERT_UPTIME_DOWN		15
#define STAND_ALONE_MENU_CERT_1TO10_0			16
#define STAND_ALONE_MENU_CERT_1TO10_1			17
#define STAND_ALONE_MENU_CERT_1TO10_UP			18
#define STAND_ALONE_MENU_CERT_1TO10_DOWN		19


#define TT_RISING_FALLING	1		//update del 1 a 10V (rampa subida y bajada)
#define TT_RISING_FALLING_FIRST_TIME	1
#define TT_STARTING			400		//tarda 940 msegs en arrancar la fuente MAS O MENOS OK
#define TT_MENU_TIMEOUT		30000

//#define TT_STARTING			100		//tarda 940 msegs en arrancar la fuente

#define ONE_TEN_INITIAL		51
//#define ONE_TEN_INITIAL		10		//valor inicial 1 a 10V para que la fuente arranue en regimen
#define MIN_DIMMING		5

//-------- Functions -------------
unsigned char FuncStandAlone (void);
unsigned char MenuStandAlone(void);
unsigned char ShowConfStandAlone(void);

void FuncStandAloneReset (void);
void MenuStandAloneReset (void);
void ShowConfStandAloneReset(void);
void ShowConfStandAloneResetEnd(void);

//para certificaciones
unsigned char FuncStandAloneCert (void);
void MenuStandAloneCert(void);
void MenuStandAloneResetCert(void);

#endif /* STANDALONE_H_ */
