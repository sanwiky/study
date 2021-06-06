

#include "nbiot.h"

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
//----------------------------------------
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  
//----------------------------------------
u8 Scan_Wtime = 0;//保存扫描需要的时间
u8 BT_Scan_mode=0;//蓝牙扫描设备模式标志
char *extstrx;
extern char  RxBuffer1[100],RxCounter;
BC95 BC95_Status;
u8 jie[255];   //存放接收信息
//----------------------------------------
//usmart支持部分 
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART2_RX_STA;
//     1,清零USART2_RX_STA;
void sim_at_response(u8 mode)
{
    if(USART2_RX_STA&0X8000)		//接收到一次数据了
    {
        USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
        printf("%s",USART2_RX_BUF);	//发送到串口
        if(mode)USART2_RX_STA=0;
    }
}
//////////////////////////////////////////////////////////////////////////////////
//ATK-NB 各项测试(拨号测试、短信测试、GPRS测试、蓝牙测试)共用代码

//NB发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//其他,期待应答结果的位置(str的位置)
u8* NB_check_cmd(u8 *str)
{
    char *strx=0;
    if(USART2_RX_STA&0X8000)		//接收到一次数据了
    {
        USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//添加结束符
        strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
    }
    return (u8*)strx;
}
//向NB发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 NB_send_cmd(u8 *cmd,u8 *Re1,u8 *Re2,u8 *Re3,u16 waittime)
{

    u8 res=0;
    USART2_RX_STA=0;
    if((u32)cmd<=0XFF)
    {
        while(DMA1_Channel7->CNDTR!=0);	//等待通道7传输完成
        USART2->DR=(u32)cmd;
    }else
    {
        u2_printf("%s\r\n",cmd);    //发送命令
        printf("ATSEND:%s\r\n",cmd);       //打印调试
    }

    if(waittime==1100)              //11s后读回串口数据(扫描模式)
    {
        Scan_Wtime = 11;            //需要定时的时间
        TIM4_SetARR(9999);          //产生1S定时中断
    }


    if((Re1&&waittime)||(Re3&&waittime)||(Re2&&waittime))		//需要等待应答
    {
        while(--waittime)	//等待倒计时
        {
            if(BT_Scan_mode)    //蓝牙扫描模式
            {
                res=KEY_Scan(0);//返回上一级
                if(res==WKUP_PRES)return 2;
            }
            delay_ms(10);

            if(USART2_RX_STA&0X8000)//接收到期待的应答结果
            {
                printf("ATREV:");
                printf((const char*)USART2_RX_BUF,"\r\n"); //收到的模块反馈信息

                if (NB_check_cmd(Re1))
                {
                    return 1;
                }
                if (NB_check_cmd(Re2))
                {
                    return 2;
                }
                if (NB_check_cmd(Re3))
                {
                    return 3;
                }
                USART2_RX_STA=0;
            }
        }
    }
    return res;
} 

//接收NB返回数据（蓝牙测试模式下使用）
//request:期待接收命令字符串
//waittimg:等待时间(单位：10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 NB_wait_request(u8 *request ,u16 waittime)
{
    u8 res = 1;
    u8 key;
    if(request && waittime)
    {
        while(--waittime)
        {
            key=KEY_Scan(0);
            if(key==WKUP_PRES) return 2;//返回上一级
            delay_ms(10);
            if(USART2_RX_STA &0x8000)//接收到期待的应答结果
            {
                if(NB_check_cmd(request)) break;//得到有效数据
                USART2_RX_STA=0;
            }
        }
        if(waittime==0)res=0;
    }
    return res;
}

//将1个字符转换为16进制数字
//chr:字符,0~9/A~F/a~F
//返回值:chr对应的16进制数值
u8 NB_chr2hex(u8 chr)
{
    if(chr>='0'&&chr<='9')return chr-'0';
    if(chr>='A'&&chr<='F')return (chr-'A'+10);
    if(chr>='a'&&chr<='f')return (chr-'a'+10);
    return 0;
}
//将1个16进制数字转换为字符
//hex:16进制数字,0~15;
//返回值:字符
u8 NB_hex2chr(u8 hex)
{
    if(hex<=9)return hex+'0';
    if(hex>=10&&hex<=15)return (hex-10+'A');
    return '0';
}

void BC95_RECData(void)
{
    if(strstr((const char*)USART2_RX_BUF,(const char*)"55010066"))
    {
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_SET);		 //关闭继电器
        printf("关闭继电器\r\n");
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_SET);		 //关闭继电器
    }
    else if(strstr((const char*)USART2_RX_BUF,(const char*)"55010166"))
    {
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_RESET);		//打开继电器
        printf("打开继电器\r\n");
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_RESET);		//打开继电器
    }
}

