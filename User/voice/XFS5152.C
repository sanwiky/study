/******************************************************************
**  工程名称：YS-XFS5152语音合成配套程序
**
***************************飞音云电子******************************/

#include <string.h>
#include "bsp_usart.h"
#include "XFS5152.h"



/**************芯片设置命令*********************/
const char voice_ShanWaiKeJi[] = "山外科技";
const char voice_ChooseMode[]  = "请选择模式";
const char voice_InitDevice[]  = "初始化设备";
const char voice_SystermStart[]= "系统启动中";

const char voice_size_0[] = "静音";
const char voice_size_1[] = "音量一";
const char voice_size_2[] = "音量二";
const char voice_size_3[] = "音量三";
const char voice_size_4[] = "音量四";
const char voice_size_5[] = "音量五";
/***********************************************/

/***********************************************************
* 名    称：  YS-XFS5051 文本合成函数
* 功    能：  发送合成文本到XFS5051芯片进行合成播放
* 入口参数：  *HZdata:文本指针变量 
* 出口参数：
* 说    明： 本函数只用于文本合成，具备背景音乐选择。默认波特率9600bps。					 
* 调用方法：例： SYN_FrameInfo（“飞音云电子”）；
**********************************************************/
void XFS_VoiceChange(unsigned char voicenum)
{
/****************需要发送的文本**********************************/ 
	unsigned  char Frame_Info[10]; //定义的文本长度
	unsigned  int  i=0; 

/*****************帧固定配置信息**************************************/           
	Frame_Info[0] = 0xFD ; 			//构造帧头FD
	Frame_Info[1] = 0x00 ; 			//构造数据区长度的高字节
  Frame_Info[2] = 0x06 ;      //构造数据区长度的低字节
	Frame_Info[3] = 0x01 ; 			//构造命令字：合成播放命令		 		 
	Frame_Info[4] = 0x01;       //文本编码格式：GBK 
	Frame_Info[5] = 0x5B ; 			//构造命令字：合成播放命令		 		 
	Frame_Info[6] = 0x76;       //文本编码格式：GBK 
	Frame_Info[7] = 0x30+voicenum; 			//构造命令字：合成播放命令		 		 
	Frame_Info[8] = 0x5D;       //文本编码格式：GBK 
 
	for(i=0;i<9;i++)
	{
	   Usart_SendByte(USART2,Frame_Info[i]);	
	}
}

/***********************************************************
* 名    称：  YS-XFS5051 文本合成函数
* 功    能：  发送合成文本到XFS5051芯片进行合成播放
* 入口参数：  *HZdata:文本指针变量 
* 出口参数：
* 说    明： 本函数只用于文本合成，具备背景音乐选择。默认波特率9600bps。					 
* 调用方法：例： SYN_FrameInfo（“飞音云电子”）；
**********************************************************/
void XFS_FrameInfo(const char *HZdata, unsigned short HZdatalen)
{
/****************需要发送的文本**********************************/ 
	unsigned  char  Frame_Info[50]; //定义的文本长度
	unsigned  int i=0; 

/*****************帧固定配置信息**************************************/           
	Frame_Info[0] = 0xFD ; 			//构造帧头FD
	Frame_Info[1] = 0x00 ; 			//构造数据区长度的高字节
  Frame_Info[2] = HZdatalen+2;//构造数据区长度的低字节
	Frame_Info[3] = 0x01 ; 			//构造命令字：合成播放命令		 		 
	Frame_Info[4] = 0x01;       //文本编码格式：GBK 
 
/*******************发送帧信息***************************************/		  
  memcpy(&Frame_Info[5], HZdata, HZdatalen);

	for(i=0;i<5+HZdatalen;i++)
	{
	   Usart_SendByte(USART2,Frame_Info[i]);	
	}	
}

/***********************************************************
* 名    称：  YS-XFS5051 文本合成函数
* 功    能：  发送合成文本到XFS5051芯片进行合成播放
* 入口参数：  *HZdata:文本指针变量 
* 出口参数：
* 说    明： 本函数只用于文本合成，具备背景音乐选择。默认波特率9600bps。					 
* 调用方法：例： SYN_FrameInfo（“飞音云电子”）；
**********************************************************/
void XFS_VoiceInit(void)
{
	XFS_VoiceChange(1);          //默认启动音量一
}



