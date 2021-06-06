/***************************************************************
  * @file    main.c
  * @author  test
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   STM32全系列开发板-LiteOS！
  **************************************************************/

#include <string.h>

 /* LiteOS 头文件 */
#include "los_sys.h"
#include "los_task.ph"
#include "los_queue.h"
#include "los_sem.h"
#include "los_mux.h"
#include "los_swtmr.h"

/* 板级外设头文件 */
#include "bsp_usart.h"
#include "bsp_led.h"
#include "bsp_key.h" 
#include "bsp_beep.h"
#include "bsp_spi_flash.h"
#include "bsp_adc.h"
#include "OLED_I2C.h"
#include "XFS5152.h"
#include "nbiot.h"

/* WIFI */
#include "bsp_esp8266.h"
#include <stdio.h>  
#include <string.h>  
#include <stdbool.h>
#include "OLED_I2C.h"


/********************************** 宏定义 ***************************************/
/* 定义按键事件 */
#define KEY1_EVENT  (0x01 << 0)//设置事件掩码的位0
#define KEY2_EVENT  (0x01 << 1)//设置事件掩码的位1

/* FLASH定义 */
#define  FLASH_WriteAddress     0x00000
#define  FLASH_ReadAddress      FLASH_WriteAddress
#define  FLASH_SectorToErase    FLASH_WriteAddress
#define  SPI_FLASH_ID           0XEF4017    //W25Q64

/* 显示页面 */
#define DIS_PAGE_1         1
#define DIS_PAGE_2         2
#define DIS_PAGE_3         3
#define DIS_PAGE_4         4
#define DIS_PAGE_5         5
#define DIS_PAGE_MAX       DIS_PAGE_5

#define DIS_PAGE_VOICE     0xF0


/****************************** WIFI需要设置的参数********************************/
#define      macUser_ESP8266_ApSsid                       "Tenda_702"          //要连接的热点的名称
#define      macUser_ESP8266_ApPwd                        "4001001199"         //要连接的热点的密钥

#define      macUser_ESP8266_TcpServer_IP                 "192.168.0.113"      //要连接的服务器的 IP
#define      macUser_ESP8266_TcpServer_Port               "8080"               //要连接的服务器的端口



/********************************** 内核对象句柄 *********************************/
/*
 * 信号量，消息队列，事件标志组，软件定时器这些都属于内核的对象，要想使用这些内核
 * 对象，必须先创建，创建成功之后会返回一个相应的句柄。实际上就是一个指针，后续我
 * 们就可以通过这个句柄操作这些内核对象。
 *
 * 内核对象说白了就是一种全局的数据结构，通过这些数据结构我们可以实现任务间的通信，
 * 任务间的事件同步等各种功能。至于这些功能的实现我们是通过调用这些内核对象的函数
 * 来完成的
 * 
 */
 
/* 定义蜂鸣器二值信号量的句柄 */
UINT32 Beer_BinarySem_Handle;

/* 定义OLED刷屏二值信号量的句柄 */
UINT32 Refresh_BinarySem_Handle;

/* 定义FLASH操作互斥量的句柄 */
UINT32 Flash_Mutex_Handle;

/* 定义OLED刷屏定时器1句柄（ID） */
UINT16 Timer1_Handle;

/* 定义FLASH操作事件标志组的控制块 */
EVENT_CB_S Flash_EventGroup_CB;

/* 定义LED指示事件标志组的控制块 */
EVENT_CB_S LED_EventGroup_CB;

/******************************* 全局变量声明 ************************************/
/*
 * 当我们在写应用程序的时候，可能需要用到一些全局变量。
 */

/* 发送缓冲区初始化 */
UINT8 Tx_Buffer[256] = " this is a test for write to spi flash!";
UINT8 Rx_Buffer[256];

/* 定义应用任务句柄 */
UINT32 Run_Task_Handle;
UINT32 Led_Task_Handle;
UINT32 Key_Task_Handle;
UINT32 Beer_Task_Handle;
UINT32 Flash_Task_Handle;
UINT32 OLED_Task_Handle;
UINT32 NbIot_Task_Handle;
UINT32 WIFI_Task_Handle;

