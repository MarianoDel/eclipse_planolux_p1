/**
  ******************************************************************************
  * @file    Template_2/stm32f0_uart.c
  * @author  Nahuel
  * @version V1.0
  * @date    22-August-2014
  * @brief   UART functions.
  ******************************************************************************
  * @attention
  *
  * Use this functions to configure serial comunication interface (UART).
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "hard.h"
#include "stm32f0xx.h"
#include "stm32f0xx_conf.h"
#include "stm32f0xx_adc.h"
//#include "stm32f0xx_can.h"
//#include "stm32f0xx_cec.h"
//#include "stm32f0xx_comp.h"
//#include "stm32f0xx_crc.h"
//#include "stm32f0xx_crs.h"
//#include "stm32f0xx_dac.h"
//#include "stm32f0xx_dbgmcu.h"
//#include "stm32f0xx_dma.h"
//#include "stm32f0xx_exti.h"
//#include "stm32f0xx_flash.h"
#include "stm32f0xx_gpio.h"
//#include "stm32f0xx_i2c.h"
//#include "stm32f0xx_iwdg.h"
#include "stm32f0xx_misc.h"
//#include "stm32f0xx_pwr.h"
#include "stm32f0xx_rcc.h"
//#include "stm32f0xx_rtc.h"
#include "stm32f0xx_spi.h"
//#include "stm32f0xx_syscfg.h"
#include "stm32f0xx_tim.h"
#include "stm32f0xx_usart.h"
//#include "stm32f0xx_wwdg.h"
#include "system_stm32f0xx.h"
#include "stm32f0xx_it.h"

#include "stm32f0x_uart.h"
#include "dmx_transceiver.h"

#include <string.h>




//--- Private typedef ---//
//--- Private define ---//
//--- Private macro ---//

//#define USE_USARTx_TIMEOUT

#ifdef USE_USARTx_TIMEOUT
#define BUFFRX_TIMEOUT 200 //200ms
#define BUFFTX_TIMEOUT 200 //200ms
#endif


//--- VARIABLES EXTERNAS ---//
//extern volatile unsigned char buffrx_ready;
//extern volatile unsigned char *pbuffrx;

extern volatile unsigned char Packet_Detected_Flag;
extern volatile unsigned char dmx_receive_flag;
extern volatile unsigned short DMX_channel_received;
extern volatile unsigned short DMX_channel_selected;
extern volatile unsigned char DMX_channel_quantity;
extern volatile unsigned char data1[];
//static unsigned char data_back[10];
extern volatile unsigned char data[];


volatile unsigned char * pdmx;

//--- Private variables ---//
//Reception buffer.

//Transmission buffer.

//--- Private function prototypes ---//
//--- Private functions ---//

void Usart_Time_1ms (void)
{
#ifdef USE_USARTx_TIMEOUT
	if (buffrx_timeout > 1)
		buffrx_timeout--; //Se detiene con buffrx_timeout = 1.

	if (bufftx_timeout > 1)
		bufftx_timeout--; //Se detiene con bufftx_timeout = 1.
#endif
}
//-------------------------------------------//
// @brief  UART configure.
// @param  None
// @retval None
//------------------------------------------//
void USARTx_IRQHandler(void)
{
	unsigned short i;
	unsigned char dummy;

	/* USART in mode Receiver --------------------------------------------------*/
	//if (USART_GetITStatus(USARTx, USART_IT_RXNE) == SET)
	if (USART1->ISR & USART_ISR_RXNE)
	{
		//RX DMX
		//data0 = USART_ReceiveData(USART3);
		dummy = USARTx->RDR & 0x0FF;

		if (dmx_receive_flag)
		{
			if (DMX_channel_received == 0)		//empieza paquete
				LED_ON;

			//TODO: aca ver si es DMX o RDM
			if (DMX_channel_received < 511)
			{
				data1[DMX_channel_received] = dummy;
				DMX_channel_received++;
			}
			else
				DMX_channel_received = 0;

			//TODO: revisar canales 510 + 4
			if (DMX_channel_received >= (DMX_channel_selected + DMX_channel_quantity))
			{
				//los paquetes empiezan en 0 pero no lo verifico
				for (i=0; i<DMX_channel_quantity; i++)
				{
					data[i] = data1[(DMX_channel_selected) + i];
				}

				/*
				if ((data[0] < 10) || (data[0] > 240))	//revisa el error de salto de canal
					LED2_ON;
				else
					LED2_OFF;	//trata de encontrar el error de deteccion de trama
				*/

				//--- Reception end ---//
				DMX_channel_received = 0;
				//USARTx_RX_DISA;
				dmx_receive_flag = 0;
				Packet_Detected_Flag = 1;
				LED_OFF;	//termina paquete
			}
		}
		else
			USARTx->RQR |= 0x08;	//hace un flush de los datos sin leerlos
	}

	/* USART in mode Transmitter -------------------------------------------------*/
	//if (USART_GetITStatus(USARTx, USART_IT_TXE) == SET)


	if (USART1->CR1 & USART_CR1_TXEIE)
	{
		if (USART1->ISR & USART_ISR_TXE)
		{
	//		USARTx->CR1 &= ~0x00000088;	//bajo TXEIE bajo TE
			//USART1->CR1 &= ~USART_CR1_TXEIE;
			//USARTx->TDR = 0x00;

			if (pdmx < &data1[512])
			{
				USARTx->TDR = *pdmx;
				pdmx++;
			}
			else
			{
				USART1->CR1 &= ~USART_CR1_TXEIE;
				SendDMXPacket(PCKT_UPDATE);
			}

/*
			switch (transmit_mode)
			{
				case TRANSMIT_DMX:
					//activo interrupt

					//envio start code

					break;

				case TRANSMITING_DMX:
					if (pdmx < &data1[512])

					break;

				case TRANSMIT_RDM:

					break;

				default:
					transmit_mode = TRANSMIT_DMX;
					break;

			}
*/

		}

	}





	//if ((USART_GetITStatus(USARTx, USART_IT_ORE) == SET) || (USART_GetITStatus(USARTx, USART_IT_NE) == SET) || (USART_GetITStatus(USARTx, USART_IT_FE) == SET))
	if ((USART1->ISR & USART_ISR_ORE) || (USART1->ISR & USART_ISR_NE) || (USART1->ISR & USART_ISR_FE))
	{
		USARTx->ICR |= 0x0e;
		dummy = USARTx->RDR;
		//return;
	}
}

