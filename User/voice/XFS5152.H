/***************************飞音云电子****************************
**  工程名称：YS-XFS5051语音合成配套程序
***************************飞音云电子******************************/


#ifndef XFS_H
#define XFS_H


extern const char voice_ShanWaiKeJi[];//山外科技
extern const char voice_ChooseMode[];//请选择模式
extern const char voice_InitDevice[];//初始化设备
extern const char voice_SystermStart[];//系统启动中

extern const char voice_size_0[];//静音
extern const char voice_size_1[];//音量一
extern const char voice_size_2[];//音量二
extern const char voice_size_3[];//音量三
extern const char voice_size_4[];//音量四
extern const char voice_size_5[];//音量五
extern const char voice_size_6[];//音量六

void XFS_VoiceInit(void);
void XFS_VoiceChange(unsigned char voicenum);
void XFS_FrameInfo(const char *HZdata, unsigned short HZdatalen);
	
#endif