/* 函数声明 */
static UINT32 AppTaskCreate(void);
static UINT32 Creat_Run_Task(void);
static UINT32 Creat_Led_Task(void);
static UINT32 Creat_Key_Task(void);
static UINT32 Creat_Beer_Task(void);
static UINT32 Creat_Flash_Task(void);
static UINT32 Creat_OLED_Task(void);
static UINT32 Creat_NbIot_Task(void);
static UINT32 Creat_WIFI_Task();
static void Timer1_Callback(void);

static void Run_Task(void);
static void Led_Task(void);
static void Key_Task(void);
static void Beer_Task(void);
static void Flash_Task(void);
static void OLED_Task(void);
static void NbIot_Task(void);
static void WIFI_Task(void);
static void BSP_Init(void);

extern const unsigned char BMP1[];
extern const unsigned char BMP2[];
extern const unsigned char BMP4_0[];
extern const unsigned char BMP4_1[];
extern const unsigned char BMP4_2[];
extern const unsigned char BMP4_3[];
extern const unsigned char BMP4_4[];
extern const unsigned char BMP5_0[];
extern const unsigned char BMP5_1[];
extern const unsigned char BMP5_2[];
extern const unsigned char BMP5_3[];
extern const unsigned char BMP5_4[];
extern const unsigned char BMP5_5[];
extern const unsigned char BMP_VOICE_0[];
extern const unsigned char BMP_VOICE_1[];
extern const unsigned char BMP_VOICE_2[];
extern const unsigned char BMP_VOICE_3[];
extern const unsigned char BMP_VOICE_4[];
extern const unsigned char BMP_VOICE_5[];

extern uint16_t ADC_ConvertedValue;

/* OLED切换新页面 */
UINT32 newpage;
/* 声音量 */
UINT32 voicesize = 1;
/* 定时器计数 */
UINT32 timercount = 1;
/* 具体数据 */
UINT8 disdata[18];
UINT8 discount = 0;
/* WIFI */
uint8_t ucTcpClosedFlag = 0;
char cStr [ 1500 ] = { 0 };


/***************************************************************
  * @brief  主函数
  * @param  无
  * @retval 无
  * @note   第一步：开发板硬件初始化 
			      第二步：创建APP应用任务
			      第三步：启动LiteOS，开始多任务调度，启动失败则输出错误信息
  **************************************************************/
int main(void)
{	
	UINT32 uwRet = LOS_OK;  //定义一个任务创建的返回值，默认为创建成功
	
	/* 板载相关初始化 */
  BSP_Init();
	
	printf("\r\n\r\n............开始系统初始化............\r\n\r\n");
	
	/* LiteOS 内核初始化 */
	uwRet = LOS_KernelInit();
  if (uwRet != LOS_OK)
  {
		printf("LiteOS 核心初始化失败！失败代码0x%X\r\n",uwRet);
		return LOS_NOK;
  }

	uwRet = AppTaskCreate();
	if (uwRet != LOS_OK)
  {
		printf("AppTaskCreate创建任务失败！失败代码0x%X\r\n",uwRet);
		return LOS_NOK;
  }

  /* 开启LiteOS任务调度 */
  LOS_Start();
	
	//正常情况下不会执行到这里
	while(1);
	
}

/*********************************************************************************
  * @ 函数名  ： Timer1_Callback
  * @ 功能说明： 软件定时器回调函数1
  * @ 参数    ： 传入1个参数，但未使用  
  * @ 返回值  ： 无
  ********************************************************************************/
static void Timer1_Callback(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;
	
	timercount++;
	
	if(!(timercount%20))
		newpage = DIS_PAGE_1;
	
	/* 释放二值信号量，刷新OLED*/
	uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
	if(LOS_OK != uwRet)
	{
		printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
	}
}

