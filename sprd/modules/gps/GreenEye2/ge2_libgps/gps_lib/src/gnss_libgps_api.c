
#include "gnss_libgps_api.h"
#include "gps_common.h"



//it define the  GPS Parse 
TGNSS_DataProcessor_t gGPSStream;
//#define E ALOGE


//for debug 
static void debuginfor(void* pSrc, int len)
{
	unsigned char* ptmp = pSrc;

	if(len > 0 &&  (5 != ptmp[4]))
	{
		D("0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",ptmp[0],ptmp[1],ptmp[2],ptmp[3],ptmp[4],ptmp[5],ptmp[6],ptmp[7],ptmp[8],ptmp[9]);
	}
	//E("\r\n");
}

/******************************************************************
---4 bytes----- 1 byte  --1 byte ---2 bytes ---2 bytes------n bytes-----
|                 |          |             |                |            |   
| (~)            |  Type | subtype |  Length     |  CRC        |   data  |
____________________________________________________________________
********************************************************************/

/*--------------------------------------------------------------------------
    Function:  GNSS_searchFlag

    Description:
        it find the matic  word form the data buffer 
        
    Parameters:
     pStream: point the current  parse  data 
     pData: point the uart buffer data 
     Len       : the receive data length
     processedLen: it deal with data len  
     
    Return: void 
----------------------------------------------------------------------------------*/
static void GNSS_searchFlag(TGNSS_DataProcessor_t* pStream,unsigned char* pData,
								 unsigned short Len, unsigned short* pProcessedLen)
{
	unsigned short headsize = pStream->headSize;
	unsigned char* pStartData = pData;
    int i = 0;
	unsigned char* pEndData = pData + Len;
	
	for(i = 0; i < Len; i++)// the magic number is 4 '~'
	{
		if(0x7E == *pStartData)
		{
			headsize++;
			if(GNSS_MAGIC_NUMBER_LEN == headsize)//we got the 4 magic '~' 
			{
                pStartData++;
				memset(pStream->curHeader,GNSS_MAGIC_NUMBER,GNSS_MAGIC_NUMBER_LEN);
				pStream->state = GNSS_RECV_COLLECT_HEAD;
				break;
			} 
		}
		else
		{
			headsize = 0;
		}

	    pStartData++;

	}
    //if(pStartData == pEndData)
     //E("pdata is go to end\n");
	pStream->headSize = headsize;
	*pProcessedLen = pStartData - pData;
	
}

/*--------------------------------------------------------------------------
    Function:  GNSS_Checksum

    Description:
        it Calculate the CRC  for the 8 bytes in head bufferr 
        
    Parameters:
     pHeadData: point the head  data 
     
    Return: void 
----------------------------------------------------------------------------------*/

static unsigned short GNSS_Checksum(unsigned char* pHeadData)
{
    // The first 4 octet is 0x7e
    // 0x7e7e + 0x7e7e = 0xfcfc
    unsigned int  sum = 0xfcfc;
    unsigned short nAdd;

    pHeadData += 4;

    nAdd = *pHeadData++;
    nAdd <<= 8;
    nAdd += *pHeadData++;
    sum += nAdd;

    nAdd = *pHeadData++;
    nAdd <<= 8;
    nAdd += *pHeadData++;
    sum += nAdd;

    // The carry is 2 at most, so we need 2 additions at most.

    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short)(~sum);
}

/*--------------------------------------------------------------------------
    Function:  GNSS_DataChecksum

    Description:
        it auto fill the encode head context in   one packet  
    Parameters:
     pInData: point the send data context  
     pOutData : the one packet head address             
--------------------------------------------------------------------------*/
static unsigned short GNSS_DataChecksum(unsigned char* pData,
                                                   unsigned short outLen)
{
    unsigned int  sum = 0;
    unsigned short nAdd;

    if((NULL == pData) ||(0 == outLen))
    {
        return 0;
    }

    while(outLen > 2)
    {
        nAdd = *pData++;
        nAdd <<= 8;
        nAdd += *pData++;
        sum += nAdd;
        outLen -= 2;
    }
    if(2== outLen)
    {
        nAdd = *pData++;
        nAdd <<= 8;
        nAdd += *pData++;
        sum += nAdd;
    }
    else
    {
        nAdd = *pData++;
        nAdd <<= 8;
        sum += nAdd;
    }
    // The carry is 2 at most, so we need 2 additions at most.

    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = (sum & 0xFFFF) + (sum >> 16);

    return (unsigned short)(~sum);
}


