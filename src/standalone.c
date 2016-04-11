/*
 * standalone.c
 *
 *  Created on: 11/02/2016
 *      Author: Mariano
 */

#include "standalone.h"
#include "lcd.h"
#include "main_menu.h"
#include "hard.h"
#include "stm32f0xx.h"

#include <stdio.h>
#include <string.h>

#include "stm32f0x_gpio.h"


/* Externals variables ---------------------------------------------------------*/
extern volatile unsigned short standalone_timer;
extern volatile unsigned short standalone_enable_menu_timer;

extern const char * s_blank_line [];

extern StandAlone_Typedef StandAloneStruct_constant;

/* Global variables ------------------------------------------------------------*/
unsigned char standalone_state = 0;
unsigned char standalone_selections = 0;
unsigned short standalone_ii = 0;
//unsigned short standalone_dimming_top = 0;

unsigned char standalone_dimming_last_value = 0;
unsigned char standalone_dimming_last_slope = 0;

unsigned char standalone_menu_state = 0;
unsigned char standalone_show_conf = 0;



const unsigned char s_sel [] = { 0x02, 0x08, 0x0f };

StandAlone_Typedef StandAloneStruct_local;
#define standalone_dimming_top StandAloneStruct_local.max_dimmer_value_dmx

//-------- Functions -------------