/*******************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 任务创建，为了方便管理，所有的任务创建函数都可以放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  *************************************************************/
static UINT32 AppTaskCreate(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;
	
	/* 创建一个软件定时器定时器*/
	uwRet = LOS_SwtmrCreate(500, 					/* 软件定时器的定时时间（ms）*/	
													LOS_SWTMR_MODE_PERIOD, 	/* 软件定时器模式 周期模式 */
													(SWTMR_PROC_FUNC)Timer1_Callback,		/* 软件定时器的回调函数 */
													&Timer1_Handle,			/* 软件定时器的id */
													0);			
	if (uwRet != LOS_OK)
  {
		printf("软件定时器Timer1创建失败！\r\n");
		return uwRet;
  }
	
	/* 启动一个软件定时器定时器*/
	uwRet = LOS_SwtmrStart(Timer1_Handle);
  if (LOS_OK != uwRet)
  {
		printf("start Timer1 failed\r\n");
		return uwRet;
  }
	
	/* 创建一个二值信号量，用于key任务与Beer任务间同步 */
	uwRet = LOS_BinarySemCreate(1,&Beer_BinarySem_Handle);
	if (uwRet != LOS_OK)
	{
		printf("BinarySem创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
	}
	
	/* 创建一个二值信号量，用于OLED刷新 */
	uwRet = LOS_BinarySemCreate(1,&Refresh_BinarySem_Handle);
	if (uwRet != LOS_OK)
	{
		printf("BinarySem创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
	}

	/* 创建一个互斥量，Flash操作为互斥资源 */
	uwRet = LOS_MuxCreate(&Flash_Mutex_Handle);
	if (uwRet != LOS_OK)
	{
		printf("Mutex创建失败！失败代码0x%X\r\n",uwRet);
	}
	
	/* 创建一个事件标志组*/
	uwRet = LOS_EventInit(&Flash_EventGroup_CB);
	if (uwRet != LOS_OK)
  {
		printf("EventGroup_CB事件标志组创建失败！失败代码0x%X\n",uwRet);
  }
	
	/* 创建一个事件标志组*/
	uwRet = LOS_EventInit(&LED_EventGroup_CB);
	if (uwRet != LOS_OK)
  {
		printf("EventGroup_CB事件标志组创建失败！失败代码0x%X\n",uwRet);
  }
		
	/* 创建任务一：系统运行指示 */
	uwRet = Creat_Run_Task();
  if (uwRet != LOS_OK)
  {
		printf("Run_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }

	/* 创建任务二：KEY按键监控 */
	uwRet = Creat_Key_Task();
  if (uwRet != LOS_OK)
  {
		printf("KEY_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务三：LED闪烁，KEY触发 */
	uwRet = Creat_Led_Task();
  if (uwRet != LOS_OK)
  {
		printf("LED_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务四：蜂鸣器响，KEY触发 */
	uwRet = Creat_Beer_Task();
  if (uwRet != LOS_OK)
  {
		printf("Beer_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务五：FLash操作，KEY触发 */
	uwRet = Creat_Flash_Task();
  if (uwRet != LOS_OK)
  {
		printf("Flash_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务六：OLED显示 */
	uwRet = Creat_OLED_Task();
  if (uwRet != LOS_OK)
  {
		printf("OLED_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务七：NBIOT测试 */
	uwRet = Creat_NbIot_Task();
  if (uwRet != LOS_OK)
  {
		printf("OLED_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* 创建任务八：WIFI测试 */
	uwRet = Creat_WIFI_Task();
  if (uwRet != LOS_OK)
  {
		printf("WIFI_Task任务创建失败！失败代码0x%X\r\n",uwRet);
		return uwRet;
  }
	
	return LOS_OK;
}

/******************************************************************
  * @ 函数名  ： Creat_Run_Task
  * @ 功能说明： 创建Run_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_Run_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "Run_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Run_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&Run_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_Key_Task
  * @ 功能说明： 创Key_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_Key_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 3;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "Key_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Key_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&Key_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_Led_Task
  * @ 功能说明： 创建Led_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_Led_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "Led_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Led_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&Led_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_Beer_Task
  * @ 功能说明： 创Beer_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_Beer_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "Beer_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Beer_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&Beer_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_Flash_Task
  * @ 功能说明： 创Flash_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_Flash_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "Flash_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Flash_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&Flash_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_OLED_Task
  * @ 功能说明： 创OLED_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_OLED_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "OLED_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)OLED_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&OLED_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_NBIOT_Task
  * @ 功能说明： 创OLED_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_NbIot_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "NbIot_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)NbIot_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&NbIot_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Creat_WIFI_Task
  * @ 功能说明： 创WIFI_Task任务
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static UINT32 Creat_WIFI_Task()
{
	//定义一个创建任务的返回类型，初始化为创建成功的返回值
	UINT32 uwRet = LOS_OK;			
	
	//定义一个用于创建任务的参数结构体
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* 任务优先级，数值越小，优先级越高 */
	task_init_param.pcName = "WIFI_Task";/* 任务名 */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)WIFI_Task;/* 任务函数入口 */
	task_init_param.uwStackSize = 1024;		/* 堆栈大小 */

	uwRet = LOS_TaskCreate(&WIFI_Task_Handle, &task_init_param);/* 创建任务 */
	return uwRet;
}

/******************************************************************
  * @ 函数名  ： Run_Task
  * @ 功能说明： Run_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void Run_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;		
	
	printf("开启任务一!\r\n");
	
  /* 任务都是一个无限循环，不能返回 */
	while(1)
	{
		LED3_TOGGLE;
		LOS_TaskDelay(500);			
	}
}

/******************************************************************
  * @ 函数名  ： Key_Task
  * @ 功能说明： Key_Task任务实现，触发应用LED\BEER\FLASH发生
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void Key_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;

	printf("开启任务二!\r\n");
	
	/* 任务都是一个无限循环，不能返回 */
	while(1)
	{
		/* K1 被按下 */
		if( KEY_Scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN) == KEY_ON )     		
		{
			/* 释放二值信号量，通知Flash操作 */
			LOS_EventWrite(&Flash_EventGroup_CB,KEY1_EVENT); 
			
			/* 释放二值信号量，通知对应LED亮操作 */
			LOS_EventWrite(&LED_EventGroup_CB,KEY1_EVENT); 

			/* 释放二值信号量，通知蜂鸣器响  */
			uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
			}
		
			/* 循环更新的OLED页面*/
			if(newpage >= DIS_PAGE_MAX)
				newpage = DIS_PAGE_1;
			else
				newpage++;
			
			/* 释放二值信号量，刷新OLED*/
			uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
			}		
		}
		
		/* K2 被按下 */
		if( KEY_Scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN) == KEY_ON ) 
		{
			/* 释放二值信号量，通知Flash操作 */
			LOS_EventWrite(&Flash_EventGroup_CB,KEY2_EVENT);  
			
			/* 释放二值信号量，通知对应LED亮操作 */
			LOS_EventWrite(&LED_EventGroup_CB,KEY2_EVENT); 
			
			/* 释放二值信号量，通知蜂鸣器响  */
			uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
			}
			
			/* 改变音量 */
			if(voicesize >= 5)
				voicesize = 0;
			else
				voicesize++;	

			switch(voicesize)
			{
				case 0://静音
					XFS_FrameInfo(voice_size_0, strlen(voice_size_0));
					DelayMs(10);
					XFS_VoiceChange(voicesize);
					break;
				case 1://音量一
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_1, strlen(voice_size_1));
					break;
				case 2://音量二
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_2, strlen(voice_size_2));
					break;
				case 3://音量三
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_3, strlen(voice_size_3));
					break;
				case 4://音量四
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_4, strlen(voice_size_4));
					break;
				case 5://音量五
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_5, strlen(voice_size_5));
					break;	
        default://音量默认一
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_1, strlen(voice_size_1));
					break;
			}	
			
			/* 显示音乐页面 */
			newpage = DIS_PAGE_VOICE;
			
			/* 释放二值信号量，刷新OLED为VOICE页面*/
			uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
			}		
		}
		
		/* 20ms扫描一次 */
		LOS_TaskDelay(50);       
	}
}

