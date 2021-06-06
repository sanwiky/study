/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI
  
  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "bsp_esp8266.h"
#include <string.h>

/*****************  �ж� **********************/
extern uint8_t ucTcpClosedFlag;
/* ����wifiģ�� */
void USART3_IRQHandler(void)
{
	uint8_t ucCh;
	
	/* ���յ������ж� */
	if ( USART_GetITStatus ( macESP8266_USARTx, USART_IT_RXNE ) != RESET )
	{
		ucCh  = USART_ReceiveData( macESP8266_USARTx );
		
		if ( strEsp8266_Fram_Record .InfBit .FramLength < ( RX_BUF_MAX_LEN - 1 ) )                       //Ԥ��1���ֽ�д������
			strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ++ ]  = ucCh;

	}
	 	 
	if ( USART_GetITStatus( macESP8266_USARTx, USART_IT_IDLE ) == SET )                                         //����֡�������
	{
    strEsp8266_Fram_Record .InfBit .FramFinishFlag = 1;
		
		ucCh = USART_ReceiveData( macESP8266_USARTx );                                                              //�������������жϱ�־λ(�ȶ�USART_SR��Ȼ���USART_DR)
	
		ucTcpClosedFlag = strstr ( strEsp8266_Fram_Record .Data_RX_BUF, "CLOSED\r\n" ) ? 1 : 0;
		
  }
}

/* ������Խ��� */
extern unsigned char disdata[18];
extern unsigned char discount;
void UART4_IRQHandler(void)
{
	/* �ȴ������������� */
	if(USART_GetFlagStatus(UART4, USART_FLAG_RXNE) != RESET)
		disdata[discount++] = USART_ReceiveData(UART4);
	
	/* buf���˻ع����ұ�֤��ʼ������������ */
	if(discount == sizeof(disdata))
	{
		if(disdata[discount-1] == 0x0A)
			discount = 0;
		else
			discount--;
	}		
}

/* NBIotģ�� */
void UART5_IRQHandler(void)
{

}