static void researchFlag(TGNSS_DataProcessor_t* pStream)
{
    unsigned char* pStart = pStream->curHeader;


    if(0x7e == pStart[4])//slip the first '~' 
    {
        memmove(pStart + 4, pStart + 5, 5);
        pStream->headSize = 9;
        pStream->state = GNSS_RECV_COLLECT_HEAD;
    }
    else
    {
        unsigned int value4Byte;
        
        value4Byte = pStart[5];
        value4Byte <<= 8;
        value4Byte |= pStart[6];
        value4Byte <<= 8;
        value4Byte |= pStart[7];
        value4Byte <<= 8;
        value4Byte |= pStart[8];
        if(0x7e7e7e7e == value4Byte)//get the [5--8] is magic head 
        {
            pStart[4] = pStart[9];
            pStream->headSize = 5;
            pStream->state = GNSS_RECV_COLLECT_HEAD;
        }
        else  //get [6--9]
        {
            value4Byte <<= 8;
            value4Byte |= pStart[9];
            if(0x7e7e7e7e == value4Byte)
            {
                pStream->headSize = 4;
                pStream->state = GNSS_RECV_COLLECT_HEAD;
            }
            else if(0x7e7e7e == (0xffffff & value4Byte))
            {
                pStream->headSize = 3;
                pStream->state = GNSS_RECV_SEARCH_FLAG;
            }
            else if(0x7e7e == (0xffff & value4Byte))
            {
                pStream->headSize = 2;
                pStream->state = GNSS_RECV_SEARCH_FLAG;
            }
            else if(0x7e == (0xff & value4Byte))
            {
                pStream->headSize = 1;
                pStream->state = GNSS_RECV_SEARCH_FLAG;
            }
            else
            {
                pStream->headSize = 0;
                pStream->state = GNSS_RECV_SEARCH_FLAG;
            }
        }
    }
}


static void GNSS_CollectHeader(TGNSS_DataProcessor_t* pStream, unsigned char* pData, 
									 unsigned short Len, unsigned short* pProcessedLen)
{
    unsigned short headsize = pStream->headSize;
    unsigned short remainLen = GNSS_MAX_HEAD_LEN - headsize;
    unsigned short processedLen;
    unsigned short crc;
    unsigned short crcInFrame;
	
    processedLen = remainLen > Len ? Len : remainLen;
    memcpy(pStream->curHeader + headsize, pData, processedLen);
    headsize += processedLen;
    *pProcessedLen = processedLen;
	pStream->headSize = headsize;
	
    if(GNSS_MAX_HEAD_LEN != headsize)  // We have not got 10 bytes
    {
        return ;
    }
	
	//debuginfor(pStream->curHeader,GNSS_MAX_HEAD_LEN);
    // We have got 10 bytes
    // Calculate the checksum (only 8 bytes in head buffer)
    crc = GNSS_Checksum(pStream->curHeader);
    crcInFrame = pStream->curHeader[8];
    crcInFrame <<= 8;
    crcInFrame |= pStream->curHeader[9];
	
    if(crc == crcInFrame)  // We have got a right header
    {
        unsigned short dataLen;

        // Set the frame length here
        dataLen = pStream->curHeader[6];
        dataLen <<= 8;
        dataLen |= pStream->curHeader[7];
        pStream->dataLen = dataLen;
		pStream->cmdData.type = pStream->curHeader[4];
		pStream->cmdData.subType = pStream->curHeader[5];
		pStream->cmdData.length = dataLen;
		//E("GNSS_CollectHeader get the type =%d, subtype =%d\r\n",pStream->curHeader[4],pStream->curHeader[5]);
		if(0 == dataLen)
		{
			gps_dispatch(&pStream->cmdData);
			pStream->state = GNSS_RECV_SEARCH_FLAG;
			pStream->headSize = 0;
			pStream->receivedDataLen = 0;
			pStream->dataLen = 0;
		
		}
		else
		{
		    if(dataLen < (MAX_MSG_BUFF_SIZE - GNSS_MAX_HEAD_LEN - GNSS_MAX_DATA_CRC))
		    {
			    pStream->state = GNSS_RECV_DATA;
		    }
            else
            {
                pStream->errorNum++;
                researchFlag(pStream);
                E("GNSS_CollectHeader error = %d dataLen = %d\r\n",pStream->errorNum,dataLen);
            }
		}

    }
	else // This is a wrong header.Search from the second byte of the header
	{
		pStream->errorNum++;
		E("crcInFrame = 0x%x crc =0x%x\r\n",crcInFrame,crc);
		researchFlag(pStream);
		E("GNSS_CollectHeader error = %d\r\n",pStream->errorNum);
	}
    
    return ;
}


