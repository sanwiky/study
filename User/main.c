/***************************************************************
  * @file    main.c
  * @author  test
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   STM32ȫϵ�п�����-LiteOS��
  **************************************************************/

#include <string.h>

 /* LiteOS ͷ�ļ� */
#include "los_sys.h"
#include "los_task.ph"
#include "los_queue.h"
#include "los_sem.h"
#include "los_mux.h"
#include "los_swtmr.h"

/* �弶����ͷ�ļ� */
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


/********************************** �궨�� ***************************************/
/* ���尴���¼� */
#define KEY1_EVENT  (0x01 << 0)//�����¼������λ0
#define KEY2_EVENT  (0x01 << 1)//�����¼������λ1

/* FLASH���� */
#define  FLASH_WriteAddress     0x00000
#define  FLASH_ReadAddress      FLASH_WriteAddress
#define  FLASH_SectorToErase    FLASH_WriteAddress
#define  SPI_FLASH_ID           0XEF4017    //W25Q64

/* ��ʾҳ�� */
#define DIS_PAGE_1         1
#define DIS_PAGE_2         2
#define DIS_PAGE_3         3
#define DIS_PAGE_4         4
#define DIS_PAGE_5         5
#define DIS_PAGE_MAX       DIS_PAGE_5

#define DIS_PAGE_VOICE     0xF0


/****************************** WIFI��Ҫ���õĲ���********************************/
#define      macUser_ESP8266_ApSsid                       "Tenda_702"          //Ҫ���ӵ��ȵ������
#define      macUser_ESP8266_ApPwd                        "4001001199"         //Ҫ���ӵ��ȵ����Կ

#define      macUser_ESP8266_TcpServer_IP                 "192.168.0.113"      //Ҫ���ӵķ������� IP
#define      macUser_ESP8266_TcpServer_Port               "8080"               //Ҫ���ӵķ������Ķ˿�



/********************************** �ں˶����� *********************************/
/*
 * �ź�������Ϣ���У��¼���־�飬�����ʱ����Щ�������ں˵Ķ���Ҫ��ʹ����Щ�ں�
 * ���󣬱����ȴ����������ɹ�֮��᷵��һ����Ӧ�ľ����ʵ���Ͼ���һ��ָ�룬������
 * �ǾͿ���ͨ��������������Щ�ں˶���
 *
 * �ں˶���˵���˾���һ��ȫ�ֵ����ݽṹ��ͨ����Щ���ݽṹ���ǿ���ʵ��������ͨ�ţ�
 * �������¼�ͬ���ȸ��ֹ��ܡ�������Щ���ܵ�ʵ��������ͨ��������Щ�ں˶���ĺ���
 * ����ɵ�
 * 
 */
 
/* �����������ֵ�ź����ľ�� */
UINT32 Beer_BinarySem_Handle;

/* ����OLEDˢ����ֵ�ź����ľ�� */
UINT32 Refresh_BinarySem_Handle;

/* ����FLASH�����������ľ�� */
UINT32 Flash_Mutex_Handle;

/* ����OLEDˢ����ʱ��1�����ID�� */
UINT16 Timer1_Handle;

/* ����FLASH�����¼���־��Ŀ��ƿ� */
EVENT_CB_S Flash_EventGroup_CB;

/* ����LEDָʾ�¼���־��Ŀ��ƿ� */
EVENT_CB_S LED_EventGroup_CB;

/******************************* ȫ�ֱ������� ************************************/
/*
 * ��������дӦ�ó����ʱ�򣬿�����Ҫ�õ�һЩȫ�ֱ�����
 */

/* ���ͻ�������ʼ�� */
UINT8 Tx_Buffer[256] = " this is a test for write to spi flash!";
UINT8 Rx_Buffer[256];

/* ����Ӧ�������� */
UINT32 Run_Task_Handle;
UINT32 Led_Task_Handle;
UINT32 Key_Task_Handle;
UINT32 Beer_Task_Handle;
UINT32 Flash_Task_Handle;
UINT32 OLED_Task_Handle;
UINT32 NbIot_Task_Handle;
UINT32 WIFI_Task_Handle;