/******************************************************************
  * @ 函数名  ： Led_Task
  * @ 功能说明： Led_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void Led_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT32 *r_queue;
	
	printf("开启任务三!\r\n");
	
  /* 任务都是一个无限循环，不能返回 */
	while(1)
	{
		/* 等待事件标志组 */
		uwRet = LOS_EventRead(	&LED_EventGroup_CB,        	//事件标志组对象
														KEY1_EVENT|KEY2_EVENT,  //等待任务感兴趣的事件
														LOS_WAITMODE_OR,     	//等待两位均被置位
														LOS_WAIT_FOREVER ); 		//无期限等待
					
    if(( uwRet & KEY1_EVENT) == KEY1_EVENT) 
    {
			LOS_EventClear(&LED_EventGroup_CB, ~KEY1_EVENT);	//清除事件标志
			
			LED1_TOGGLE;
		} 
		else if(( uwRet & KEY2_EVENT) == KEY2_EVENT) 
		{
			LOS_EventClear(&LED_EventGroup_CB, ~KEY2_EVENT);	//清除事件标志
			
			LED2_TOGGLE;
		} 		
	}
}

/******************************************************************
  * @ 函数名  ： Beer_Task
  * @ 功能说明： Beer_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void Beer_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;		
	
	printf("开启任务四!\r\n");
	
  /* 任务都是一个无限循环，不能返回 */
	while(1)
	{
		/* 获取二值信号量 xSemaphore,没获取到则一直等待 */
		uwRet = LOS_SemPend( Beer_BinarySem_Handle , 
		                     LOS_WAIT_FOREVER ); 
		if(LOS_OK != uwRet)
		{
			printf("获取信号量失败！错误代码0x%X\r\n",uwRet);	
		}		
		
		macBEEP(ON);
		LOS_TaskDelay(50); 
		macBEEP(OFF);
	}
}