static int GNSS_CollectData(TGNSS_DataProcessor_t* pStream, unsigned char* pData, 
							unsigned short Len, unsigned short* pProcessedLen)
{
    unsigned short nFrameRemain = pStream->dataLen - pStream->receivedDataLen + GNSS_MAX_DATA_CRC;
    TCmdData_t* pPacket = &pStream->cmdData;
    unsigned short nCopy = nFrameRemain > Len ? Len : nFrameRemain;
    unsigned short dataCrc;
    unsigned short CrcInFrame;

    memcpy(pPacket->buff + pStream->receivedDataLen, pData, nCopy);
    pStream->receivedDataLen += nCopy;

    *pProcessedLen = nCopy;
    // Have we got the whole frame?
    if(pStream->receivedDataLen == (pStream->dataLen + GNSS_MAX_DATA_CRC))
    {
        dataCrc = GNSS_DataChecksum(pPacket->buff, pPacket->length);
        CrcInFrame = pPacket->buff[pPacket->length];
        CrcInFrame <<= 8;
        CrcInFrame |= pPacket->buff[(pPacket->length + 1)];
        if(dataCrc == CrcInFrame)
        {
            gps_dispatch(&pStream->cmdData);
        }
        else
        {
            if((GNSS_LIBGPS_DATA_TYPE== pPacket->type) &&
                ((GNSS_LIBGPS_MEMDUMP_DATA_SUBTYPE == pPacket->subType) ||
                (GNSS_LIBGPS_MEMDUMP_CODE_SUBTYPE == pPacket->subType)))
            {
                gps_dispatch(&pStream->cmdData);
            }
            else
            {
                D("The Data CheckSum error  type = %d, subtype = %d len =%d",
                  pPacket->type,pPacket->subType, pPacket->length);
                
                D("The Data CheckSum error CRC = %d CrcInFrame =%d",
                  dataCrc,CrcInFrame);
            }
        }
        pStream->state = GNSS_RECV_Complete;
		pStream->state = GNSS_RECV_SEARCH_FLAG;
		pStream->headSize = 0;
		pStream->receivedDataLen = 0;
		pStream->dataLen = 0;
		//E("recv one packet complete\n");
    }
   
    return 0;
}



/*--------------------------------------------------------------------------
    Function:  GNSS_InitOnePacket

    Description:
        the init GNSS parse data 
        
    Parameters:
     pStream: point the current  parse  data 
     
    Return:
        TRUE             One GNSS frame completed
        FALSE            One GNSS frame not completed
        Negetive        Error
--------------------------------------------------------------------------*/

void  GNSS_InitParsePacket(TGNSS_DataProcessor_t* pStream)
{
	pStream->state = GNSS_RECV_SEARCH_FLAG;
	pStream->headSize = 0;
	pStream->receivedDataLen = 0;
	pStream->dataLen = 0;
	pStream->errorNum = 0;
}


