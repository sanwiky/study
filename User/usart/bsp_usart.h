#ifndef __USART_H
#define	__USART_H


#include "stm32f10x.h"
#include <stdio.h>

/** 
  * ���ں궨�壬��ͬ�Ĵ��ڹ��ص����ߺ�IO��һ������ֲʱ��Ҫ�޸��⼸����
	* 1-�޸�����ʱ�ӵĺ꣬uart1���ص�apb2���ߣ�����uart���ص�apb1����
	* 2-�޸�GPIO�ĺ�
  */
	
// ����1-USART1
#define  DEBUG_USARTx                   USART1
#define  DEBUG_USART_CLK                RCC_APB2Periph_USART1
#define  DEBUG_USART_APBxClkCmd         RCC_APB2PeriphClockCmd
#define  DEBUG_USART_BAUDRATE           115200

// USART GPIO ���ź궨��
#define  DEBUG_USART_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define  DEBUG_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
    
#define  DEBUG_USART_TX_GPIO_PORT       GPIOA   
#define  DEBUG_USART_TX_GPIO_PIN        GPIO_Pin_9
#define  DEBUG_USART_RX_GPIO_PORT       GPIOA
#define  DEBUG_USART_RX_GPIO_PIN        GPIO_Pin_10

#define  DEBUG_USART_IRQ                USART1_IRQn
#define  DEBUG_USART_IRQHandler         USART1_IRQHandler


// ����2-USART2
//#define  USARTx                   USART2
#define  USART2_CLK                     RCC_APB1Periph_USART2
#define  USART2_APBxClkCmd              RCC_APB1PeriphClockCmd
#define  USART2_BAUDRATE                9600

// USART GPIO ���ź궨��
#define  USART2_GPIO_CLK                (RCC_APB2Periph_GPIOA)
#define  USART2_GPIO_APBxClkCmd         RCC_APB2PeriphClockCmd
    
#define  USART2_TX_GPIO_PORT            GPIOA   
#define  USART2_TX_GPIO_PIN             GPIO_Pin_2
#define  USART2_RX_GPIO_PORT            GPIOA
#define  USART2_RX_GPIO_PIN             GPIO_Pin_3

#define  USART2_IRQ                     USART2_IRQn
#define  USART2_IRQHandler              USART2_IRQHandler

//// ����3-USART3
////#define  USARTx                   USART3
//#define  USART3_CLK                     RCC_APB1Periph_USART3
//#define  USART3_APBxClkCmd              RCC_APB1PeriphClockCmd
//#define  USART3_BAUDRATE                9600

//// USART GPIO ���ź궨��
//#define  USART3_GPIO_CLK                (RCC_APB2Periph_GPIOB)
//#define  USART3_GPIO_APBxClkCmd         RCC_APB2PeriphClockCmd
//    
//#define  USART3_TX_GPIO_PORT            GPIOB   
//#define  USART3_TX_GPIO_PIN             GPIO_Pin_10
//#define  USART3_RX_GPIO_PORT            GPIOB
//#define  USART3_RX_GPIO_PIN             GPIO_Pin_11

//#define  USART3_IRQ                     USART3_IRQn
//#define  USART3_IRQHandler              USART3_IRQHandler

// ����4-UART4
//#define  USARTx                   UART4
#define  UART4_CLK                     RCC_APB1Periph_UART4
#define  UART4_APBxClkCmd              RCC_APB1PeriphClockCmd
#define  UART4_BAUDRATE                9600

// UART GPIO ���ź궨��
#define  UART4_GPIO_CLK                (RCC_APB2Periph_GPIOC)
#define  UART4_GPIO_APBxClkCmd         RCC_APB2PeriphClockCmd
    
#define  UART4_TX_GPIO_PORT            GPIOC   
#define  UART4_TX_GPIO_PIN             GPIO_Pin_10
#define  UART4_RX_GPIO_PORT            GPIOC
#define  UART4_RX_GPIO_PIN             GPIO_Pin_11

#define  UART4_IRQ                     UART4_IRQn
#define  UART4_IRQHandler              UART4_IRQHandler


// ����5-UART5
//#define  DEBUG_USARTx                   UART5
#define  UART5_CLK                      RCC_APB1Periph_UART5
#define  UART5_APBxClkCmd               RCC_APB1PeriphClockCmd
#define  UART5_BAUDRATE                 115200

// USART GPIO ���ź궨��
#define  UART5_GPIO_CLK                 (RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD)
#define  UART5_GPIO_APBxClkCmd          RCC_APB2PeriphClockCmd
    
#define  UART5_TX_GPIO_PORT             GPIOC   
#define  UART5_TX_GPIO_PIN              GPIO_Pin_12
#define  UART5_RX_GPIO_PORT             GPIOD
#define  UART5_RX_GPIO_PIN              GPIO_Pin_2

#define  UART5_IRQ                      UART5_IRQn
#define  UART5_IRQHandler               UART5_IRQHandler


void DEBUG_USART_Config(void);
void USART2_Config(void);
//void USART3_Config(void);
void UART4_Config(void);
void UART5_Config(void);
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendString( USART_TypeDef * pUSARTx, char *str);
void Usart_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch);

#endif /* __USART_H */