ErrorStatus NB_init(void)
{
    u8 data=0,ret=0;
    u8 err=0;
    USART2_RX_STA=0;

    delay_ms(2000);                                                 //等待系统启动
    if(NB_send_cmd("AT","OK","NULL","NULL",1000))err|=1<<0;         //检测是否应答AT指令

    USART2_RX_STA=0;
    if(NB_send_cmd("AT+CMEE=1","OK","NULL","NULL",2000))err|=1<<1;  //允许错误值 /* Use AT+CMEE=1 to enable result code and use numeric values */

    USART2_RX_STA=0;
    NB_send_cmd("AT+CGSN=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;
    NB_send_cmd("AT+NBAND?\r\n","NBAND","OK","NULL",2000); //设置频段号
    printf("HEE:");
    printf((const char*)USART2_RX_BUF,"\r\n"); //收到的模块反馈信息,直接反馈了ret信息没有被清空

    if(strstr((char*)USART2_RX_BUF,"+NBAND:8"))
       OLED_ShowString(4,4,"BAND:8 REG[..]");//显示移动联通;
		if(strstr((char*)USART2_RX_BUF,"+NBAND:5"))
			 OLED_ShowString(4,4,"BAND:5 REG[..]");//显示移动联通;
														 
    USART2_RX_STA=0;
    NB_send_cmd("AT+NBAND?\r\n","+NBAND:7","NULL","NULL",2000); 	//获取频段号 参数不要改 增加检测时间


    USART2_RX_STA=0;
    if(NB_send_cmd("AT+CIMI","OK","NULL","NULL",2000))err|=1<<3;	//获取卡号，类似是否存在卡的意思，比较重要。
    ClearRAM((u8*)cardIMSI,80);
    sprintf(cardIMSI,"%s",(char*)(&USART2_RX_BUF[2]));
    printf("我是卡号：%s\r\n",cardIMSI);//发送命令

    if(!strstr((char*)cardIMSI ,"ERROR"))
        OLED_ShowString(4,4,"NBSIMCARD [OK]");//显示卡状态OK
    else
        OLED_ShowString(4,4,"NBSIMCARD [NO]");
/*
    USART2_RX_STA=0;                        //激活PDP
    NB_send_cmd("AT+CGATT=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;
    //delay_ms(500);                          //查询PDP的激活情况
    NB_send_cmd("AT+CGATT?\r\n","+CGATT: 1","OK","NULL",2000);
*/
    USART2_RX_STA=0;                        //显示信号
    NB_send_cmd("AT+CSQ\r\n","+CSQ","NULL","NULL",2000);
    if(strstr((const char*)USART2_RX_BUF,(const char*)"+CSQ:"))
    {
        ClearRAM((u8*)signalCSQ,80);            //清空信号数组
        strncpy((char*)(signalCSQ+strlen((char*)signalCSQ)),(char*)(&USART2_RX_BUF[3]),6); //拷贝信号的6个字节
        OLED_ShowString(4,6,(char*)signalCSQ);      //显示信号质量
        printf("我是信号质量：%s\r\n",USART2_RX_BUF);  //打印信号质量 所有字符都打印
    }

    USART2_RX_STA=0;                                //注册状态
    NB_send_cmd("AT+CEREG?\r\n","+CEREG:","NULL","NULL",2000);

    USART2_RX_STA=0;                                ///* Use AT+CEREG =1 to enable network registration unsolicited result code */
    NB_send_cmd("AT+CEREG=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //获取IMEI
    NB_send_cmd("AT+CGSN=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //配置APN
    NB_send_cmd("AT+CGDCONT=1,\042IP\042,\042HUAWEI.COM\042","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //激活PDP
    NB_send_cmd("AT+CGATT=1\r\n","OK","NULL","NULL",2000);
		
		delay_ms(2000);
    USART2_RX_STA=0;                                //查询激活状态
    //NB_send_cmd("AT+CGATT?\r\n","+CGATT","NULL","NULL",2000);
     delay_ms(2000);
    NB_send_cmd("AT+CGATT?\r\n","+CGATT: 1","OK","NULL",2000);

/*不要获取配置信息
    USART2_RX_STA=0;                                //获取配置信息
    NB_send_cmd("AT+NCONFIG?\r\n","OK","NULL","NULL",2000);
*/

    USART2_RX_STA=0;
    NB_send_cmd("AT+NSOCL=0\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //获取ip地址
    NB_send_cmd("AT+CGPADDR\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;/* Use AT+NSOCR to create a socket on the UE and associates with specified protocol */
    NB_send_cmd("AT+NSOCR=DGRAM,17,3005,1\r\n","OK","NULL","NULL",2000);//UDP

    delay_ms(2000);

    USART2_RX_STA=0;                                                 //发送www.csgsm.com 13个字符
    data=NB_send_cmd(SENDUDPDATA,"OK","NULL","NULL",2000);	//发送数据到服务器

    USART2_RX_STA=0;
    if (data == 1 || data == 2 || data == 3 || ret==1)
    {
        printf("\r\n====data=%d=====\r\n",data);
        return SUCCESS;
    }
    else
    {
        return ERROR;
    }
} 

//重启NB模块
ErrorStatus NB_ret(void)
{
    NB_send_cmd("AT+NRB\r\n","OK","NULL","NULL",2000);
    USART2_RX_STA=0;
    delay_ms(5000);
    printf("OK\r\n");
}