/*--------------------------------------------------------------------------
    Function:  GNSS_ParseOnePacket

    Description:
        Parse the input uart  data 
        
    Parameters:
     pStream: point the current  parse  data 
     UseData: point the uart buffer data 
     Len       : the receive data length
     
    Return:
        TRUE             One GNSS frame completed
        FALSE            One GNSS frame not completed
        Negetive        Error
--------------------------------------------------------------------------*/
int GNSS_ParseOnePacket(TGNSS_DataProcessor_t* pStream,  unsigned char* pData, unsigned short Len)
{
    unsigned char* pInput;
    unsigned short remainLen = 0; 
    unsigned short processedLen = 0;
	int nRet = 1;

	remainLen = Len;
	pInput = pData;

    //first check the memory 
	if((pStream == NULL) ||(pData == NULL)) return -1;

	while(remainLen)
	{
	        switch(pStream->state)
            {
        	    case GNSS_RECV_SEARCH_FLAG:	    
				GNSS_searchFlag(pStream, pInput, remainLen, &processedLen);
				break;
    		    
        	    case GNSS_RECV_COLLECT_HEAD:	    
				GNSS_CollectHeader(pStream,pInput,remainLen, &processedLen);
				break;
    		    
        	    case GNSS_RECV_DATA:
				GNSS_CollectData(pStream, pInput, remainLen, &processedLen);
				break;
                default: 
				break;
    		    
			}
		remainLen -= processedLen;
		pInput    += processedLen;
		
	}

        return nRet;
}


/*--------------------------------------------------------------------------
    Function:  GNSS_FillHead

    Description:
        it auto fill the encode head context in   one packet  
    Parameters:
     pInData: point the send data context  
     pOutData : the one packet head address             
--------------------------------------------------------------------------*/
void GNSS_FillHead(TCmdData_t* pIndata,unsigned char* pOutData)
{
 	unsigned char* pData = pOutData;
	unsigned short crc = 0;
	

	*pData++ =  GNSS_MAGIC_NUMBER;
	*pData++ =  GNSS_MAGIC_NUMBER;
	*pData++ =  GNSS_MAGIC_NUMBER;
	*pData++ =  GNSS_MAGIC_NUMBER;
	*pData++ =  pIndata->type;                            //type
	*pData++ =  pIndata->subType;
	*pData++ =  GNSS_GET_HIGH_BYTE(pIndata->length);       // len 
	*pData++ =  GNSS_GET_LOW_BYTE(pIndata->length);
     //calc crc 
	crc = GNSS_Checksum(pOutData);
	*pData++ = GNSS_GET_HIGH_BYTE(crc);
	*pData++ = GNSS_GET_LOW_BYTE(crc);
}


/*--------------------------------------------------------------------------
    Function:  GNSS_EncodeOnePacket

    Description:
        encode  one packet  
    Parameters:
     pInData: point the uart buffer data 
     pOutData : the one packet 
     outLen     : the the packet len 

     notes:  the  input data length <= 2038 ,or it must divide up many SubPackage 
     
    Return: the encode data len , or error code 
       
--------------------------------------------------------------------------*/
int GNSS_EncodeOnePacket( TCmdData_t* pInData,unsigned char* pOutData,unsigned short outLen)
{
    //unsigned char* pHead;
	int len = 0; 
    unsigned char* pCrcData = NULL;
    unsigned short dataCheckSum; 
    //check the input  data valid 
	if (NULL == pInData ||(pInData->length > MAX_MSG_BUFF_SIZE))
	{
		return -1;
	}

	if(outLen<= GNSS_MAX_HEAD_LEN)
	{
	    return -1;
	}
	
    //len = (outLen >= (pInData->dataLen + GNSS_MAX_HEAD_LEN))? (pInData->dataLen):( outLen-GNSS_MAX_HEAD_LEN);
    len =  pInData->length;
	//First fill the GNSSS head context 
	GNSS_FillHead(pInData,pOutData);
	if(len)
	{
		memcpy((pOutData+GNSS_MAX_HEAD_LEN),pInData->buff,len);
        dataCheckSum = GNSS_DataChecksum(pInData->buff,len);
        pCrcData = pOutData+GNSS_MAX_HEAD_LEN + len;
        *pCrcData++ = GNSS_GET_HIGH_BYTE(dataCheckSum);
        *pCrcData++ = GNSS_GET_LOW_BYTE(dataCheckSum);
	}
	
	len += GNSS_MAX_HEAD_LEN + GNSS_MAX_DATA_CRC;
	return len; 

}

