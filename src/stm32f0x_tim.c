/**
  ******************************************************************************
  * @file    Template_2/stm32f0x_tim.c
  * @author  Nahuel
  * @version V1.0
  * @date    22-August-2014
  * @brief   TIM functions.
  ******************************************************************************
  * @attention
  *
  * Use this functions to configure timers.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f0x_tim.h"
#include "stm32f0xx_tim.h"
#include "stm32f0xx_misc.h"
#include "uart.h"
#include "hard.h"

#include "dmx_transceiver.h"

//--- VARIABLES EXTERNAS ---//
extern volatile unsigned char timer_1seg;
extern volatile unsigned short timer_led_comm;
extern volatile unsigned short wait_ms_var;

// ------- para determinar igrid -------
extern volatile unsigned char igrid_timer;

//--- VARIABLES GLOBALES ---//

volatile unsigned short timer_1000 = 0;



//--- FUNCIONES DEL MODULO ---//
void Update_TIM3_CH1 (unsigned short a)
{
	TIM3->CCR1 = a;
}

void Update_TIM3_CH2 (unsigned short a)
{
	TIM3->CCR2 = a;
}

void Update_TIM3_CH3 (unsigned short a)
{
	TIM3->CCR3 = a;
}

void Update_TIM3_CH4 (unsigned short a)
{
	TIM3->CCR4 = a;
}

void Wait_ms (unsigned short wait)
{
	wait_ms_var = wait;

	while (wait_ms_var);
}

//-------------------------------------------//
// @brief  TIM configure.
// @param  None
// @retval None
//------------------------------------------//
void TIM3_IRQHandler (void)	//1 ms
{
	/*
	Usart_Time_1ms ();

	if (timer_1seg)
	{
		if (timer_1000)
			timer_1000--;
		else
		{
			timer_1seg--;
			timer_1000 = 1000;
		}
	}

	if (timer_led_comm)
		timer_led_comm--;

	if (timer_standby)
		timer_standby--;
	*/
	//bajar flag
	if (TIM3->SR & 0x01)	//bajo el flag
		TIM3->SR = 0x00;
}


void TIM_3_Init (void)
{

	//NVIC_InitTypeDef NVIC_InitStructure;

	if (!RCC_TIM3_CLK)
		RCC_TIM3_CLK_ON;

	//Configuracion del timer.
	TIM3->CR1 = 0x00;		//clk int / 1; upcounting
	TIM3->CR2 = 0x00;		//igual al reset
//	TIM3->CCMR2 = 0x7070;			//CH4 y CH3 output PWM mode 2
	TIM3->CCMR1 = 0x6060;			//CH1 CH2 PWM mode 2
	TIM3->CCMR2 = 0x0000;			//
	TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;	//CH1 enable on pin
	TIM3->ARR = 255;
	//TIM3->ARR = 1023;		//para probar parte baja de placa mosfet (comparar con placa china)
	TIM3->CNT = 0;
	//TIM3->PSC = 0;
	TIM3->PSC = 11;
	//TIM3->EGR = TIM_EGR_UG;

	// Enable timer ver UDIS
	//TIM3->DIER |= TIM_DIER_UIE;
	TIM3->CR1 |= TIM_CR1_CEN;

	//Timer sin Int
	//NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	//NVIC_InitStructure.NVIC_IRQChannelPriority = 5;
	//NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//NVIC_Init(&NVIC_InitStructure);

	//Configuracion Pines
	//Alternate Fuction
	GPIOA->AFR[0] = 0x11000000;	//PA7 -> AF1; PA6 -> AF1
	//GPIOB->AFR[0] = 0x00000011;	//PB1 -> AF1; PB0 -> AF1

}

void TIM_6_Init (void)
{
	if (!RCC_TIM6_CLK)
		RCC_TIM6_CLK_ON;

	//Configuracion del timer.
	TIM6->CR1 = 0x00;		//clk int / 1; upcounting
	TIM6->PSC = 47;			//tick cada 1us
	TIM6->ARR = 0xFFFF;			//para que arranque
	//TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM14_IRQHandler (void)	//100uS
{

	if (TIM14->SR & 0x01)
		//bajar flag
		TIM14->SR = 0x00;
}


void TIM_14_Init (void)
{

	//NVIC_InitTypeDef NVIC_InitStructure;

	if (!RCC_TIM14_CLK)
		RCC_TIM14_CLK_ON;

	/*
	//Configuracion del timer.
	TIM14->ARR = 2000; //10m
	TIM14->CNT = 0;
	TIM14->PSC = 479;
	TIM14->EGR = TIM_EGR_UG;

	// Enable timer ver UDIS
	TIM14->DIER |= TIM_DIER_UIE;
	TIM14->CR1 |= TIM_CR1_CEN;

	NVIC_InitStructure.NVIC_IRQChannel = TIM14_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 5;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	*/

	//Configuracion del timer.
	TIM14->CR1 = 0x00;		//clk int / 1; upcounting; uev
	TIM14->PSC = 47;			//tick cada 1us
	TIM14->ARR = 0xFFFF;			//para que arranque
	TIM14->EGR |= 0x0001;
}

void TIM16_IRQHandler (void)	//es one shoot
{
	SendDMXPacket(PCKT_UPDATE);

	if (TIM16->SR & 0x01)
		//bajar flag
		TIM16->SR = 0x00;
}


void TIM_16_Init (void)
{
	if (!RCC_TIM16_CLK)
		RCC_TIM16_CLK_ON;

	//Configuracion del timer.
	TIM16->ARR = 0;
	TIM16->CNT = 0;
	TIM16->PSC = 47;

	// Enable timer interrupt ver UDIS
	TIM16->DIER |= TIM_DIER_UIE;
	TIM16->CR1 |= TIM_CR1_URS | TIM_CR1_OPM;	//solo int cuando hay overflow y one shot

	NVIC_EnableIRQ(TIM16_IRQn);
	NVIC_SetPriority(TIM16_IRQn, 7);
}

void OneShootTIM16 (unsigned short a)
{
	TIM16->ARR = a;
	TIM16->CR1 |= TIM_CR1_CEN;
}

void TIM17_IRQHandler (void)	//200uS
{
	igrid_timer = 1;

	if (TIM17->SR & 0x01)
		TIM17->SR = 0x00;		//bajar flag
}


void TIM_17_Init (void)
{
	if (!RCC_TIM17_CLK)
		RCC_TIM17_CLK_ON;

	//Configuracion del timer.
	TIM17->ARR = 400;		//200us
	TIM17->CNT = 0;
	TIM17->PSC = 47;

	// Enable timer interrupt ver UDIS
	TIM17->DIER |= TIM_DIER_UIE;
	TIM17->CR1 |= TIM_CR1_URS | TIM_CR1_CEN;	//solo int cuando hay overflow y one shot

	NVIC_EnableIRQ(TIM17_IRQn);
	NVIC_SetPriority(TIM17_IRQn, 8);
}

//--- end of file ---//


