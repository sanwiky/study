/***************************�����Ƶ���****************************
**  �������ƣ�YS-XFS5051�����ϳ����׳���
***************************�����Ƶ���******************************/


#ifndef XFS_H
#define XFS_H


extern const char voice_ShanWaiKeJi[];//ɽ��Ƽ�
extern const char voice_ChooseMode[];//��ѡ��ģʽ
extern const char voice_InitDevice[];//��ʼ���豸
extern const char voice_SystermStart[];//ϵͳ������

extern const char voice_size_0[];//����
extern const char voice_size_1[];//����һ
extern const char voice_size_2[];//������
extern const char voice_size_3[];//������
extern const char voice_size_4[];//������
extern const char voice_size_5[];//������
extern const char voice_size_6[];//������

void XFS_VoiceInit(void);
void XFS_VoiceChange(unsigned char voicenum);
void XFS_FrameInfo(const char *HZdata, unsigned short HZdatalen);
	
#endif