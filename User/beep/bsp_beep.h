#ifndef __BEEP_H
#define	__BEEP_H


#include "stm32f10x.h"


/* 定义蜂鸣器连接的GPIO端口, 用户只需要修改下面的代码即可改变控制的蜂鸣器引脚 */
#define macBEEP_GPIO_PORT    	GPIOA			              /* GPIO端口 */
#define macBEEP_GPIO_CLK 	    RCC_APB2Periph_GPIOA		/* GPIO端口时钟 */
#define macBEEP_GPIO_PIN		  GPIO_Pin_8			        /* 连接到蜂鸣器的GPIO */

/* 高电平时，蜂鸣器响 */
#define ON  1
#define OFF 0

/* 带参宏，可以像内联函数一样使用 */
#define macBEEP(a)	if (a)	\
					GPIO_SetBits(macBEEP_GPIO_PORT,macBEEP_GPIO_PIN);\
					else		\
					GPIO_ResetBits(macBEEP_GPIO_PORT,macBEEP_GPIO_PIN)

void BEEP_GPIO_Config(void);
					
#endif /* __BEEP_H */
