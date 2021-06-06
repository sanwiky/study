

#include "nbiot.h"

#define USART_REC_LEN  			200  	//�����������ֽ��� 200
//----------------------------------------
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  
//----------------------------------------
u8 Scan_Wtime = 0;//����ɨ����Ҫ��ʱ��
u8 BT_Scan_mode=0;//����ɨ���豸ģʽ��־
char *extstrx;
extern char  RxBuffer1[100],RxCounter;
BC95 BC95_Status;
u8 jie[255];   //��Ž�����Ϣ
//----------------------------------------
//usmart֧�ֲ��� 
//���յ���ATָ��Ӧ�����ݷ��ظ����Դ���
//mode:0,������USART2_RX_STA;
//     1,����USART2_RX_STA;
void sim_at_response(u8 mode)
{
    if(USART2_RX_STA&0X8000)		//���յ�һ��������
    {
        USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
        printf("%s",USART2_RX_BUF);	//���͵�����
        if(mode)USART2_RX_STA=0;
    }
}
//////////////////////////////////////////////////////////////////////////////////
//ATK-NB �������(���Ų��ԡ����Ų��ԡ�GPRS���ԡ���������)���ô���

//NB���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8* NB_check_cmd(u8 *str)
{
    char *strx=0;
    if(USART2_RX_STA&0X8000)		//���յ�һ��������
    {
        USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;//��ӽ�����
        strx=strstr((const char*)USART2_RX_BUF,(const char*)str);
    }
    return (u8*)strx;
}
//��NB��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 NB_send_cmd(u8 *cmd,u8 *Re1,u8 *Re2,u8 *Re3,u16 waittime)
{

    u8 res=0;
    USART2_RX_STA=0;
    if((u32)cmd<=0XFF)
    {
        while(DMA1_Channel7->CNDTR!=0);	//�ȴ�ͨ��7�������
        USART2->DR=(u32)cmd;
    }else
    {
        u2_printf("%s\r\n",cmd);    //��������
        printf("ATSEND:%s\r\n",cmd);       //��ӡ����
    }

    if(waittime==1100)              //11s����ش�������(ɨ��ģʽ)
    {
        Scan_Wtime = 11;            //��Ҫ��ʱ��ʱ��
        TIM4_SetARR(9999);          //����1S��ʱ�ж�
    }


    if((Re1&&waittime)||(Re3&&waittime)||(Re2&&waittime))		//��Ҫ�ȴ�Ӧ��
    {
        while(--waittime)	//�ȴ�����ʱ
        {
            if(BT_Scan_mode)    //����ɨ��ģʽ
            {
                res=KEY_Scan(0);//������һ��
                if(res==WKUP_PRES)return 2;
            }
            delay_ms(10);

            if(USART2_RX_STA&0X8000)//���յ��ڴ���Ӧ����
            {
                printf("ATREV:");
                printf((const char*)USART2_RX_BUF,"\r\n"); //�յ���ģ�鷴����Ϣ

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

//����NB�������ݣ���������ģʽ��ʹ�ã�
//request:�ڴ����������ַ���
//waittimg:�ȴ�ʱ��(��λ��10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 NB_wait_request(u8 *request ,u16 waittime)
{
    u8 res = 1;
    u8 key;
    if(request && waittime)
    {
        while(--waittime)
        {
            key=KEY_Scan(0);
            if(key==WKUP_PRES) return 2;//������һ��
            delay_ms(10);
            if(USART2_RX_STA &0x8000)//���յ��ڴ���Ӧ����
            {
                if(NB_check_cmd(request)) break;//�õ���Ч����
                USART2_RX_STA=0;
            }
        }
        if(waittime==0)res=0;
    }
    return res;
}

//��1���ַ�ת��Ϊ16��������
//chr:�ַ�,0~9/A~F/a~F
//����ֵ:chr��Ӧ��16������ֵ
u8 NB_chr2hex(u8 chr)
{
    if(chr>='0'&&chr<='9')return chr-'0';
    if(chr>='A'&&chr<='F')return (chr-'A'+10);
    if(chr>='a'&&chr<='f')return (chr-'a'+10);
    return 0;
}
//��1��16��������ת��Ϊ�ַ�
//hex:16��������,0~15;
//����ֵ:�ַ�
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
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_SET);		 //�رռ̵���
        printf("�رռ̵���\r\n");
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_SET);		 //�رռ̵���
    }
    else if(strstr((const char*)USART2_RX_BUF,(const char*)"55010166"))
    {
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_RESET);		//�򿪼̵���
        printf("�򿪼̵���\r\n");
        Relay_WriteBit(RELAY_IO, Relay_B, Bit_RESET);		//�򿪼̵���
    }
}

