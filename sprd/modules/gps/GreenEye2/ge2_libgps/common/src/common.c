#include <stdio.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include "gps_api.h"
#include "gps_common.h"


/*--------------------------------------------------------------------------
	Function:  config_xml_ctrl

	Description:
		config xml control function, include read and write.
	Parameters:
 		mode:
			GET_XML 0x00
			SET_XML 0x01
		token:the string will be ctrl.
		val:the value get or set.
        type SUPL_TYPE    0
        type CONFIG_TYPE  1
	Return:1 for fail,0 for success.
--------------------------------------------------------------------------*/
int config_xml_ctrl(char mode,const char *token,char *val,int type)
{
	GpsState *s = _gps_state;
	char len = 0;
	FILE *fp = NULL;
    char * pReadc = NULL;
	char *curptr,*begin,*end;
	int sz=0,size = 0;
	char *config_xml = NULL;

	if(type == SUPL_TYPE)
	{
		config_xml = SUPL_XML_PATH;
	}
	else if(type == CONFIG_TYPE)
	{
	    if(s->getetcconfig)
        {
            config_xml = CONFIG_ETC_PATH;
        }
        else
        {
            config_xml = CONFIG_XML_PATH;
        }
        
	}
	if(config_xml == NULL)
	{
		E("config_xml_ctrl: config_xml is NULL");
		return 1;
	}
	if(access(config_xml,0) == -1){
		E("config_xml_ctrl: fail to access config xml because of %s\n",strerror(errno));
		return 1;
	}
    
    if(mode == SET_XML)
    {
	    fp = fopen(config_xml,"r+");
    }
    else
   	{
	    fp = fopen(config_xml,"r");
    }
   	if(fp == NULL)
	{
		E("config_xml_ctrl: fail to open config xml because of %s\n",strerror(errno));
		return 1;
	}
    fseek(fp,0L,SEEK_END);
	size = ftell(fp);
	E("size is %d\n",size);
	pReadc = calloc(size,1);
	if(pReadc == NULL)
	{
	    fclose(fp);
		E("config_xml_ctrl: calloc buffer fail\n");
	    return 1;
	}
    rewind(fp);
	fread(pReadc,size,1,fp);
	if(mode == SET_XML)  //set token to xml
	{
		if((curptr = strstr(pReadc,token)) != NULL)
		{
			if((begin = strchr(curptr+strlen(token)+2,'\"')) != NULL)
			{
				if((end = strchr(begin+1,'\"')) != NULL)
				{
					if(strlen(val) == (unsigned int)(end-begin-1))
					{
						fseek(fp,(begin - pReadc + 1),SEEK_SET);
						fwrite(val,1,strlen(val),fp);
					}
					else
					{
						fseek(fp,(begin - pReadc + 1),SEEK_SET);
						fwrite(val,1,strlen(val),fp);
						sz = ftell(fp);
						
						E("size is %d\n",(int)(sz+size-(end-pReadc)));
						fwrite(end,1,size-(end-pReadc),fp);
						fflush(fp);
						truncate(config_xml,sz+size-(end-pReadc));
					}

					E("get val is %s\n",val);
				}
				else
				{
					fclose(fp);
					free(pReadc);
					return 1;
				}
			}
			else
			{
				fclose(fp);
				free(pReadc);
				return 1;
			}
		}
		else
		{
			E("can't get this token in setting\n");
			fclose(fp);
			free(pReadc);
			return 1;
		}
	}
	else if(mode == GET_XML)  //get  val from xml
	{  
		if((curptr = strstr(pReadc,token)) != NULL)
		{
			if((begin = strchr(curptr+strlen(token)+2,'\"')) != NULL)
			{
				if((end = strchr(begin+1,'\"')) != NULL)
				{
					memcpy(val,begin+1,end-begin-1);                
					D("get val is %s\n",val);
				}
				else
				{
					fclose(fp);
					free(pReadc);
					return 1;
				}
			}
			else
			{
				fclose(fp);
				free(pReadc);
				return 1;
			}
		}
		else
		{
			E("can't get this token in getting\n");
			fclose(fp);
			free(pReadc);
			return 1;
		}        

	}
	fclose(fp);
	free(pReadc);
	return 0;
}

