#ifndef __BEEP_H
#define	__BEEP_H


#include "stm32f10x.h"


/* 定义蜂鸣器连接的GPIO端口, 用户只需要修改下面的代码即可改变控制的蜂鸣器引脚 */
#define BEEP_GPIO_PORT    	GPIOC			              /* GPIO端口 */
#define BEEP_GPIO_CLK 	    RCC_APB2Periph_GPIOC		/* GPIO端口时钟 */
#define BEEP_GPIO_PIN		  GPIO_Pin_0			        /* 连接到蜂鸣器的GPIO */

///* 高电平时，蜂鸣器响 */
//#define ON  1
//#define OFF 0

 

/* 直接操作寄存器的方法控制IO */
#define	digitalHi(p,i)		 {p->BSRR=i;}	 //输出为高电平		
#define digitalLo(p,i)		 {p->BRR=i;}	 //输出低电平
#define digitalToggle(p,i) {p->ODR ^=i;} //输出反转状态

#define BEEP_OFF  BEEP(0)
#define BEEP_ON   BEEP(1)
#define BEEP_TOGGLE       digitalToggle(BEEP_GPIO_PORT, BEEP_GPIO_PIN)

/* 带参宏，可以像内联函数一样使用 */
#define BEEP(a)	if (a)	\
					GPIO_SetBits(BEEP_GPIO_PORT,BEEP_GPIO_PIN);\
					else		\
					GPIO_ResetBits(BEEP_GPIO_PORT,BEEP_GPIO_PIN)

void BEEP_GPIO_Config(void);
					
#endif /* __BEEP_H */
