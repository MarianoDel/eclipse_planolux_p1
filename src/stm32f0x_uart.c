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
volatile unsigned short rdm_bytes_left = 0;

//Reception buffer.

//Transmission buffer.

//--- Private function prototypes ---//
//--- Private functions ---//


//-------------------------------------------//
// @brief  UART configure.
// @param  None
// @retval None
//------------------------------------------//
void USART1_IRQHandler(void)
{
	unsigned short i;
	unsigned char dummy;

	/* USART in mode Receiver --------------------------------------------------*/
	if (USART1->ISR & USART_ISR_RXNE)
	{
		//RX DMX
		dummy = USART1->RDR & 0x0FF;

		if (dmx_receive_flag != PCKT_NOT_READY)
		{
			if (DMX_channel_received == 0)		//empieza paquete me fijo si es DMX o RDM
			{
				LED_ON;
				if (dummy == 0xCC)	//es RDM
				{
					dmx_receive_flag = PCKT_RDM;
					rdm_bytes_left = 0;
				}
				else if (dummy == 0x00)	//es DMX
				{
					dmx_receive_flag = PCKT_DMX;
				}
				else
				{
					LED_OFF;
					return;		//no se que es vuelvo
				}
				data1[0] = dummy;
				DMX_channel_received++;
			}
			else
			{
				//estoy recibiendo un paquete, segun cual sea me fijo el final
				if (dmx_receive_flag == PCKT_DMX)
				{
					if (DMX_channel_received < 512)
					{
						data1[DMX_channel_received] = dummy;
						DMX_channel_received++;

						if (DMX_channel_received >= (DMX_channel_selected + DMX_channel_quantity))
						{
							//en data[0] siempre copio el ch0, depues los elegidos
							data[0] = data1[0];
							for (i=0; i<DMX_channel_quantity; i++)
							{
								data[i+1] = data1[(DMX_channel_selected) + i];
							}
							//--- Reception end ---//
							DMX_channel_received = 0;
							dmx_receive_flag = PCKT_NOT_READY;
							Packet_Detected_Flag = 1;
							LED_OFF;	//termina paquete
						}
					}
					else
					{
						//debe ser algun error
						DMX_channel_received = 0;
						dmx_receive_flag = PCKT_NOT_READY;
						LED_OFF;	//termina paquete
					}
				}	//fin if PCKT_DMX

				if (dmx_receive_flag == PCKT_RDM)	//estoy recibiendo paquete RDM
				{
					if (DMX_channel_received == 1)		//el segundo byte es el largo de paquete
					{
						rdm_bytes_left = dummy;
						data1[DMX_channel_received] = dummy;
						DMX_channel_received++;
					}
					else if (DMX_channel_received < rdm_bytes_left)	//bytes sucesivos
					{
						data1[DMX_channel_received] = dummy;
						DMX_channel_received++;
					}
					else	//termina paquete RDM
					{
						for (i = 0; i < rdm_bytes_left; i++)	//backup info
						{
							data[i] = data1[i];
						}
						//--- Reception end ---//
						DMX_channel_received = 0;
						dmx_receive_flag = PCKT_NOT_READY;
						Packet_Detected_Flag = 1;
						LED_OFF;	//termina paquete
					}
				}	//fin if PCKT_RDM

			}	//fin else dmx_channel_received
		}
		else
			USART1->RQR |= 0x08;	//hace un flush de los datos sin leerlos
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
				USART1->TDR = *pdmx;
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

	if ((USART1->ISR & USART_ISR_ORE) || (USART1->ISR & USART_ISR_NE) || (USART1->ISR & USART_ISR_FE))
	{
		USART1->ICR |= 0x0e;
		dummy = USART1->RDR;
	}
}

void UsartSendDMX (void)
{
	//data1[0] = 0x00;
	pdmx = &data1[0];
	//USART_ITConfig(USARTx, USART_IT_TXE, ENABLE);
	USART1->CR1 |= USART_CR1_TXEIE;
}

void USART1Config(void)
{
	if (!USART1_CLK)
		USART1_CLK_ON;

	GPIOA->AFR[1] |= 0x0000110;	//PA10 -> AF1 PA9 -> AF1

	USART1->BRR = USART_250000;
	USART1->CR2 |= USART_CR2_STOP_1;	//2 bits stop
	//USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
//	USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_UE;	//SIN TX
	USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;	//para pruebas TX

	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn, 5);
}


void USARTSend(unsigned char value)
{
	while ((USART1->ISR & 0x00000080) == 0);
	USART1->TDR = value;
}

//--- end of file ---//