ErrorStatus NB_init(void)
{
    u8 data=0,ret=0;
    u8 err=0;
    USART2_RX_STA=0;

    delay_ms(2000);                                                 //�ȴ�ϵͳ����
    if(NB_send_cmd("AT","OK","NULL","NULL",1000))err|=1<<0;         //����Ƿ�Ӧ��ATָ��

    USART2_RX_STA=0;
    if(NB_send_cmd("AT+CMEE=1","OK","NULL","NULL",2000))err|=1<<1;  //�������ֵ /* Use AT+CMEE=1 to enable result code and use numeric values */

    USART2_RX_STA=0;
    NB_send_cmd("AT+CGSN=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;
    NB_send_cmd("AT+NBAND?\r\n","NBAND","OK","NULL",2000); //����Ƶ�κ�
    printf("HEE:");
    printf((const char*)USART2_RX_BUF,"\r\n"); //�յ���ģ�鷴����Ϣ,ֱ�ӷ�����ret��Ϣû�б����

    if(strstr((char*)USART2_RX_BUF,"+NBAND:8"))
       OLED_ShowString(4,4,"BAND:8 REG[..]");//��ʾ�ƶ���ͨ;
		if(strstr((char*)USART2_RX_BUF,"+NBAND:5"))
			 OLED_ShowString(4,4,"BAND:5 REG[..]");//��ʾ�ƶ���ͨ;
														 
    USART2_RX_STA=0;
    NB_send_cmd("AT+NBAND?\r\n","+NBAND:7","NULL","NULL",2000); 	//��ȡƵ�κ� ������Ҫ�� ���Ӽ��ʱ��


    USART2_RX_STA=0;
    if(NB_send_cmd("AT+CIMI","OK","NULL","NULL",2000))err|=1<<3;	//��ȡ���ţ������Ƿ���ڿ�����˼���Ƚ���Ҫ��
    ClearRAM((u8*)cardIMSI,80);
    sprintf(cardIMSI,"%s",(char*)(&USART2_RX_BUF[2]));
    printf("���ǿ��ţ�%s\r\n",cardIMSI);//��������

    if(!strstr((char*)cardIMSI ,"ERROR"))
        OLED_ShowString(4,4,"NBSIMCARD [OK]");//��ʾ��״̬OK
    else
        OLED_ShowString(4,4,"NBSIMCARD [NO]");
/*
    USART2_RX_STA=0;                        //����PDP
    NB_send_cmd("AT+CGATT=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;
    //delay_ms(500);                          //��ѯPDP�ļ������
    NB_send_cmd("AT+CGATT?\r\n","+CGATT: 1","OK","NULL",2000);
*/
    USART2_RX_STA=0;                        //��ʾ�ź�
    NB_send_cmd("AT+CSQ\r\n","+CSQ","NULL","NULL",2000);
    if(strstr((const char*)USART2_RX_BUF,(const char*)"+CSQ:"))
    {
        ClearRAM((u8*)signalCSQ,80);            //����ź�����
        strncpy((char*)(signalCSQ+strlen((char*)signalCSQ)),(char*)(&USART2_RX_BUF[3]),6); //�����źŵ�6���ֽ�
        OLED_ShowString(4,6,(char*)signalCSQ);      //��ʾ�ź�����
        printf("�����ź�������%s\r\n",USART2_RX_BUF);  //��ӡ�ź����� �����ַ�����ӡ
    }

    USART2_RX_STA=0;                                //ע��״̬
    NB_send_cmd("AT+CEREG?\r\n","+CEREG:","NULL","NULL",2000);

    USART2_RX_STA=0;                                ///* Use AT+CEREG =1 to enable network registration unsolicited result code */
    NB_send_cmd("AT+CEREG=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //��ȡIMEI
    NB_send_cmd("AT+CGSN=1\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //����APN
    NB_send_cmd("AT+CGDCONT=1,\042IP\042,\042HUAWEI.COM\042","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //����PDP
    NB_send_cmd("AT+CGATT=1\r\n","OK","NULL","NULL",2000);
		
		delay_ms(2000);
    USART2_RX_STA=0;                                //��ѯ����״̬
    //NB_send_cmd("AT+CGATT?\r\n","+CGATT","NULL","NULL",2000);
     delay_ms(2000);
    NB_send_cmd("AT+CGATT?\r\n","+CGATT: 1","OK","NULL",2000);

/*��Ҫ��ȡ������Ϣ
    USART2_RX_STA=0;                                //��ȡ������Ϣ
    NB_send_cmd("AT+NCONFIG?\r\n","OK","NULL","NULL",2000);
*/

    USART2_RX_STA=0;
    NB_send_cmd("AT+NSOCL=0\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;                                //��ȡip��ַ
    NB_send_cmd("AT+CGPADDR\r\n","OK","NULL","NULL",2000);

    USART2_RX_STA=0;/* Use AT+NSOCR to create a socket on the UE and associates with specified protocol */
    NB_send_cmd("AT+NSOCR=DGRAM,17,3005,1\r\n","OK","NULL","NULL",2000);//UDP

    delay_ms(2000);

    USART2_RX_STA=0;                                                 //����www.csgsm.com 13���ַ�
    data=NB_send_cmd(SENDUDPDATA,"OK","NULL","NULL",2000);	//�������ݵ�������

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

//����NBģ��
ErrorStatus NB_ret(void)
{
    NB_send_cmd("AT+NRB\r\n","OK","NULL","NULL",2000);
    USART2_RX_STA=0;
    delay_ms(5000);
    printf("OK\r\n");
}