//muestro el menu y hago funciones (pero sin mostrar nada) hasta que pasen 30 segs
//con standalone_menu_show
unsigned char FuncStandAlone (void)
{
	unsigned char resp = RESP_CONTINUE;
	unsigned char resp_down = RESP_CONTINUE;

	switch (standalone_state)
	{
		case STAND_ALONE_INIT:
			//un segundo la pantalla principal
			standalone_timer = 1000;
			standalone_enable_menu_timer = TT_MENU_TIMEOUT;	//30 segs de menu standalone

			//TODO: leer estructura y verificar funcion cargar valore sdefualt o ultimos seleccioneados
			memcpy(&StandAloneStruct_local, &StandAloneStruct_constant, sizeof(StandAloneStruct_local));
			MenuStandAloneReset();
			standalone_selections = MENU_OFF;
			standalone_state++;
			break;

		case STAND_ALONE_SHOW_CONF:
			resp_down = ShowConfStandAlone();

			if (resp_down == RESP_FINISH)
			{
				standalone_state = STAND_ALONE_UPDATE;
				standalone_selections = MENU_ON;
			}
			break;

		case STAND_ALONE_UPDATE:
			EXTIOff ();
			standalone_dimming_last_slope = DIM_DOWN;
			standalone_dimming_last_value = StandAloneStruct_local.max_dimmer_value_dmx;
			standalone_state = STAND_ALONE_OFF;
			break;

		case STAND_ALONE_OFF:
			if (CheckACSw() > S_NO)	//estan prendiendo o queriendo dimerizar
			{
				standalone_timer = 1500;
				standalone_state = STAND_ALONE_OFF_1;
			}
			break;

		case STAND_ALONE_OFF_1:
			if (CheckACSw() == S_NO)	//liberaron el sw
			{
				RELAY_ON;
				if (standalone_selections == MENU_OFF)
				{
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "Switching ON... ");
				}

				if (standalone_dimming_last_value >= ONE_TEN_INITIAL)
				{
					standalone_state = STAND_ALONE_RISING;
					standalone_timer = 0;
				}
				else
				{
					standalone_ii = ONE_TEN_INITIAL;
					Update_TIM3_CH1 (standalone_ii);
					standalone_timer = TT_STARTING;	//tarda 940 msegs en arrancar la fuente
					standalone_state = STAND_ALONE_OFF_2;
				}
			}
			break;

		case STAND_ALONE_OFF_2:
			if (!standalone_timer)
			{
				//standalone_timer = TT_RISING_FALLING;
				standalone_timer = TT_RISING_FALLING_FIRST_TIME;
				if (standalone_ii != standalone_dimming_last_value)
				{
					if (standalone_ii < standalone_dimming_last_value)
						standalone_ii++;
					else
						standalone_ii--;

					Update_TIM3_CH1 (standalone_ii);
				}
				else
				{
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights ON     ");
					}
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_RISING:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.power_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.power_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING_FIRST_TIME;

				if ((standalone_ii < standalone_dimming_last_value) && (standalone_ii < StandAloneStruct_local.max_dimmer_value_dmx))
				{
					Update_TIM3_CH1 (standalone_ii);
					standalone_ii++;
				}
				else
				{
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights ON     ");
					}

					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_ON:
			if (CheckACSw() > S_NO)	//estan apagando o queriendo dimerizar
			{
				standalone_timer = 1500;
				standalone_state = STAND_ALONE_ON_1;
			}
			break;

		case STAND_ALONE_ON_1:
			if (CheckACSw() == S_NO)	//liberaron el sw
			{
				if (standalone_timer)	//apagaron
				{
					standalone_dimming_last_value = standalone_ii;
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "Switching OFF...");

						//me fijo si estoy muy cerca del limite bajo
//						local = StandAloneStruct_local.min_dimmer_value_dmx - standalone_ii;
//						if ((local < 10) && (local > -10))
//							standalone_dimming_last_slope = DIM_UP;
					}
					standalone_state = STAND_ALONE_FALLING;
					standalone_timer = TT_RISING_FALLING;		//TODO guarda que me bloque el if de abajo
				}
			}

			if (!standalone_timer)		//estan dimerizando!!!
				standalone_state = STAND_ALONE_DIMMING_LAST;

			break;

		case STAND_ALONE_FALLING:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.power_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.power_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING_FIRST_TIME;

				if (standalone_ii > 0)
				{
					Update_TIM3_CH1 (standalone_ii);
					standalone_ii--;
				}
				else
				{
					Update_TIM3_CH1 (0);
					standalone_state = STAND_ALONE_OFF;
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights OFF    ");
					}
					RELAY_OFF;
				}
			}
			break;

		case STAND_ALONE_DIMMING_LAST:
			if (standalone_dimming_last_slope == DIM_UP)
				standalone_state = STAND_ALONE_DIMMING_UP;
			else
				standalone_state = STAND_ALONE_DIMMING_DOWN;

			break;

		case STAND_ALONE_DIMMING_UP:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.dimming_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.dimming_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING;

				if (CheckACSw() > S_NO)
				{
					if (standalone_ii < StandAloneStruct_local.max_dimmer_value_dmx)
					{
						standalone_ii++;
						Update_TIM3_CH1 (standalone_ii);
					}
				}
				else	//si liberaron y estoy en maximo lo doy vuelta
				{
					if (standalone_ii >= StandAloneStruct_local.max_dimmer_value_dmx)
						standalone_dimming_last_slope = DIM_DOWN;
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_DIMMING_DOWN:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.dimming_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.dimming_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING;

				if (CheckACSw() > S_NO)
				{
					if (standalone_ii > StandAloneStruct_local.min_dimmer_value_dmx)
					{
						standalone_ii--;
						Update_TIM3_CH1 (standalone_ii);
					}
				}
				else	//si liberaron y estoy en minimo lo doy vuelta
				{
					if (standalone_ii <= StandAloneStruct_local.min_dimmer_value_dmx)
						standalone_dimming_last_slope = DIM_UP;
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		default:
			standalone_state = STAND_ALONE_INIT;
			break;
	}


	//veo el menu solo si alguien toca los botones o timeout
	switch (standalone_selections)
	{
		case MENU_ON:
			//estado normal
			//resp_down = MenuStandAlone(MENU_NORMAL);
			resp_down = MenuStandAlone();

			if (resp_down == RESP_WORKING)	//alguien esta tratando de seleccionar algo, le doy tiempo
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;

			if (resp_down == RESP_SELECTED)	//se selecciono algo
			{
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;
				//standalone_state = STAND_ALONE_INIT;
				if (RELAY)
					standalone_state = STAND_ALONE_ON;
				else
					standalone_state = STAND_ALONE_OFF;
				standalone_selections++;
			}

			if (!standalone_enable_menu_timer)	//ya mostre el menu mucho tiempo, lo apago
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *)s_blank_line);
				LCD_2DO_RENGLON;
				LCDTransmitStr((const char *)s_blank_line);
				standalone_selections = MENU_OFF;
			}
			break;

		case MENU_SELECTED:
			//estado algo seleccionado espero update
			resp_down = FuncShowBlink ((const char *) "Something Select", (const char *) "Updating Values", 1, BLINK_NO);

			if (resp_down == RESP_FINISH)
			{
				standalone_state = STAND_ALONE_UPDATE;
				standalone_selections = MENU_ON;
			}
			break;

		case MENU_OFF:
			//estado menu apagado
			if ((CheckS1() > S_NO) || (CheckS2() > S_NO))
			{
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;			//vuelvo a mostrar
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");

				standalone_selections++;

				if (standalone_state == STAND_ALONE_SHOW_CONF)
					ShowConfStandAloneResetEnd();
			}
			break;

		case MENU_WAIT_FREE:
			if ((CheckS1() == S_NO) && (CheckS2() == S_NO))
			{
				standalone_selections = MENU_ON;
				//voy a activar el Menu
				//me fijo en ue parte del Menu estaba
				//TODO ES UNA CHANCHADA, PERO BUENO...
				if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_DIMMING)
				{
					//salgo directo
					LCD_2DO_RENGLON;
					LCDTransmitStr((const char *) "Cont.     Select");
				}
				else
				{
					if (standalone_menu_state <= STAND_ALONE_MENU_MOV_SENS_SELECTED_2)
					{
						standalone_menu_state = STAND_ALONE_MENU_MOV_SENS;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_LDR_SELECTED_5)
					{
						standalone_menu_state = STAND_ALONE_MENU_LDR;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_MAX_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_MIN_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_START_SELECTED_1)
					{
						standalone_menu_state =STAND_ALONE_MENU_RAMP_ON_START;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING;
					}
					FuncOptionsReset ();
					FuncShowSelectv2Reset ();
					FuncChangeReset ();
				}
			}
			break;

		default:
			standalone_selections = 0;
			break;
	}

	if (CheckS1() > S_HALF)
		resp = RESP_CHANGE_ALL_UP;

	return resp;
}