/******************************************************************
  * @ 函数名  ： Flash_Task
  * @ 功能说明： Flash_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void Flash_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT32 *r_queue;
	UINT32 DeviceID = 0;
	UINT32 FlashID = 0;
	
	printf("开启任务五！\r\n");
	
  /* 任务都是一个无限循环，不能返回 */
	while(1)
	{
		/* 等待事件标志组 */
		uwRet = LOS_EventRead(	&Flash_EventGroup_CB,        	//事件标志组对象
														KEY1_EVENT|KEY2_EVENT,  //等待任务感兴趣的事件
														LOS_WAITMODE_OR,     	//等待两位均被置位
														LOS_WAIT_FOREVER ); 		//无期限等待
					
    if(( uwRet & KEY1_EVENT) == KEY1_EVENT) 
    {
			timercount = 1;
			
			LOS_EventClear(&Flash_EventGroup_CB, ~KEY1_EVENT);	//清除事件标志
			
			/* 获取互斥量,没获取到则一直等待*/
			uwRet = LOS_MuxPend(Flash_Mutex_Handle , LOS_WAIT_FOREVER ); 
			if (uwRet != LOS_OK)
				printf("获得互斥操作资源出错！\r\n");
		
			/* Get SPI Flash Device ID */
			DeviceID = SPI_FLASH_ReadDeviceID();
			
			/* Get SPI Flash ID,Check the SPI Flash ID */
			FlashID = SPI_FLASH_ReadID();
			if (FlashID != SPI_FLASH_ID)  /* #define  sFLASH_ID  0XEF4017 */
			{
				/* 释放互斥量	*/
				LOS_MuxPost(Flash_Mutex_Handle);  
				
				printf("FlashID is 0x%X,  Manufacturer Device ID is 0x%X\r\n", FlashID, DeviceID);		
				continue;
			}		
			
			/* 写Flash前，先擦除将要写的Flash */
			SPI_FLASH_SectorErase(FLASH_SectorToErase);	 	
			printf("擦除扇区为：0x%08x \r\n", FLASH_SectorToErase);
			
			/* 将发送缓冲区的数据写到flash中 */
			SPI_FLASH_BufferWrite(Tx_Buffer, FLASH_WriteAddress, sizeof(Tx_Buffer));
			printf("写入的数据为：%s \r\n", Tx_Buffer);
			
			/* 释放互斥量	*/
			LOS_MuxPost(Flash_Mutex_Handle);  

			printf("flash擦写结束！\r\n");
		}
		else if(( uwRet & KEY2_EVENT) == KEY2_EVENT) 
    {
			timercount = 1;
			
			LOS_EventClear(&Flash_EventGroup_CB, ~KEY2_EVENT);	//清除事件标志

			//获取互斥量,没获取到则一直等待
			uwRet = LOS_MuxPend(Flash_Mutex_Handle , LOS_WAIT_FOREVER ); 
			if (uwRet != LOS_OK)
				printf("获得互斥操作资源出错！\r\n");
			
			memset(Rx_Buffer,0,sizeof(Rx_Buffer));
			
			/* 将刚刚写入的数据读出来放到接收缓冲区中 */
			SPI_FLASH_BufferRead(Rx_Buffer, FLASH_ReadAddress, sizeof(Rx_Buffer));
			printf("读出的数据为：%s \r\n", Rx_Buffer);	

			/* 释放互斥量	*/
			LOS_MuxPost(Flash_Mutex_Handle);  	
			
			printf("flash读结束！\r\n");
		}	
	}
}
	
