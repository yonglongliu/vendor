#ifndef __GPS_API_H
#define __GPS_API_H

#define	MSISDN_GET_CMD  1
#define	IMSI_GET_CMD  3
#define	CELL_LOCATION_GET_CMD  4
#define	VERIF_GET_CMD  5
#define	SMS_GET_CMD  6
#define	DATA_GET_CMD  7
#define	SEND_TIME 10
#define	SEND_LOCTION 11
#define	SEND_MDL 12

#define SEND_DELIVER_ASSIST_END 13
#define SEND_SUPL_END 14
#define SEND_ACQUIST_ASSIST 15
#define SEND_SUPL_READY 16
#define SEND_MSA_POSITION 17

#define GET_POSITION   18
#define WIFI_INFO      19
#define SMS_CP_CMD     20
#define IP_GET_CMD     21
#define SIM_PLMN_CMD   22



#define EVENT_WAIT_FOREVER   (0xFFFFFFFF)


//define the CircularBuffer

typedef struct _TCircular_buffer
{
	char* pBuffer;
	unsigned int  BufferSize;
	unsigned int  DataLen;
	unsigned int  ReadPosition;
	unsigned int  WritePosition;
} TCircular_buffer;

int InitializeCircularBuffer( TCircular_buffer* pBuffer, unsigned int  BufferSize);

void FreeCircularBuffer(TCircular_buffer* pBuffer);

void ResetCircularBuffer(TCircular_buffer* pBuffer);


unsigned int CircularBufferDataLen(TCircular_buffer* pBuffer);

unsigned int PushCircularBufferEntry( TCircular_buffer* pBuffer,const  unsigned char* pData, unsigned int len);
int CreateEvent(void *pData, const char* name);
int  DestroyEvent(void *pData);
int WaitEvent( void *pData, unsigned int mSecTimeout);
int SignalEvent(void *pData);
FILE* FileOpen(const char* aFileName);
int  FileClose(FILE* fileHandle);
int GetFileSize(const char *aFileName);
void WriteFile( FILE* fp,const unsigned char *pData,int len,int offset);
int  FileRead(void *aData, unsigned long aSize, unsigned long aCount, void* aFile);

int get_xml_value(const char *token, char *val);
char set_xml_value(const char *token, char *val);
int config_xml_ctrl(char mode,const char *token,char *val,int type);
void set_up_callbacks(void);
void set_cp_callbacks(void);
void agps_send_lastrefparam(int intervaltime);
int ThreadCreateCond(void *pData, const pthread_condattr_t *attr);
int  ThreadDestroyCond(void *pData);
int ThreadWaitCond( void *pData, unsigned int mSecTimeout);
int ThreadSignalCond(void *pData);

#endif