/*--------------------------------------------------------------------------
	Function:  get_xml_value

	Description:
		it  get  token  value in  supl.xml.
	Parameters:
 		below. 
	Return:void
--------------------------------------------------------------------------*/
int get_xml_value(const char *token, char *val)
{
	GpsState *s = _gps_state;
	unsigned int i = 0,j = 0;
	char *pdata,len = 0;
	FILE *fp = NULL;
	char * pReadc = NULL;
	struct stat Filestat;
	int ret = -1,parused = 0;

	if(access("/data/gnss/supl/supl.xml",0) == -1)
	{
		E("get_xml_value fail to access supl xml: errno=%d, strerror(errno)=%s", errno, strerror(errno));
		return ret;
	}

	fp = fopen("/data/gnss/supl/supl.xml","r+");
	if(fp == NULL)
	{
		E("get_xml_value fail to open supl xml: errno=%d, strerror(errno)=%s", errno, strerror(errno));
		return ret;
	}

	fflush(fp);

	if(strlen(token) >= 8)
	{
		if(!memcmp(token,"SUPL-CER",8))
		{
			E("now get supl path value");
			parused = 1;
		}
	}
	stat("/data/gnss/supl/supl.xml", &Filestat);
	pReadc = malloc(Filestat.st_size);
	if(pReadc == NULL)
	{
	    fclose(fp);
		E("get_xml_value: malloc buffer fail\n");
	    return ret;
	}
	memset(pReadc,0,Filestat.st_size);
	fread(pReadc,Filestat.st_size,1,fp);
	len = strlen(token);
	pdata = calloc(len + 2,1);
	if(pdata == NULL)
	{
		E("get_xml_value: calloc buffer fail\n");
		free(pReadc);
		fclose(fp);
		return ret;
	}
	for(i = 0;i < Filestat.st_size;i++)
	{
		memcpy(pdata,pReadc+i,len);
		pdata[len + 1] = '\0';
		ret = strcmp(pdata,token);
		if(ret == 0) break;
	}
	if(0 == ret)
	{
		while(pReadc[i+len+9] != '\"')
		{
			val[j++] = pReadc[i+len+9];
			i++;
			if((parused == 1) && (j > 120)) break;
			else if((parused == 0) && (j > 31)) break;
		}
	}
	val[j] = '\0';

	free(pdata);
	free(pReadc);
	fclose(fp);
	return 0;
}
/*--------------------------------------------------------------------------
	Function:  set_xml_value

	Description:
		it  set  value  to supl xml. 
	Parameters:
 		token-----the string will set.
		val----value will be set. 
	Return:char,1 fail,0 sucess
--------------------------------------------------------------------------*/
char set_xml_value(const char *token, char *val)
{
	GpsState *s = _gps_state;
	unsigned int i = 0,j = 0;
	char *pdata,ret = 1,len = 0;
	FILE *fp = NULL;
	struct stat Filestat;
	char * pReadc = NULL;

    if(access("/data/gnss/supl/supl.xml",0) == -1)
	{
		E("set_xml_value fail to access supl xml: errno=%d, strerror(errno)=%s", errno, strerror(errno));
        return 0;
    }
	fp = fopen("/data/gnss/supl/supl.xml","r+");
   	if(fp == NULL)
	{
		E("set_xml_value fail to open supl xml: errno=%d, strerror(errno)=%s", errno, strerror(errno));
	    return ret;
    }

	stat("/data/gnss/supl/supl.xml", &Filestat);
	pReadc = calloc(Filestat.st_size ,1);
	if(pReadc == NULL){
	    fclose(fp);
		E("set_xml_value: calloc Filestat.st_size buffer fail\n");
	    return ret;
	}
	fread(pReadc,Filestat.st_size,1,fp);

	len = strlen(token);
	pdata = calloc(len + 2,1);
	if(pdata == NULL)
	{
		fclose(fp);
		free(pReadc);
		E("set_xml_value: calloc len size buffer fail\n");
		return 1;
	}
	for(i = 0;i < Filestat.st_size;i++)
	{
		memcpy(pdata,pReadc+i,len);
		pdata[len + 1] = '\0';
		ret = strcmp(pdata,token);
		if(ret == 0)
		break;
	}

	while((ret == 0) && (pReadc[i+len+9] != '\"'))
	{
		pReadc[i+len+9] = val[j++];
		i++;	
		if(j > 31)
		break;
	}
	fseek(fp,0L,SEEK_SET);
	fwrite(pReadc,Filestat.st_size,1,fp);

        fclose(fp);
        free(pReadc);
	free(pdata);
	return ret;
}