/******************************************************************
  * @ 函数名  ： OLED_Task
  * @ 功能说明： OLED_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void OLED_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT8 i,j;
	UINT32 oldpage;
	UINT8 bmp_index = 0;
	UINT8 voiceflag;
	UINT8 dis[8];
	UINT8 disAD[16];
	UINT16 disnum;
	float ADC_ConvertedValueLocal; 
	
	printf("开启任务六！\r\n");
	
	/* OLED初始化 */
	OLED_Init();

	/* 全屏点亮 */
	OLED_Fill(0xFF);
	DelayMs(50);
	/*全屏灭*/
	OLED_Fill(0x00);
	DelayMs(50);
	
	/*开机显示首页*/
	newpage = DIS_PAGE_1;
	oldpage = DIS_PAGE_1;
	
	while(1)
	{		
		/* 切换新页面显示，先清屏 */
		if(oldpage != newpage)		
		{				
			oldpage = newpage;			
			OLED_CLS();

			/* 增加页面提示音 */
			switch(oldpage)
			{
				case DIS_PAGE_1:
					XFS_FrameInfo(voice_ShanWaiKeJi, strlen(voice_ShanWaiKeJi));
					break;
				case DIS_PAGE_2:
					XFS_FrameInfo(voice_ChooseMode, strlen(voice_ChooseMode));
					break;
				case DIS_PAGE_3:
					XFS_FrameInfo(voice_InitDevice, strlen(voice_InitDevice));
					break;
				case DIS_PAGE_4:
					XFS_FrameInfo(voice_SystermStart, strlen(voice_SystermStart));
					break;
			}
			
			/* 更新画面起始页面 */
			bmp_index = 0;	
		}
		
    switch(oldpage)
		{
			case DIS_PAGE_1:
			{	
				OLED_DrawHalfBMP(0,0,128,8,(unsigned char *)BMP1,0);//测试左半BMP位图显示
				
				/*测试显示中文*/
				for(i=0;i<4;i++)
				{
					OLED_ShowCN(64+i*16,0,i);									
				}

				OLED_ShowStr(64,2,(unsigned char*)"Sanwiky",2);				//测试8*16字符
	
				OLED_ShowStr(64,5,(unsigned char*)"Dis:",1);				//测试8*8字符	
				OLED_ShowStr(88,5,(unsigned char*)"        ",1);	
				
				memset(dis,0,sizeof(dis));
				
			  // 关闭接收中断，避免检查中接收数据缓冲区变化
				USART_ITConfig(UART4, USART_IT_RXNE, DISABLE);	
				
				for(i=0;i<sizeof(disdata);i++)
				{
					if(disdata[i] == 0x0A)
					{		
						i++;
						break;
					}
				}
				
				for(j=0;j<sizeof(disdata) && i<sizeof(disdata);j++,i++)
				{
					if(disdata[i] == 0x0A)
						break;
					dis[j] = disdata[i];
				}				
				
				// 打开接收中断
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);			
				
				if(i == sizeof(disdata))
					OLED_ShowStr(88,5,(unsigned char*)"***mm",1);				//测试8*8字符
				else
					OLED_ShowStr(88,5,dis,1);				//测试8*8字符
				
				i=0;
				while(dis[i] != 'm' && dis[i] != 0)
					i++;
				
				j=0;
				disnum = 0;
				while(i != j)
					disnum = disnum*10 + (dis[j++]-0x30);
				
				if(disnum > 100)
				{
					/* 释放二值信号量，通知蜂鸣器响  */
					uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
					if(LOS_OK != uwRet)
					{
						printf("释放信号量失败！错误代码0x%X\r\n",uwRet);
					}					
				}
				
				/* 显示ADC电压转换值 */
		    ADC_ConvertedValueLocal =(float) ADC_ConvertedValue/4096*3.3; // 读取转换的AD值
				sprintf((char*)disAD, "Vad:%.2fV",ADC_ConvertedValueLocal); 
				OLED_ShowStr(64,7,disAD,1);				//测试8*8字符		
					
				break;
			}
			case DIS_PAGE_2:
			{		
				OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP2);//测试BMP位图显示
				
				break;
			}
			case DIS_PAGE_3:
			{	
				switch(bmp_index)
				{
					case 0:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_0);//测试BMP位图显示
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_1);//测试BMP位图显示
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_2);//测试BMP位图显示
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_3);//测试BMP位图显示
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_4);//测试BMP位图显示
						break;
					default:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_0);//测试BMP位图显示
						break;
				}
				
				if(++bmp_index >= 5)
					bmp_index = 0;
				
				break;
			}
			case DIS_PAGE_4:
			{
				switch(bmp_index)
				{
					case 0:
						OLED_ShowStr(24,4,(unsigned char*)"I",1);				//测试8*16字符
						break;
					case 1:
						OLED_ShowStr(24,4,(unsigned char*)"I L",1);				//测试8*16字符
						break;
					case 2:
						OLED_ShowStr(24,4,(unsigned char*)"I LO",1);				//测试8*16字符
						break;
					case 3:
						OLED_ShowStr(24,4,(unsigned char*)"I LOV",1);				//测试8*16字符
						break;
					case 4:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE",1);				//测试8*16字符
						break;
					case 5:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE Y",1);				//测试8*16字符
						break;
					case 6:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YO",1);				//测试8*16字符
						break;
					case 7:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU",1);				//测试8*16字符
						break;
					case 8:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//测试8*16字符
						break;
					case 9:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//测试8*16字符
						break;
					default:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//测试8*16字符
						break;
				}
				
				if((bmp_index++) >= 9)
				{
					OLED_CLS();
					bmp_index = 0;
				}
				
				break;
			}
			case DIS_PAGE_5:
			{
				switch(bmp_index)
				{
					case 0:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_0);//测试BMP位图显示
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_1);//测试BMP位图显示
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_2);//测试BMP位图显示
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_3);//测试BMP位图显示
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_4);//测试BMP位图显示
						break;
					case 5:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_5);//测试BMP位图显示
						break;
					default:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_0);//测试BMP位图显示
						break;
				}
				
				if(++bmp_index >= 6)
					bmp_index = 0;

				break;
			}
			case DIS_PAGE_VOICE:
			{
				switch(voicesize)
				{
					case 0:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_0);//测试BMP位图显示
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_1);//测试BMP位图显示
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_2);//测试BMP位图显示
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_3);//测试BMP位图显示
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_4);//测试BMP位图显示
						break;
					case 5:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_5);//测试BMP位图显示
						break;
				}
			}
			default:
				break;
		}
		
		/* 获取二值信号量 xSemaphore,没获取到则一直等待 */
		uwRet = LOS_SemPend( Refresh_BinarySem_Handle , 
		                     LOS_WAIT_FOREVER ); 
		if(LOS_OK != uwRet)
		{
			printf("获取信号量失败！错误代码0x%X\r\n",uwRet);	
		}	
	}
}