/* �������� */
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

/* OLED�л���ҳ�� */
UINT32 newpage;
/* ������ */
UINT32 voicesize = 1;
/* ��ʱ������ */
UINT32 timercount = 1;
/* �������� */
UINT8 disdata[18];
UINT8 discount = 0;
/* WIFI */
uint8_t ucTcpClosedFlag = 0;
char cStr [ 1500 ] = { 0 };


/***************************************************************
  * @brief  ������
  * @param  ��
  * @retval ��
  * @note   ��һ����������Ӳ����ʼ�� 
			      �ڶ���������APPӦ������
			      ������������LiteOS����ʼ��������ȣ�����ʧ�������������Ϣ
  **************************************************************/
int main(void)
{	
	UINT32 uwRet = LOS_OK;  //����һ�����񴴽��ķ���ֵ��Ĭ��Ϊ�����ɹ�
	
	/* ������س�ʼ�� */
  BSP_Init();
	
	printf("\r\n\r\n............��ʼϵͳ��ʼ��............\r\n\r\n");
	
	/* LiteOS �ں˳�ʼ�� */
	uwRet = LOS_KernelInit();
  if (uwRet != LOS_OK)
  {
		printf("LiteOS ���ĳ�ʼ��ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return LOS_NOK;
  }

	uwRet = AppTaskCreate();
	if (uwRet != LOS_OK)
  {
		printf("AppTaskCreate��������ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return LOS_NOK;
  }

  /* ����LiteOS������� */
  LOS_Start();
	
	//��������²���ִ�е�����
	while(1);
	
}

/*********************************************************************************
  * @ ������  �� Timer1_Callback
  * @ ����˵���� �����ʱ���ص�����1
  * @ ����    �� ����1����������δʹ��  
  * @ ����ֵ  �� ��
  ********************************************************************************/
static void Timer1_Callback(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;
	
	timercount++;
	
	if(!(timercount%20))
		newpage = DIS_PAGE_1;
	
	/* �ͷŶ�ֵ�ź�����ˢ��OLED*/
	uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
	if(LOS_OK != uwRet)
	{
		printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
	}
}

/*******************************************************************
  * @ ������  �� AppTaskCreate
  * @ ����˵���� ���񴴽���Ϊ�˷���������е����񴴽����������Է��������������
  * @ ����    �� ��  
  * @ ����ֵ  �� ��
  *************************************************************/
static UINT32 AppTaskCreate(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;
	
	/* ����һ�������ʱ����ʱ��*/
	uwRet = LOS_SwtmrCreate(500, 					/* �����ʱ���Ķ�ʱʱ�䣨ms��*/	
													LOS_SWTMR_MODE_PERIOD, 	/* �����ʱ��ģʽ ����ģʽ */
													(SWTMR_PROC_FUNC)Timer1_Callback,		/* �����ʱ���Ļص����� */
													&Timer1_Handle,			/* �����ʱ����id */
													0);			
	if (uwRet != LOS_OK)
  {
		printf("�����ʱ��Timer1����ʧ�ܣ�\r\n");
		return uwRet;
  }
	
	/* ����һ�������ʱ����ʱ��*/
	uwRet = LOS_SwtmrStart(Timer1_Handle);
  if (LOS_OK != uwRet)
  {
		printf("start Timer1 failed\r\n");
		return uwRet;
  }
	
	/* ����һ����ֵ�ź���������key������Beer�����ͬ�� */
	uwRet = LOS_BinarySemCreate(1,&Beer_BinarySem_Handle);
	if (uwRet != LOS_OK)
	{
		printf("BinarySem����ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
	}
	
	/* ����һ����ֵ�ź���������OLEDˢ�� */
	uwRet = LOS_BinarySemCreate(1,&Refresh_BinarySem_Handle);
	if (uwRet != LOS_OK)
	{
		printf("BinarySem����ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
	}

	/* ����һ����������Flash����Ϊ������Դ */
	uwRet = LOS_MuxCreate(&Flash_Mutex_Handle);
	if (uwRet != LOS_OK)
	{
		printf("Mutex����ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
	}
	
	/* ����һ���¼���־��*/
	uwRet = LOS_EventInit(&Flash_EventGroup_CB);
	if (uwRet != LOS_OK)
  {
		printf("EventGroup_CB�¼���־�鴴��ʧ�ܣ�ʧ�ܴ���0x%X\n",uwRet);
  }
	
	/* ����һ���¼���־��*/
	uwRet = LOS_EventInit(&LED_EventGroup_CB);
	if (uwRet != LOS_OK)
  {
		printf("EventGroup_CB�¼���־�鴴��ʧ�ܣ�ʧ�ܴ���0x%X\n",uwRet);
  }
		
	/* ��������һ��ϵͳ����ָʾ */
	uwRet = Creat_Run_Task();
  if (uwRet != LOS_OK)
  {
		printf("Run_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }

	/* �����������KEY������� */
	uwRet = Creat_Key_Task();
  if (uwRet != LOS_OK)
  {
		printf("KEY_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ������������LED��˸��KEY���� */
	uwRet = Creat_Led_Task();
  if (uwRet != LOS_OK)
  {
		printf("LED_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ���������ģ��������죬KEY���� */
	uwRet = Creat_Beer_Task();
  if (uwRet != LOS_OK)
  {
		printf("Beer_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ���������壺FLash������KEY���� */
	uwRet = Creat_Flash_Task();
  if (uwRet != LOS_OK)
  {
		printf("Flash_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ������������OLED��ʾ */
	uwRet = Creat_OLED_Task();
  if (uwRet != LOS_OK)
  {
		printf("OLED_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ���������ߣ�NBIOT���� */
	uwRet = Creat_NbIot_Task();
  if (uwRet != LOS_OK)
  {
		printf("OLED_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	/* ��������ˣ�WIFI���� */
	uwRet = Creat_WIFI_Task();
  if (uwRet != LOS_OK)
  {
		printf("WIFI_Task���񴴽�ʧ�ܣ�ʧ�ܴ���0x%X\r\n",uwRet);
		return uwRet;
  }
	
	return LOS_OK;
}

/******************************************************************
  * @ ������  �� Creat_Run_Task
  * @ ����˵���� ����Run_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_Run_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "Run_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Run_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&Run_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_Key_Task
  * @ ����˵���� ��Key_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_Key_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 3;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "Key_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Key_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&Key_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_Led_Task
  * @ ����˵���� ����Led_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_Led_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "Led_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Led_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&Led_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_Beer_Task
  * @ ����˵���� ��Beer_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_Beer_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "Beer_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Beer_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&Beer_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_Flash_Task
  * @ ����˵���� ��Flash_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_Flash_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "Flash_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)Flash_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&Flash_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_OLED_Task
  * @ ����˵���� ��OLED_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_OLED_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "OLED_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)OLED_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&OLED_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_NBIOT_Task
  * @ ����˵���� ��OLED_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_NbIot_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "NbIot_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)NbIot_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&NbIot_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Creat_WIFI_Task
  * @ ����˵���� ��WIFI_Task����
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static UINT32 Creat_WIFI_Task()
{
	//����һ����������ķ������ͣ���ʼ��Ϊ�����ɹ��ķ���ֵ
	UINT32 uwRet = LOS_OK;			
	
	//����һ�����ڴ�������Ĳ����ṹ��
	TSK_INIT_PARAM_S task_init_param;	

	task_init_param.usTaskPrio = 4;	/* �������ȼ�����ֵԽС�����ȼ�Խ�� */
	task_init_param.pcName = "WIFI_Task";/* ������ */
	task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)WIFI_Task;/* ��������� */
	task_init_param.uwStackSize = 1024;		/* ��ջ��С */

	uwRet = LOS_TaskCreate(&WIFI_Task_Handle, &task_init_param);/* �������� */
	return uwRet;
}

/******************************************************************
  * @ ������  �� Run_Task
  * @ ����˵���� Run_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void Run_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;		
	
	printf("��������һ!\r\n");
	
  /* ������һ������ѭ�������ܷ��� */
	while(1)
	{
		LED3_TOGGLE;
		LOS_TaskDelay(500);			
	}
}

/******************************************************************
  * @ ������  �� Key_Task
  * @ ����˵���� Key_Task����ʵ�֣�����Ӧ��LED\BEER\FLASH����
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void Key_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;

	printf("���������!\r\n");
	
	/* ������һ������ѭ�������ܷ��� */
	while(1)
	{
		/* K1 ������ */
		if( KEY_Scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN) == KEY_ON )     		
		{
			/* �ͷŶ�ֵ�ź�����֪ͨFlash���� */
			LOS_EventWrite(&Flash_EventGroup_CB,KEY1_EVENT); 
			
			/* �ͷŶ�ֵ�ź�����֪ͨ��ӦLED������ */
			LOS_EventWrite(&LED_EventGroup_CB,KEY1_EVENT); 

			/* �ͷŶ�ֵ�ź�����֪ͨ��������  */
			uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
			}
		
			/* ѭ�����µ�OLEDҳ��*/
			if(newpage >= DIS_PAGE_MAX)
				newpage = DIS_PAGE_1;
			else
				newpage++;
			
			/* �ͷŶ�ֵ�ź�����ˢ��OLED*/
			uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
			}		
		}
		
		/* K2 ������ */
		if( KEY_Scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN) == KEY_ON ) 
		{
			/* �ͷŶ�ֵ�ź�����֪ͨFlash���� */
			LOS_EventWrite(&Flash_EventGroup_CB,KEY2_EVENT);  
			
			/* �ͷŶ�ֵ�ź�����֪ͨ��ӦLED������ */
			LOS_EventWrite(&LED_EventGroup_CB,KEY2_EVENT); 
			
			/* �ͷŶ�ֵ�ź�����֪ͨ��������  */
			uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
			}
			
			/* �ı����� */
			if(voicesize >= 5)
				voicesize = 0;
			else
				voicesize++;	

			switch(voicesize)
			{
				case 0://����
					XFS_FrameInfo(voice_size_0, strlen(voice_size_0));
					DelayMs(10);
					XFS_VoiceChange(voicesize);
					break;
				case 1://����һ
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_1, strlen(voice_size_1));
					break;
				case 2://������
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_2, strlen(voice_size_2));
					break;
				case 3://������
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_3, strlen(voice_size_3));
					break;
				case 4://������
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_4, strlen(voice_size_4));
					break;
				case 5://������
				  XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_5, strlen(voice_size_5));
					break;	
        default://����Ĭ��һ
					XFS_VoiceChange(voicesize);
					DelayMs(10);
					XFS_FrameInfo(voice_size_1, strlen(voice_size_1));
					break;
			}	
			
			/* ��ʾ����ҳ�� */
			newpage = DIS_PAGE_VOICE;
			
			/* �ͷŶ�ֵ�ź�����ˢ��OLEDΪVOICEҳ��*/
			uwRet = LOS_SemPost( Refresh_BinarySem_Handle ); 
			if(LOS_OK != uwRet)
			{
				printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
			}		
		}
		
		/* 20msɨ��һ�� */
		LOS_TaskDelay(50);       
	}
}

/******************************************************************
  * @ ������  �� Led_Task
  * @ ����˵���� Led_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void Led_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT32 *r_queue;
	
	printf("����������!\r\n");
	
  /* ������һ������ѭ�������ܷ��� */
	while(1)
	{
		/* �ȴ��¼���־�� */
		uwRet = LOS_EventRead(	&LED_EventGroup_CB,        	//�¼���־�����
														KEY1_EVENT|KEY2_EVENT,  //�ȴ��������Ȥ���¼�
														LOS_WAITMODE_OR,     	//�ȴ���λ������λ
														LOS_WAIT_FOREVER ); 		//�����޵ȴ�
					
    if(( uwRet & KEY1_EVENT) == KEY1_EVENT) 
    {
			LOS_EventClear(&LED_EventGroup_CB, ~KEY1_EVENT);	//����¼���־
			
			LED1_TOGGLE;
		} 
		else if(( uwRet & KEY2_EVENT) == KEY2_EVENT) 
		{
			LOS_EventClear(&LED_EventGroup_CB, ~KEY2_EVENT);	//����¼���־
			
			LED2_TOGGLE;
		} 		
	}
}

/******************************************************************
  * @ ������  �� Beer_Task
  * @ ����˵���� Beer_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void Beer_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;		
	
	printf("����������!\r\n");
	
  /* ������һ������ѭ�������ܷ��� */
	while(1)
	{
		/* ��ȡ��ֵ�ź��� xSemaphore,û��ȡ����һֱ�ȴ� */
		uwRet = LOS_SemPend( Beer_BinarySem_Handle , 
		                     LOS_WAIT_FOREVER ); 
		if(LOS_OK != uwRet)
		{
			printf("��ȡ�ź���ʧ�ܣ��������0x%X\r\n",uwRet);	
		}		
		
		macBEEP(ON);
		LOS_TaskDelay(50); 
		macBEEP(OFF);
	}
}

/******************************************************************
  * @ ������  �� Flash_Task
  * @ ����˵���� Flash_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void Flash_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT32 *r_queue;
	UINT32 DeviceID = 0;
	UINT32 FlashID = 0;
	
	printf("���������壡\r\n");
	
  /* ������һ������ѭ�������ܷ��� */
	while(1)
	{
		/* �ȴ��¼���־�� */
		uwRet = LOS_EventRead(	&Flash_EventGroup_CB,        	//�¼���־�����
														KEY1_EVENT|KEY2_EVENT,  //�ȴ��������Ȥ���¼�
														LOS_WAITMODE_OR,     	//�ȴ���λ������λ
														LOS_WAIT_FOREVER ); 		//�����޵ȴ�
					
    if(( uwRet & KEY1_EVENT) == KEY1_EVENT) 
    {
			timercount = 1;
			
			LOS_EventClear(&Flash_EventGroup_CB, ~KEY1_EVENT);	//����¼���־
			
			/* ��ȡ������,û��ȡ����һֱ�ȴ�*/
			uwRet = LOS_MuxPend(Flash_Mutex_Handle , LOS_WAIT_FOREVER ); 
			if (uwRet != LOS_OK)
				printf("��û��������Դ����\r\n");
		
			/* Get SPI Flash Device ID */
			DeviceID = SPI_FLASH_ReadDeviceID();
			
			/* Get SPI Flash ID,Check the SPI Flash ID */
			FlashID = SPI_FLASH_ReadID();
			if (FlashID != SPI_FLASH_ID)  /* #define  sFLASH_ID  0XEF4017 */
			{
				/* �ͷŻ�����	*/
				LOS_MuxPost(Flash_Mutex_Handle);  
				
				printf("FlashID is 0x%X,  Manufacturer Device ID is 0x%X\r\n", FlashID, DeviceID);		
				continue;
			}		
			
			/* дFlashǰ���Ȳ�����Ҫд��Flash */
			SPI_FLASH_SectorErase(FLASH_SectorToErase);	 	
			printf("��������Ϊ��0x%08x \r\n", FLASH_SectorToErase);
			
			/* �����ͻ�����������д��flash�� */
			SPI_FLASH_BufferWrite(Tx_Buffer, FLASH_WriteAddress, sizeof(Tx_Buffer));
			printf("д�������Ϊ��%s \r\n", Tx_Buffer);
			
			/* �ͷŻ�����	*/
			LOS_MuxPost(Flash_Mutex_Handle);  

			printf("flash��д������\r\n");
		}
		else if(( uwRet & KEY2_EVENT) == KEY2_EVENT) 
    {
			timercount = 1;
			
			LOS_EventClear(&Flash_EventGroup_CB, ~KEY2_EVENT);	//����¼���־

			//��ȡ������,û��ȡ����һֱ�ȴ�
			uwRet = LOS_MuxPend(Flash_Mutex_Handle , LOS_WAIT_FOREVER ); 
			if (uwRet != LOS_OK)
				printf("��û��������Դ����\r\n");
			
			memset(Rx_Buffer,0,sizeof(Rx_Buffer));
			
			/* ���ո�д������ݶ������ŵ����ջ������� */
			SPI_FLASH_BufferRead(Rx_Buffer, FLASH_ReadAddress, sizeof(Rx_Buffer));
			printf("����������Ϊ��%s \r\n", Rx_Buffer);	

			/* �ͷŻ�����	*/
			LOS_MuxPost(Flash_Mutex_Handle);  	
			
			printf("flash��������\r\n");
		}	
	}
}
	
/******************************************************************
  * @ ������  �� OLED_Task
  * @ ����˵���� OLED_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void OLED_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;		
	UINT8 i,j;
	UINT32 oldpage;
	UINT8 bmp_index = 0;
	UINT8 voiceflag;
	UINT8 dis[8];
	UINT8 disAD[16];
	UINT16 disnum;
	float ADC_ConvertedValueLocal; 
	
	printf("������������\r\n");
	
	/* OLED��ʼ�� */
	OLED_Init();

	/* ȫ������ */
	OLED_Fill(0xFF);
	DelayMs(50);
	/*ȫ����*/
	OLED_Fill(0x00);
	DelayMs(50);
	
	/*������ʾ��ҳ*/
	newpage = DIS_PAGE_1;
	oldpage = DIS_PAGE_1;
	
	while(1)
	{		
		/* �л���ҳ����ʾ�������� */
		if(oldpage != newpage)		
		{				
			oldpage = newpage;			
			OLED_CLS();

			/* ����ҳ����ʾ�� */
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
			
			/* ���»�����ʼҳ�� */
			bmp_index = 0;	
		}
		
    switch(oldpage)
		{
			case DIS_PAGE_1:
			{	
				OLED_DrawHalfBMP(0,0,128,8,(unsigned char *)BMP1,0);//�������BMPλͼ��ʾ
				
				/*������ʾ����*/
				for(i=0;i<4;i++)
				{
					OLED_ShowCN(64+i*16,0,i);									
				}

				OLED_ShowStr(64,2,(unsigned char*)"Sanwiky",2);				//����8*16�ַ�
	
				OLED_ShowStr(64,5,(unsigned char*)"Dis:",1);				//����8*8�ַ�	
				OLED_ShowStr(88,5,(unsigned char*)"        ",1);	
				
				memset(dis,0,sizeof(dis));
				
			  // �رս����жϣ��������н������ݻ������仯
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
				
				// �򿪽����ж�
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);			
				
				if(i == sizeof(disdata))
					OLED_ShowStr(88,5,(unsigned char*)"***mm",1);				//����8*8�ַ�
				else
					OLED_ShowStr(88,5,dis,1);				//����8*8�ַ�
				
				i=0;
				while(dis[i] != 'm' && dis[i] != 0)
					i++;
				
				j=0;
				disnum = 0;
				while(i != j)
					disnum = disnum*10 + (dis[j++]-0x30);
				
				if(disnum > 100)
				{
					/* �ͷŶ�ֵ�ź�����֪ͨ��������  */
					uwRet = LOS_SemPost( Beer_BinarySem_Handle ); 
					if(LOS_OK != uwRet)
					{
						printf("�ͷ��ź���ʧ�ܣ��������0x%X\r\n",uwRet);
					}					
				}
				
				/* ��ʾADC��ѹת��ֵ */
		    ADC_ConvertedValueLocal =(float) ADC_ConvertedValue/4096*3.3; // ��ȡת����ADֵ
				sprintf((char*)disAD, "Vad:%.2fV",ADC_ConvertedValueLocal); 
				OLED_ShowStr(64,7,disAD,1);				//����8*8�ַ�		
					
				break;
			}
			case DIS_PAGE_2:
			{		
				OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP2);//����BMPλͼ��ʾ
				
				break;
			}
			case DIS_PAGE_3:
			{	
				switch(bmp_index)
				{
					case 0:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_0);//����BMPλͼ��ʾ
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_1);//����BMPλͼ��ʾ
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_2);//����BMPλͼ��ʾ
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_3);//����BMPλͼ��ʾ
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_4);//����BMPλͼ��ʾ
						break;
					default:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP4_0);//����BMPλͼ��ʾ
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
						OLED_ShowStr(24,4,(unsigned char*)"I",1);				//����8*16�ַ�
						break;
					case 1:
						OLED_ShowStr(24,4,(unsigned char*)"I L",1);				//����8*16�ַ�
						break;
					case 2:
						OLED_ShowStr(24,4,(unsigned char*)"I LO",1);				//����8*16�ַ�
						break;
					case 3:
						OLED_ShowStr(24,4,(unsigned char*)"I LOV",1);				//����8*16�ַ�
						break;
					case 4:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE",1);				//����8*16�ַ�
						break;
					case 5:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE Y",1);				//����8*16�ַ�
						break;
					case 6:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YO",1);				//����8*16�ַ�
						break;
					case 7:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU",1);				//����8*16�ַ�
						break;
					case 8:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//����8*16�ַ�
						break;
					case 9:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//����8*16�ַ�
						break;
					default:
						OLED_ShowStr(24,4,(unsigned char*)"I LOVE YOU!",1);				//����8*16�ַ�
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
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_0);//����BMPλͼ��ʾ
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_1);//����BMPλͼ��ʾ
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_2);//����BMPλͼ��ʾ
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_3);//����BMPλͼ��ʾ
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_4);//����BMPλͼ��ʾ
						break;
					case 5:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_5);//����BMPλͼ��ʾ
						break;
					default:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP5_0);//����BMPλͼ��ʾ
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
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_0);//����BMPλͼ��ʾ
						break;
					case 1:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_1);//����BMPλͼ��ʾ
						break;
					case 2:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_2);//����BMPλͼ��ʾ
						break;
					case 3:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_3);//����BMPλͼ��ʾ
						break;
					case 4:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_4);//����BMPλͼ��ʾ
						break;
					case 5:
						OLED_DrawBMP(0,0,128,8,(unsigned char *)BMP_VOICE_5);//����BMPλͼ��ʾ
						break;
				}
			}
			default:
				break;
		}
		
		/* ��ȡ��ֵ�ź��� xSemaphore,û��ȡ����һֱ�ȴ� */
		uwRet = LOS_SemPend( Refresh_BinarySem_Handle , 
		                     LOS_WAIT_FOREVER ); 
		if(LOS_OK != uwRet)
		{
			printf("��ȡ�ź���ʧ�ܣ��������0x%X\r\n",uwRet);	
		}	
	}
}