int InitializeCircularBuffer( TCircular_buffer* pBuffer, unsigned int BufferSize)
{
	pBuffer->pBuffer = malloc(BufferSize);
	if(!pBuffer->pBuffer)
	{
		E("InitializeCircularBuffer malloc failed\r\n");
		return -1;
	}
	pBuffer->DataLen = 0;
	pBuffer->BufferSize = BufferSize;
	pBuffer->ReadPosition = 0;
	pBuffer->WritePosition = 0;
       
       return 0;
}

void  FreeCircularBuffer(TCircular_buffer* pBuffer)
{
	if(pBuffer->pBuffer)
	{
		free(pBuffer->pBuffer);
		pBuffer->pBuffer = 0;
	}
}

void ResetCircularBuffer(TCircular_buffer* pBuffer)
{
	pBuffer->ReadPosition = 0;
	pBuffer->WritePosition = 0;
	pBuffer->DataLen = 0;
}


unsigned int CircularBufferFreeLen(TCircular_buffer* pBuffer)
{
	return pBuffer->BufferSize - pBuffer->DataLen;
}

unsigned int CircularBufferDataLen(TCircular_buffer* pBuffer)
{
	return pBuffer->DataLen;
}

unsigned int CircularBufferWritePosition(TCircular_buffer* pBuffer)
{
	return pBuffer->WritePosition;
}

unsigned int PushCircularBufferEntry( TCircular_buffer* pBuffer,const unsigned char* pData, unsigned int len)
{
	unsigned int NextPosition = pBuffer->WritePosition;
    unsigned int leftLen = pBuffer->BufferSize - pBuffer->DataLen -1 ; //left one bytes  
    unsigned int inputDataLen = len;
	unsigned int writeLen = 0;

	if ((pData == NULL) || (len == 0))
	{
		return 0;
	}
	
	writeLen = inputDataLen = (leftLen > len)? len:leftLen;

	
	do
    {
		pBuffer->pBuffer[NextPosition] = *pData++;
		NextPosition = (NextPosition + 1) % pBuffer->BufferSize;
	} while(--inputDataLen);

	pBuffer->DataLen += writeLen;
    pBuffer->WritePosition = NextPosition;

	return writeLen;
}


int CreateEvent(void *pData, const char* name)
{
	int ret = sem_init((sem_t *)pData, 
				0, // not shared ,only the libgps process 
				0);// initially set to non signaled state
    D("create event name: %s",name);
    
 	if( ret != 0) 
	{
		ret= errno;
		E("the CreateEvent error %s\r\n",strerror(errno));
	}

	return ret;
}