/******************************************************************
  * @ 函数名  ： NbIot_Task
  * @ 功能说明： NbIot_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void NbIot_Task(void)
{
	/* 定义一个返回类型变量，初始化为LOS_OK */
	UINT32 uwRet = LOS_OK;

	while(1)
	{
		DelayMs(100);
	}
}

/******************************************************************
  * @ 函数名  ： WIFI_Task
  * @ 功能说明： WIFI_Task任务实现
  * @ 参数    ： NULL 
  * @ 返回值  ： NULL
  *****************************************************************/
static void WIFI_Task(void)
{
	uint8_t ucStatus;
	char * pRecStr;
	char cStr [ 100 ] = { 0 };
		
  printf ( "\r\n正在配置 ESP8266 ......\r\n" );
	
	/* 使能芯片 */
	macESP8266_CH_ENABLE();
	
	/* AT指令测试网络连接 */
	ESP8266_AT_Test ();
	
	/* 修改模式 */
	ESP8266_Net_Mode_Choose ( STA );

	/* 连接wifi节点 */
  while ( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );	
	
	/* 单路连接模式 */
	ESP8266_Enable_MultipleId ( DISABLE );
	
	/* TCP网络连接目标服务器 */
	while ( !	ESP8266_Link_Server ( enumTCP, macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, Single_ID_0 ) );
	
	/* 设置透传模式 */
	while ( ! ESP8266_UnvarnishSend () );
	
	printf ( "\r\n配置 ESP8266 完毕\r\n" );
	
	while ( 1 )
	{		
		sprintf ( cStr,
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n\
		ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n" );
		
		/* 透传数据 */
		ESP8266_SendString ( ENABLE, cStr, 0, Single_ID_0 );               //发送数据
		
		DelayMs ( 30 );
		
		/* 检测是否失去连接 */
		if ( ucTcpClosedFlag )                                             
		{
			/* 退出透传模式，进入指令模式 */
			ESP8266_ExitUnvarnishSend ();                                    
			
			/* 获取连接状态 */
			do ucStatus = ESP8266_Get_LinkStatus (); 		
			while ( ! ucStatus );
			
			/* 确认失去连接后重连 */
			if ( ucStatus == 4 )                                             
			{
				printf ( "\r\n正在重连热点和服务器 ......\r\n" );
				
				/* 连接wifi节点 */
				while ( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );
				
				/* TCP网络连接目标服务器 */
				while ( !	ESP8266_Link_Server ( enumTCP, macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, Single_ID_0 ) );
				
				printf ( "\r\n重连热点和服务器成功\r\n" );

			}
			
			/* 设置透传模式 */
			while ( ! ESP8266_UnvarnishSend () );		
			
		}
		
		/* wifi中断接收数据 */
		if( strEsp8266_Fram_Record .InfBit .FramFinishFlag )
		{
			strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ] = '\0';
			printf ("\r\n%s\r\n", strEsp8266_Fram_Record .Data_RX_BUF);
			
			strEsp8266_Fram_Record .InfBit .FramLength = 0;
			strEsp8266_Fram_Record .InfBit .FramFinishFlag = 0;
		}
		
//		/* wifi接收数据 */
//		pRecStr = ESP8266_ReceiveString ( ENABLE );
//		printf ("\r\n%s\r\n", pRecStr);
//		
//		DelayMs ( 20 );
	}
}

/*******************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  ******************************************************************/
static void BSP_Init(void)
{
	/*
	 * STM32中断优先级分组为4，即4bit都用来表示抢占优先级，范围为：0~15
	 * 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断，
	 * 都统一用这个优先级分组，千万不要再分组，切忌。
	 */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	/* LED 初始化 */
	LED_GPIO_Config();

	/* 串口初始化	*/
	DEBUG_USART_Config();// 调试串口输出
	USART2_Config();// 语音串口输出
	//USART3_Config();// 
  ESP8266_Init (); // WIFI无线网络
	UART4_Config();// 测距串口输入
	UART5_Config();// NBIOT通讯模块

	/* 按键初始化	*/
	KEY_GPIO_Config();
	
	/* 蜂鸣器初始化	*/
	BEEP_GPIO_Config();
	
	/* SPI串行flash */
	SPI_FLASH_Init();

	/* IIC初始化OLED */
	I2C_Configuration();

	/* 语音模块初始化 */
	XFS_VoiceInit();
	
	/* ADC初始化 */
	ADCx_Init();
}


/*********************************************END OF FILE**********************/