/******************************************************************
  * @ ������  �� NbIot_Task
  * @ ����˵���� NbIot_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void NbIot_Task(void)
{
	/* ����һ���������ͱ�������ʼ��ΪLOS_OK */
	UINT32 uwRet = LOS_OK;

	while(1)
	{
		DelayMs(100);
	}
}

/******************************************************************
  * @ ������  �� WIFI_Task
  * @ ����˵���� WIFI_Task����ʵ��
  * @ ����    �� NULL 
  * @ ����ֵ  �� NULL
  *****************************************************************/
static void WIFI_Task(void)
{
	uint8_t ucStatus;
	char * pRecStr;
	char cStr [ 100 ] = { 0 };
		
  printf ( "\r\n�������� ESP8266 ......\r\n" );
	
	/* ʹ��оƬ */
	macESP8266_CH_ENABLE();
	
	/* ATָ������������� */
	ESP8266_AT_Test ();
	
	/* �޸�ģʽ */
	ESP8266_Net_Mode_Choose ( STA );

	/* ����wifi�ڵ� */
  while ( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );	
	
	/* ��·����ģʽ */
	ESP8266_Enable_MultipleId ( DISABLE );
	
	/* TCP��������Ŀ������� */
	while ( !	ESP8266_Link_Server ( enumTCP, macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, Single_ID_0 ) );
	
	/* ����͸��ģʽ */
	while ( ! ESP8266_UnvarnishSend () );
	
	printf ( "\r\n���� ESP8266 ���\r\n" );
	
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
		
		/* ͸������ */
		ESP8266_SendString ( ENABLE, cStr, 0, Single_ID_0 );               //��������
		
		DelayMs ( 30 );
		
		/* ����Ƿ�ʧȥ���� */
		if ( ucTcpClosedFlag )                                             
		{
			/* �˳�͸��ģʽ������ָ��ģʽ */
			ESP8266_ExitUnvarnishSend ();                                    
			
			/* ��ȡ����״̬ */
			do ucStatus = ESP8266_Get_LinkStatus (); 		
			while ( ! ucStatus );
			
			/* ȷ��ʧȥ���Ӻ����� */
			if ( ucStatus == 4 )                                             
			{
				printf ( "\r\n���������ȵ�ͷ����� ......\r\n" );
				
				/* ����wifi�ڵ� */
				while ( ! ESP8266_JoinAP ( macUser_ESP8266_ApSsid, macUser_ESP8266_ApPwd ) );
				
				/* TCP��������Ŀ������� */
				while ( !	ESP8266_Link_Server ( enumTCP, macUser_ESP8266_TcpServer_IP, macUser_ESP8266_TcpServer_Port, Single_ID_0 ) );
				
				printf ( "\r\n�����ȵ�ͷ������ɹ�\r\n" );

			}
			
			/* ����͸��ģʽ */
			while ( ! ESP8266_UnvarnishSend () );		
			
		}
		
		/* wifi�жϽ������� */
		if( strEsp8266_Fram_Record .InfBit .FramFinishFlag )
		{
			strEsp8266_Fram_Record .Data_RX_BUF [ strEsp8266_Fram_Record .InfBit .FramLength ] = '\0';
			printf ("\r\n%s\r\n", strEsp8266_Fram_Record .Data_RX_BUF);
			
			strEsp8266_Fram_Record .InfBit .FramLength = 0;
			strEsp8266_Fram_Record .InfBit .FramFinishFlag = 0;
		}
		
//		/* wifi�������� */
//		pRecStr = ESP8266_ReceiveString ( ENABLE );
//		printf ("\r\n%s\r\n", pRecStr);
//		
//		DelayMs ( 20 );
	}
}

