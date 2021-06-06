#ifndef __BC95B5_H__
#define __BC95B5_H__	 

typedef unsigned char u8;
typedef unsigned short u16;

#define swap16(x) (x&0XFF)<<8|(x&0XFF00)>>8	//�ߵ��ֽڽ����궨��

extern u8 Scan_Wtime;

void sim_at_response(u8 mode);	
u8* NB_check_cmd(u8 *str);
u8 NB_send_cmd(u8 *cmd,u8 *Re1,u8 *Re2,u8 *Re3,u16 waittime);
u8 NB_wait_request(u8 *request ,u16 waittime);
u8 NB_chr2hex(u8 chr);
u8 NB_hex2chr(u8 hex);
void BC95_RECData(void);

#endif

typedef struct
{
   u8 CSQ;    
	 u8 Socketnum;   //���
	 u8 reclen;   //��ȡ�����ݵĳ���
   u8 res;      
   u8 recdatalen[10];
   u8 recdata[100];
	 u8 uart1len[10];
	 u8 senddata[100];
} BC95;