void UsartSendDMX (void)
{
	//data1[0] = 0x00;
	pdmx = &data1[0];
	//USART_ITConfig(USARTx, USART_IT_TXE, ENABLE);
	USART1->CR1 |= USART_CR1_TXEIE;
}

void USART_Config(void)
{
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clock */
  RCC_AHBPeriphClockCmd(USARTx_TX_GPIO_CLK | USARTx_RX_GPIO_CLK, ENABLE);

  /* Enable USART clock */
  USARTx_APBPERIPHCLOCK(USARTx_CLK, ENABLE);

  /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig(USARTx_TX_GPIO_PORT, USARTx_TX_SOURCE, USARTx_TX_AF);

  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig(USARTx_RX_GPIO_PORT, USARTx_RX_SOURCE, USARTx_RX_AF);

  /* Configure USART Tx and Rx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = USARTx_TX_PIN;
  GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = USARTx_RX_PIN;
  GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStructure);

  /* USARTx configuration ----------------------------------------------------*/
  /* USARTx configured as follow:
  - BaudRate = 9600 baud
  - Word Length = 8 Bits
  - One Stop Bit
  - No parity
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled
  */
  //USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_BaudRate = 250000;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_2;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USARTx, &USART_InitStructure);

  /* NVIC configuration */
  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USARTx_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);


#ifdef USE_USARTx_TIMEOUT
  buffrx_timeout = 0;
  bufftx_timeout = 0;
#endif

  /* Enable USART */
  USART_Cmd(USARTx, ENABLE);

//  pdmx = &data1[0];
  USART1->CR1 &= ~USART_CR1_TXEIE;	//apago int del transmisor

  //--- Enable receiver interrupt ---//
  USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
}

void USARTSend(unsigned char value)
{
	while ((USARTx->ISR & 0x00000080) == 0);
	USARTx->TDR = value;
}

//--- end of file ---//