/*******************************************************************
  * @ ������  �� BSP_Init
  * @ ����˵���� �弶�����ʼ�������а����ϵĳ�ʼ�����ɷ��������������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ******************************************************************/
static void BSP_Init(void)
{
	/*
	 * STM32�ж����ȼ�����Ϊ4����4bit��������ʾ��ռ���ȼ�����ΧΪ��0~15
	 * ���ȼ�����ֻ��Ҫ����һ�μ��ɣ��Ժ������������������Ҫ�õ��жϣ�
	 * ��ͳһ��������ȼ����飬ǧ��Ҫ�ٷ��飬�мɡ�
	 */
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	/* LED ��ʼ�� */
	LED_GPIO_Config();

	/* ���ڳ�ʼ��	*/
	DEBUG_USART_Config();// ���Դ������
	USART2_Config();// �����������
	//USART3_Config();// 
  ESP8266_Init (); // WIFI��������
	UART4_Config();// ��മ������
	UART5_Config();// NBIOTͨѶģ��

	/* ������ʼ��	*/
	KEY_GPIO_Config();
	
	/* ��������ʼ��	*/
	BEEP_GPIO_Config();
	
	/* SPI����flash */
	SPI_FLASH_Init();

	/* IIC��ʼ��OLED */
	I2C_Configuration();

	/* ����ģ���ʼ�� */
	XFS_VoiceInit();
	
	/* ADC��ʼ�� */
	ADCx_Init();
}


/*********************************************END OF FILE**********************/