int  DestroyEvent(void *pData)
{
	int ret = sem_destroy((sem_t *)pData);

	if(!ret)
	{
		ret= errno;
		E("the DestroyEvent error %s\r\n",strerror(errno));
	} 

	return ret;
} 

 void compute_abs_timeout(struct timespec *ts, int ms)
{
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    ts->tv_nsec = tv.tv_usec * 1000 + (ms % 1000) * 1000000;
    ts->tv_sec = tv.tv_sec + ms / 1000;
    if (ts->tv_nsec >= 1000000000) 
    {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}


int WaitEvent(void *pData, unsigned int mSecTimeout)
{
	int ret = 0;
    struct timespec ts;
  
	if(mSecTimeout == EVENT_WAIT_FOREVER)
	{
		ret = sem_wait((sem_t*)pData); 
	}
	else 
	{
	    D("the WaitEvent before \r\n");
        compute_abs_timeout(&ts, mSecTimeout);
        do
        {
            ret = sem_timedwait((sem_t *)pData, &ts);
        } while (ret == -1 && errno == EINTR);
        
	    D("the WaitEvent after ret:%d \r\n",ret);
    }
    if((-1== ret)&&(errno == ETIMEDOUT))
    {
        E("the WaitEvent error timeout\r\n");
        ret = -1;
        return ret;
    }
    if(ret < 0)
    {
      E("the WaitEvent error %d,%s\r\n",errno,strerror(errno));    
    }
    return ret; 
}


int SignalEvent(void *pData)
{
	int ret = 0;

	ret = sem_post((sem_t*)pData);
	
	if (ret != 0)
	{
		ret = errno;
		E("the WaitEvent error %s\r\n",strerror(errno));
	}
	
	return ret;
} 


FILE* FileOpen(const char* aFileName)
{
	FILE*  file = 0;
	int ret = 0;

	//first check the file is exist 
	ret = access(aFileName, F_OK);
	
	if(0 == ret) //file exist
	{
		file = fopen(aFileName, "rb+");
	}
	else //it create file and write and read 
	{
		file = fopen(aFileName, "wb+");
	}

	return file;
}

int  FileClose(FILE* fileHandle)
{
	int ret = fclose(fileHandle);

	return ret;
}

//get the  file size  

int GetFileSize(const char *aFileName)
{
    int len = 0;

    struct stat fileStat = { 0 };

    if ( stat(aFileName, &fileStat ) != 0 )
	{
		D("GetFileSize get file size fail = %d\r\n",errno);
	}
    else
	{
		len = fileStat.st_size;
	}

    return len;
}

void WriteFile( FILE* fp,const unsigned char *pData,int len,int offset)
{
	int writeLen = len;
	int ret = 0;

	ret = fseek(fp,offset,SEEK_SET); 
	if(0 == ret)
	{
		while(writeLen)
		{
			writeLen = fwrite(pData, len, 1, fp);
			pData += writeLen;
			writeLen = len -writeLen;
		}
	}
	else
	{
		//assert(0);
	}

	fflush(fp);
	
}


int  FileRead(void *aData, unsigned long aSize, unsigned long aCount, void* aFile)
{
    return fread(aData, aSize, aCount, (FILE*)aFile);
}

int ThreadCreateCond(void *pData, const pthread_condattr_t *attr)
{
	int ret = 0;
	GpsState*  pState =(GpsState*)pData;
    
    pthread_cond_init(&pState->cond,attr);
    
 	if(ret) 
	{
		ret= errno;
		E(" ThreadCreateCond error %s\r\n",strerror(errno));
	}

	return ret;
}

int  ThreadDestroyCond(void *pData)
{
	int ret = 0;
    
	GpsState*  pState = (GpsState*)pData;

    ret = pthread_cond_destroy(&pState->cond);
 	if( ret != 0) 
	{
		ret= errno;
		E("ThreadDestroyCond error %s\r\n",strerror(errno));
	}
	return ret;
} 


int ThreadWaitCond( void *pData, unsigned int mSecTimeout)
{
	int ret = 0;
    struct timespec ts;
	GpsState*  pState = (GpsState*)pData;
    
    pthread_mutex_lock(&pState->mutex);
    compute_abs_timeout(&ts, mSecTimeout);
    ret = pthread_cond_timedwait(&pState->cond, &pState->mutex, &ts);
    pthread_mutex_unlock(&pState->mutex);
    
	if (ret != 0)
	{
		ret = errno;
		E("ThreadWaitCond  %s\r\n",strerror(errno));
                usleep(20000);//release read thread

	}
    return ret; 
}


int ThreadSignalCond(void *pData)
{
	int ret = 0;
    GpsState*  pState = (GpsState*)pData;
    
    pthread_mutex_lock(&pState->mutex);
    ret = pthread_cond_signal(&pState->cond);
    pthread_mutex_unlock(&pState->mutex);
	if (ret != 0)
	{
		ret = errno;
		E("ThreadSignalCond error %s\r\n",strerror(errno));
	}
	return ret;
} 