unsigned char MenuStandAlone(void)
{
	unsigned char resp = RESP_CONTINUE;
	unsigned char resp_down = RESP_CONTINUE;
	unsigned char dummy = 0;

	switch (standalone_menu_state)
	{
		case STAND_ALONE_MENU_INIT:
			//empiezo con las selecciones
			resp_down = FuncShowBlink ((const char *) "Starting StandAlone", (const char *) "Selections", 1, BLINK_NO);

			if (resp_down == RESP_FINISH)
				standalone_menu_state++;
			break;

		case STAND_ALONE_MENU_MOV_SENS:
			resp_down = FuncShowSelectv2 ((const char * ) "Move Sensor Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_LDR;

			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_MOV_SENS_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_LDR:
			resp_down = FuncShowSelectv2 ((const char * ) "Energy Harv Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING;


			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_LDR_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_MAX_DIMMING:
			resp_down = FuncShowSelectv2 ((const char * ) "Max Dimming Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING;

			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_MIN_DIMMING:
			resp_down = FuncShowSelectv2 ((const char * ) "Min Dimming Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_START;

			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_RAMP_ON_START:
			resp_down = FuncShowSelectv2 ((const char * ) "Ramp ON/OFF Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING;

			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_START_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_RAMP_ON_DIMMING:
			resp_down = FuncShowSelectv2 ((const char * ) "Ramp on Dim Conf");

			if (resp_down == RESP_CHANGE)	//cambio de menu
				standalone_menu_state = STAND_ALONE_MENU_MOV_SENS;

			if (resp_down == RESP_SELECTED)	//se eligio el menu
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED;

			if (resp_down != RESP_CONTINUE)
				resp = RESP_WORKING;

			break;

		case STAND_ALONE_MENU_MOV_SENS_SELECTED:
			if (StandAloneStruct_local.move_sensor_enable)
				resp_down = 0x80;
			else
				resp_down = 0x81;

			FuncOptions ((const char *) "on   off   back ",(const char *) s_blank_line, (unsigned char *)s_sel, 3, resp_down);
			standalone_menu_state++;
			break;

		case STAND_ALONE_MENU_MOV_SENS_SELECTED_1:
			resp_down = FuncOptions ((const char *) "on   off   back ",(const char *) s_blank_line, (unsigned char *)s_sel, 3, 0);

			if ((resp_down & 0x0f) == RESP_SELECTED)
			{
				resp_down = resp_down & 0xf0;
				resp_down >>= 4;
				if (resp_down == 0)
				{
					StandAloneStruct_local.move_sensor_enable = 1;
				}

				if (resp_down == 1)
				{
					StandAloneStruct_local.move_sensor_enable = 0;
				}

				if (resp_down == 2)
				{
					resp = RESP_WORKING;
					standalone_menu_state++;
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "wait to free    ");
				}
				else
				{
					resp = RESP_SELECTED;
					standalone_menu_state = STAND_ALONE_MENU_MOV_SENS;
				}
			}
			break;

		case STAND_ALONE_MENU_MOV_SENS_SELECTED_2:
			if (CheckS2() == S_NO)
				standalone_menu_state = STAND_ALONE_MENU_MOV_SENS;

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_LDR_SELECTED:
			if (StandAloneStruct_local.ldr_enable)
				resp_down = 0x80;
			else
				resp_down = 0x81;

			FuncOptions ((const char *) "on   off   back ",(const char *) s_blank_line, (unsigned char *)s_sel, 3, resp_down);
			standalone_menu_state++;
			break;

		case STAND_ALONE_MENU_LDR_SELECTED_1:
			resp_down = FuncOptions ((const char *) "on   off   back ",(const char *) s_blank_line,(unsigned char *) s_sel, 3, 0);

			if ((resp_down & 0x0f) == RESP_SELECTED)
			{
				resp_down = resp_down & 0xf0;
				resp_down >>= 4;
				if (resp_down == 0)
				{
					StandAloneStruct_local.ldr_enable = 1;
					standalone_menu_state++;
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "wait to free    ");
				}

				if (resp_down == 1)
				{
					StandAloneStruct_local.ldr_enable = 0;
					resp = RESP_SELECTED;
					standalone_menu_state = STAND_ALONE_MENU_LDR;
				}

				if (resp_down == 2)
				{
					resp = RESP_WORKING;
					standalone_menu_state = STAND_ALONE_MENU_LDR_SELECTED_4;
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "wait to free    ");
				}
			}
			break;

		case STAND_ALONE_MENU_LDR_SELECTED_2:
			if (CheckS2() == S_NO)
				standalone_menu_state++;

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_LDR_SELECTED_3:
			//resp_down = FuncChange (&StandAloneStruct_local.ldr_value);
			resp_down = FuncChangePercent (&StandAloneStruct_local.ldr_value);

			if (resp_down == RESP_FINISH)
				standalone_menu_state = STAND_ALONE_MENU_LDR_SELECTED_5;

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_LDR_SELECTED_4:
			if (CheckS2() == S_NO)
				standalone_menu_state = STAND_ALONE_MENU_LDR;

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_LDR_SELECTED_5:
			resp = RESP_WORKING;
			if (CheckS2() == S_NO)
			{
				standalone_menu_state = STAND_ALONE_MENU_LDR;
				resp = RESP_SELECTED;
			}
			break;

		case STAND_ALONE_MENU_MAX_DIMMING_SELECTED:
			resp_down = FuncChangePercent (&StandAloneStruct_local.max_dimmer_value_percent);

			if (resp_down == RESP_FINISH)
			{
				unsigned short local;
				local = StandAloneStruct_local.max_dimmer_value_percent * 255;
				local = local / 100;
				StandAloneStruct_local.max_dimmer_value_dmx = local;

				standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING_SELECTED_1;
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");
			}

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_MAX_DIMMING_SELECTED_1:
			resp = RESP_WORKING;
			if (CheckS2() == S_NO)
			{
				//hago el update si corresponde
				if (standalone_ii > StandAloneStruct_local.max_dimmer_value_dmx)
				{
					standalone_ii = StandAloneStruct_local.max_dimmer_value_dmx;
					Update_TIM3_CH1 (standalone_ii);
					standalone_dimming_last_slope = DIM_DOWN;
				}
				standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING;
				resp = RESP_SELECTED;
			}
			break;

		case STAND_ALONE_MENU_MIN_DIMMING_SELECTED:
			resp_down = FuncChangePercent (&StandAloneStruct_local.min_dimmer_value_percent);

			if (resp_down == RESP_FINISH)
			{
				unsigned short local;
				local = StandAloneStruct_local.min_dimmer_value_percent * 255;
				local = local / 100;
				StandAloneStruct_local.min_dimmer_value_dmx = local;

				standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING_SELECTED_1;
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");
			}

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_MIN_DIMMING_SELECTED_1:
			resp = RESP_WORKING;
			if (CheckS2() == S_NO)
			{
				//hago el update si corresponde
				if (standalone_ii < StandAloneStruct_local.min_dimmer_value_dmx)
				{
					standalone_ii = StandAloneStruct_local.min_dimmer_value_dmx;
					Update_TIM3_CH1 (standalone_ii);
					standalone_dimming_last_slope = DIM_UP;
				}

				standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING;
				resp = RESP_SELECTED;
			}
			break;

		case STAND_ALONE_MENU_RAMP_ON_START_SELECTED:
			//ajusto el timer
			dummy = StandAloneStruct_local.power_up_timer_value / 1000;
			resp_down = FuncChangeSecs (&dummy);

			if (resp_down == RESP_FINISH)
			{
				StandAloneStruct_local.power_up_timer_value = dummy * 1000;
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_START_SELECTED_1;
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");
			}

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_RAMP_ON_START_SELECTED_1:
			resp = RESP_WORKING;
			if (CheckS2() == S_NO)
			{
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_START;
				resp = RESP_SELECTED;
			}
			break;

		case STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED:
			//ajusto el timer
			dummy = StandAloneStruct_local.dimming_up_timer_value / 1000;
			resp_down = FuncChangeSecs (&dummy);

			if (resp_down == RESP_FINISH)
			{
				StandAloneStruct_local.dimming_up_timer_value = dummy * 1000;
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED_1;
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");
			}

			resp = RESP_WORKING;
			break;

		case STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED_1:
			resp = RESP_WORKING;
			if (CheckS2() == S_NO)
			{
				standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING;
				resp = RESP_SELECTED;
			}
			break;

		default:
			standalone_menu_state = STAND_ALONE_MENU_INIT;
			break;
	}
	return resp;
}


unsigned char ShowConfStandAlone(void)
{
	unsigned char resp = RESP_CONTINUE;
	unsigned char resp_down = RESP_CONTINUE;
	char s_current [20];	//usar s_conf[]

	switch (standalone_show_conf)
	{
		case STAND_ALONE_SHOW_CONF_0:
			LCD_1ER_RENGLON;
			LCDTransmitStr((const char *) "StandAlone Conf.");
			standalone_show_conf++;
			break;

		case STAND_ALONE_SHOW_CONF_1:
			if (StandAloneStruct_local.move_sensor_enable)
			{
				resp_down = FuncScroll2((const char *)"Mov Sens Enabled");
			}
			else
				resp_down = FuncScroll2((const char *)"Mov Sen Disabled");

			if (resp_down == RESP_FINISH)
				standalone_show_conf++;

			break;

		case STAND_ALONE_SHOW_CONF_2:
			if (StandAloneStruct_local.ldr_enable)
			{
				memset(s_current, ' ', sizeof(s_current));
				sprintf(s_current, "Harv. On at %3d", StandAloneStruct_local.ldr_value);
				strcat(s_current, (const char*)"%");
				resp_down = FuncScroll2((const char *) s_current);
			}
			else
				resp_down = FuncScroll2((const char *)"Energy Harv. OFF");

			if (resp_down == RESP_FINISH)
				standalone_show_conf++;

			break;

		case STAND_ALONE_SHOW_CONF_3:
			memset(s_current, ' ', sizeof(s_current));
			sprintf(s_current, "Max Dimm at %3d", StandAloneStruct_local.max_dimmer_value_percent);
			strcat(s_current, (const char*)"%");
			resp_down = FuncScroll2((const char *) s_current);

			if (resp_down == RESP_FINISH)
				standalone_show_conf++;

			break;

		case STAND_ALONE_SHOW_CONF_4:
			memset(s_current, ' ', sizeof(s_current));
			sprintf(s_current, "Min Dimm at %3d", StandAloneStruct_local.min_dimmer_value_percent);
			strcat(s_current, (const char*)"%");
			resp_down = FuncScroll2((const char *) s_current);

			if (resp_down == RESP_FINISH)
				standalone_show_conf++;

			break;

		case STAND_ALONE_SHOW_CONF_5:
			memset(s_current, ' ', sizeof(s_current));
			sprintf(s_current, "Ramp ON Start %2d", (StandAloneStruct_local.power_up_timer_value / 1000));
			strcat(s_current, (const char*)"s");
			resp_down = FuncScroll2((const char *) s_current);

			if (resp_down == RESP_FINISH)
				standalone_show_conf++;

			break;

		case STAND_ALONE_SHOW_CONF_6:
			memset(s_current, ' ', sizeof(s_current));
			sprintf(s_current, "Ramp Dimming %2d", (StandAloneStruct_local.dimming_up_timer_value / 1000));
			strcat(s_current, (const char*)"s");
			resp_down = FuncScroll2((const char *) s_current);

			if (resp_down == RESP_FINISH)
			{
				standalone_show_conf = STAND_ALONE_SHOW_CONF_0;
				resp = RESP_FINISH;
			}
			break;

		case STAND_ALONE_SHOW_CONF_RESET_END:
		default:
			standalone_show_conf = STAND_ALONE_SHOW_CONF_0;
			resp = RESP_FINISH;
			break;
	}
	return resp;
}

void FuncStandAloneReset (void)
{
	standalone_state = STAND_ALONE_INIT;
}

void MenuStandAloneReset(void)
{
	standalone_menu_state = STAND_ALONE_MENU_INIT;
}

void ShowConfStandAloneReset(void)
{
	standalone_show_conf = STAND_ALONE_SHOW_CONF_0;
}

void ShowConfStandAloneResetEnd(void)
{
	standalone_show_conf = STAND_ALONE_SHOW_CONF_RESET_END;
}


//muestro el menu y hago funciones (pero sin mostrar nada) hasta que pasen 30 segs
//con standalone_menu_show
unsigned char FuncStandAloneCert (void)
{
	unsigned char resp = RESP_CONTINUE;
	unsigned char resp_down = RESP_CONTINUE;

	switch (standalone_state)
	{
		case STAND_ALONE_INIT:
			//un segundo la pantalla principal
			standalone_timer = 1000;
			standalone_enable_menu_timer = TT_MENU_TIMEOUT;	//30 segs de menu standalone

			//TODO: leer estructura y verificar funcion cargar valore sdefualt o ultimos seleccioneados
			memcpy(&StandAloneStruct_local, &StandAloneStruct_constant, sizeof(StandAloneStruct_local));
			MenuStandAloneReset();
			standalone_selections = MENU_OFF;
			standalone_state++;
			break;

		case STAND_ALONE_SHOW_CONF:
			resp_down = ShowConfStandAlone();

			if (resp_down == RESP_FINISH)
			{
				standalone_state = STAND_ALONE_UPDATE;
				standalone_selections = MENU_ON;
			}
			break;

		case STAND_ALONE_UPDATE:
			EXTIOff ();
			standalone_dimming_last_slope = DIM_DOWN;
			standalone_dimming_last_value = StandAloneStruct_local.max_dimmer_value_dmx;
			standalone_state = STAND_ALONE_OFF;
			break;

		case STAND_ALONE_OFF:
			if (CheckACSw() > S_NO)	//estan prendiendo o queriendo dimerizar
			{
				standalone_timer = 1500;
				standalone_state = STAND_ALONE_OFF_1;
			}
			break;

		case STAND_ALONE_OFF_1:
			if (CheckACSw() == S_NO)	//liberaron el sw
			{
				RELAY_ON;
				if (standalone_selections == MENU_OFF)
				{
					LCD_1ER_RENGLON;
					LCDTransmitStr((const char *) "Switching ON... ");
				}

				if (standalone_dimming_last_value >= ONE_TEN_INITIAL)
				{
					standalone_state = STAND_ALONE_RISING;
					standalone_timer = 0;
				}
				else
				{
					standalone_ii = ONE_TEN_INITIAL;
					Update_TIM3_CH1 (standalone_ii);
					standalone_timer = TT_STARTING;	//tarda 940 msegs en arrancar la fuente
					standalone_state = STAND_ALONE_OFF_2;
				}
			}
			break;

		case STAND_ALONE_OFF_2:
			if (!standalone_timer)
			{
				//standalone_timer = TT_RISING_FALLING;
				standalone_timer = TT_RISING_FALLING_FIRST_TIME;
				if (standalone_ii != standalone_dimming_last_value)
				{
					if (standalone_ii < standalone_dimming_last_value)
						standalone_ii++;
					else
						standalone_ii--;

					Update_TIM3_CH1 (standalone_ii);
				}
				else
				{
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights ON     ");
					}
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_RISING:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.power_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.power_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING_FIRST_TIME;

				if ((standalone_ii < standalone_dimming_last_value) && (standalone_ii < StandAloneStruct_local.max_dimmer_value_dmx))
				{
					Update_TIM3_CH1 (standalone_ii);
					standalone_ii++;
				}
				else
				{
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights ON     ");
					}

					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_ON:
			if (CheckACSw() > S_NO)	//estan apagando o queriendo dimerizar
			{
				standalone_timer = 1500;
				standalone_state = STAND_ALONE_ON_1;
			}
			break;

		case STAND_ALONE_ON_1:
			if (CheckACSw() == S_NO)	//liberaron el sw
			{
				if (standalone_timer)	//apagaron
				{
					standalone_dimming_last_value = standalone_ii;
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "Switching OFF...");

						//me fijo si estoy muy cerca del limite bajo
//						local = StandAloneStruct_local.min_dimmer_value_dmx - standalone_ii;
//						if ((local < 10) && (local > -10))
//							standalone_dimming_last_slope = DIM_UP;
					}
					standalone_state = STAND_ALONE_FALLING;
					standalone_timer = TT_RISING_FALLING;		//TODO guarda que me bloque el if de abajo
				}
			}

			if (!standalone_timer)		//estan dimerizando!!!
				standalone_state = STAND_ALONE_DIMMING_LAST;

			break;

		case STAND_ALONE_FALLING:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.power_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.power_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING_FIRST_TIME;

				if (standalone_ii > 0)
				{
					Update_TIM3_CH1 (standalone_ii);
					standalone_ii--;
				}
				else
				{
					Update_TIM3_CH1 (0);
					standalone_state = STAND_ALONE_OFF;
					if (standalone_selections == MENU_OFF)
					{
						LCD_1ER_RENGLON;
						LCDTransmitStr((const char *) "  Lights OFF    ");
					}
					RELAY_OFF;
				}
			}
			break;

		case STAND_ALONE_DIMMING_LAST:
			if (standalone_dimming_last_slope == DIM_UP)
				standalone_state = STAND_ALONE_DIMMING_UP;
			else
				standalone_state = STAND_ALONE_DIMMING_DOWN;

			break;

		case STAND_ALONE_DIMMING_UP:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.dimming_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.dimming_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING;

				if (CheckACSw() > S_NO)
				{
					if (standalone_ii < StandAloneStruct_local.max_dimmer_value_dmx)
					{
						standalone_ii++;
						Update_TIM3_CH1 (standalone_ii);
					}
				}
				else	//si liberaron y estoy en maximo lo doy vuelta
				{
					if (standalone_ii >= StandAloneStruct_local.max_dimmer_value_dmx)
						standalone_dimming_last_slope = DIM_DOWN;
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		case STAND_ALONE_DIMMING_DOWN:
			if (!standalone_timer)
			{
				if (StandAloneStruct_local.dimming_up_timer_value > 255)
					standalone_timer = (StandAloneStruct_local.dimming_up_timer_value >> 8);
				else
					standalone_timer = TT_RISING_FALLING;

				if (CheckACSw() > S_NO)
				{
					if (standalone_ii > StandAloneStruct_local.min_dimmer_value_dmx)
					{
						standalone_ii--;
						Update_TIM3_CH1 (standalone_ii);
					}
				}
				else	//si liberaron y estoy en minimo lo doy vuelta
				{
					if (standalone_ii <= StandAloneStruct_local.min_dimmer_value_dmx)
						standalone_dimming_last_slope = DIM_UP;
					standalone_state = STAND_ALONE_ON;
				}
			}
			break;

		default:
			standalone_state = STAND_ALONE_INIT;
			break;
	}


	//veo el menu solo si alguien toca los botones o timeout
	switch (standalone_selections)
	{
		case MENU_ON:
			//estado normal
			//resp_down = MenuStandAlone(MENU_NORMAL);
			resp_down = MenuStandAlone();

			if (resp_down == RESP_WORKING)	//alguien esta tratando de seleccionar algo, le doy tiempo
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;

			if (resp_down == RESP_SELECTED)	//se selecciono algo
			{
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;
				//standalone_state = STAND_ALONE_INIT;
				if (RELAY)
					standalone_state = STAND_ALONE_ON;
				else
					standalone_state = STAND_ALONE_OFF;
				standalone_selections++;
			}

			if (!standalone_enable_menu_timer)	//ya mostre el menu mucho tiempo, lo apago
			{
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *)s_blank_line);
				LCD_2DO_RENGLON;
				LCDTransmitStr((const char *)s_blank_line);
				standalone_selections = MENU_OFF;
			}
			break;

		case MENU_SELECTED:
			//estado algo seleccionado espero update
			resp_down = FuncShowBlink ((const char *) "Something Select", (const char *) "Updating Values", 1, BLINK_NO);

			if (resp_down == RESP_FINISH)
			{
				standalone_state = STAND_ALONE_UPDATE;
				standalone_selections = MENU_ON;
			}
			break;

		case MENU_OFF:
			//estado menu apagado
			if ((CheckS1() > S_NO) || (CheckS2() > S_NO))
			{
				standalone_enable_menu_timer = TT_MENU_TIMEOUT;			//vuelvo a mostrar
				LCD_1ER_RENGLON;
				LCDTransmitStr((const char *) "wait to free    ");

				standalone_selections++;

				if (standalone_state == STAND_ALONE_SHOW_CONF)
					ShowConfStandAloneResetEnd();
			}
			break;

		case MENU_WAIT_FREE:
			if ((CheckS1() == S_NO) && (CheckS2() == S_NO))
			{
				standalone_selections = MENU_ON;
				//voy a activar el Menu
				//me fijo en ue parte del Menu estaba
				//TODO ES UNA CHANCHADA, PERO BUENO...
				if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_DIMMING)
				{
					//salgo directo
					LCD_2DO_RENGLON;
					LCDTransmitStr((const char *) "Cont.     Select");
				}
				else
				{
					if (standalone_menu_state <= STAND_ALONE_MENU_MOV_SENS_SELECTED_2)
					{
						standalone_menu_state = STAND_ALONE_MENU_MOV_SENS;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_LDR_SELECTED_5)
					{
						standalone_menu_state = STAND_ALONE_MENU_LDR;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_MAX_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_MAX_DIMMING;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_MIN_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_MIN_DIMMING;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_START_SELECTED_1)
					{
						standalone_menu_state =STAND_ALONE_MENU_RAMP_ON_START;
					}
					else if (standalone_menu_state <= STAND_ALONE_MENU_RAMP_ON_DIMMING_SELECTED_1)
					{
						standalone_menu_state = STAND_ALONE_MENU_RAMP_ON_DIMMING;
					}
					FuncOptionsReset ();
					FuncShowSelectv2Reset ();
					FuncChangeReset ();
				}
			}
			break;

		default:
			standalone_selections = 0;
			break;
	}

	if (CheckS1() > S_HALF)
		resp = RESP_CHANGE_ALL_UP;

	return resp;
}
