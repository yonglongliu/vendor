#include <math.h>
#include <stdio.h>
#include <string.h>
#include <cutils/log.h>
#include "nmeaenc.h"


#define SBAS_IN_USE_NUM 9
#define GPS_QZSS_TOTAL_NUM   (32 + 1)

unsigned int g_startmode_flg = 0;
char  g_llh_time_flg  = 0;
int gpsTow;
#define APPEND_CHAR(p, character) \
do \
{ \
	*(p) = (character); \
	(p) ++; \
} while(0)

const char ModeChar[] = { 'N', 'A', 'D', 'A', 'A', 'A', 'E', 'M', 'S' };  //lint !e19
const char ModeChar_ZY[] = { '5', '0', '1', '0', '0', '0', '2', '3', '4' };// 5 - 'N',not available
const char StatusChar[] = { 'V', 'A', 'A', 'A', 'A', 'A', 'V', 'V', 'A' };
const char TalkerType1[] = { 'G', 'B', 'B', 'G', 'G', 'G' };
const char TalkerType2[] = { 'P', 'D', 'D', 'A', 'L', 'P' };
const char *SentenceFormat[] = {
	"$GPGGA",
	"$GPGLL",
	"$GPGSA",
	"$GPGSV",
	"$GPRMC",
	"$GPVTG",
	"$GPZDA",
	"$GPGST",
	"$GPDHV",
};

const char OutputSequence[] = {
	NMEA_GGA,
    NMEA_RMC,
	NMEA_GLL,
	NMEA_GSA,
	NMEA_GSV,
	NMEA_VTG,
	NMEA_ZDA,
	NMEA_GST,
	NMEA_DHV,
};
NMEA_CP2AP g_NmeaDataCP2AP;
 

char *AppendInteger(char *p, int Value, int Digits)
{
	char Format[5] = "%0?d";

	if (Digits == 0)
	{
		Format[1] = 'd';
		Format[2] = 0;
	}
	else
	{
		Format[2] = '0' + Digits;
	}
	sprintf(p, Format, Value);
	return (p + strlen(p));
}
char *AppendFloatField(char *p, DOUBLE Value)
{
	int TempInt;

	APPEND_CHAR(p, ',');
	if (Value < 0)
	{
		Value = -Value;
		APPEND_CHAR(p, '-');
	}
	TempInt = (int)Value;
	p = AppendInteger(p, TempInt, 0);
	APPEND_CHAR(p, '.');
	TempInt = (int)((Value - TempInt) * 1000);
	p = AppendInteger(p, TempInt, 3);

	return p;
}
char *AppendFormat(char *p, int NmeaFormat)
{
	int PosFlag = g_NmeaDataCP2AP.PosFlag & 0xff;

	strcpy(p, SentenceFormat[NmeaFormat]);
	// change identifier according to positioning result
	if (PosFlag == 0)
		PosFlag = (g_NmeaDataCP2AP.PosFlag & 0xff00) >> 8;

	if (g_NmeaDataCP2AP.PvtSystemUsage == PVT_USE_GPS)
		return p + 6;
	else if (g_NmeaDataCP2AP.PvtSystemUsage == PVT_USE_GLONASS)
		p[2] = 'L';
	else if (g_NmeaDataCP2AP.PvtSystemUsage == PVT_USE_BD2)
	{
		p[1] = 'B';
		p[2] = 'D';
	}
	else
		p[2] = 'P';
	return p + 6;
}

char *AppendIntegerField(char *p, int Value)
{
	APPEND_CHAR(p, ',');
	if (Value != 0)
	{
		sprintf(p, "%d", Value);
		p += strlen(p);
	}
	else
		*p = 0;

	return p;
}
char *NmeaTerminate(char *String)
{
	unsigned char CheckSum = 0;

	String ++;		// skip the first '$'
	while (*String)
	{
		if (*String == '*' || *String == '\r' || *String == '\n')
			break;
		CheckSum ^= *String ++;
	}

	*String ++= '*';
	*String ++= (CheckSum >> 4) > 9 ? 'A' - 10 + (CheckSum >> 4) : '0' + (CheckSum >> 4);
	*String ++= (CheckSum & 0xf) > 9 ? 'A' - 10 + (CheckSum & 0xf) : '0' + (CheckSum & 0xf);
	*String ++= '\r';
	*String ++= '\n';
	return String;
}


char *ComposeGsv(unsigned char SatSystem, char *StringStart, int SatNumber, int NmeaVersion)
{
	unsigned char SvPrn[MAX_SV_IN_GSV];
	unsigned char SvEl[MAX_SV_IN_GSV];
	unsigned char SvCN0[MAX_SV_IN_GSV];
	short SvAz[MAX_SV_IN_GSV];
	signed char SvIndex[GPS_QZSS_TOTAL_NUM + SBAS_IN_USE_NUM];	// indicate whether corresponding satellite in GSV table
	int i, j, GsvSatNumber, GsvMsgNumber, MinPRN, TempInt;
	char *p = StringStart;
    char *pstart = StringStart;//@luqiang.liu 2015.06.25 output firmware data one line once
    unsigned char satNumAbove = 0;//@luqiang.liu 2015.11.28.25 Save the satellite number above
   
	if (g_NmeaDataCP2AP.systemSvNum[SatSystem] == 0)
		return StringStart;
    memset((void * )SvIndex, 0, sizeof(SvIndex));
    memset((void * )SvPrn, 0, sizeof(SvPrn));
    memset((void * )SvEl, 0, sizeof(SvEl));
    memset((void * )SvCN0, 0, sizeof(SvCN0));
    memset((void * )SvAz, 0, sizeof(SvAz));

    GsvSatNumber = g_NmeaDataCP2AP.systemSvNum[SatSystem];
    satNumAbove = 0;
    for(i=0;i<SatSystem;i++)
    {
        satNumAbove = satNumAbove + g_NmeaDataCP2AP.systemSvNum[i];
    }
    memcpy(&SvPrn,&(g_NmeaDataCP2AP.SvPrn[satNumAbove]), g_NmeaDataCP2AP.systemSvNum[SatSystem]);
    memcpy(&SvEl,&(g_NmeaDataCP2AP.SvEl[satNumAbove]), g_NmeaDataCP2AP.systemSvNum[SatSystem]);
    memcpy(&SvAz,&(g_NmeaDataCP2AP.SvAz[satNumAbove]), 2*g_NmeaDataCP2AP.systemSvNum[SatSystem]);
    memcpy(&SvCN0,&(g_NmeaDataCP2AP.SvCN0[satNumAbove]), g_NmeaDataCP2AP.systemSvNum[SatSystem]);
    
	GsvMsgNumber = (GsvSatNumber + 3) / 4;
	TempInt = 0;	// count number till GsvSatNumber
	for (i = 1; i <= GsvMsgNumber; i ++)
	{
	    memset(StringStart, 0, NMEA_STRING_LEN);
        pstart = StringStart;
		p = StringStart;
		*p ++ = '$';
		*p ++ = TalkerType1[SatSystem];
		*p ++ = TalkerType2[SatSystem];
		*p ++ = 'G';
		*p ++ = 'S';
		*p ++ = 'V';
		p = AppendIntegerField(p, GsvMsgNumber);
		p = AppendIntegerField(p, i);
		p = AppendIntegerField(p, GsvSatNumber);
		for (j = 0; j < 4; j ++)
		{
			if (TempInt >= GsvSatNumber)
				break;
			p = AppendIntegerField(p, SvPrn[TempInt]);
			p = AppendIntegerField(p, SvEl[TempInt]);
			p = AppendIntegerField(p, SvAz[TempInt]);
			p = AppendIntegerField(p, SvCN0[TempInt]);
			*p = 0;
			TempInt ++;
		}
		pstart = NmeaTerminate(pstart);
        //#ifndef _BASEBAND_CMODEL_
        //SEND_FIRMWARE_DATA((void *)StringStart, strlen(StringStart), GNSS_LIBGPS_DATA_TYPE, GNSS_LIBGPS_NMEA_SUBTYPE);
        //#endif  
        D("=====>>>>>%s,SatNumber:%d,NmeaVersion:%d",StringStart,SatNumber,NmeaVersion);
        parseNmea((unsigned char *)StringStart,strlen(StringStart));  
	}
    return StringStart;
}

int NmeaEncode(unsigned int NmeaMask, unsigned char NmeaVersion)
{
	char LatLonString[64] = {0};
	char AltituteString[64] = {0};
	char TimeString[32] = {0};
	char DateString[32] = {0};
	char SpeedString[32] = {0};
	char CourseString[32] = {0};
	char HdopString[32] = {0};
	char VdopString[32] = {0};
	char PdopString[32] = {0};
	char ModeString[4] = {0};
	char StatusString[4] = {0};
	char GstString[37] = {0};
    char nmea_string[256];	//@luqiang.liu  2015.06.15
	int Quality = 0;
	char *StringStart = nmea_string, *p;
    GpsState* s = _gps_state;

	DOUBLE Lat, Lon, ClkErr;
	DOUBLE Hdop, Vdop, Pdop,DopNmea[3];

	DOUBLE Speed =0;
	DOUBLE Course =0;
    DOUBLE tempspeed = 0;
	int TempInt;
	int SignFlag = 0;
	KINEMATIC_INFO PosVelRec;
	NLLH PosLLHRec;
	unsigned int NmeaType, OutNmeaType;
	int i, consFlag = 0;
    DOUBLE geoUndu = 0.0;
    BOOL bResValid = FALSE;
	U8 llh_time_flg = 0;

	D("NmeaEncode enter,mask:%d",NmeaMask);
	if(((g_startmode_flg == 0xff)&&(g_NmeaDataCP2AP.Quality>=1))||(g_llh_time_flg==1))
	{
	    llh_time_flg = 1;
		g_llh_time_flg = 1;
	}
	else if(g_startmode_flg != 0xff)
	{
	    llh_time_flg = 1;
	}
	else
	{
	    llh_time_flg = 0;
	}

   memcpy(&PosVelRec, &(g_NmeaDataCP2AP.PosVelRec), sizeof(KINEMATIC_INFO));
   memcpy(&PosLLHRec, &(g_NmeaDataCP2AP.PosLLHRec), sizeof(NLLH));
   
	if ((NmeaMask & (MSGGGA | MSGGLL | MSGRMC)) != 0)
	{   
	    if(llh_time_flg == 1)
		{
			Lat = PosLLHRec.lat * 180 / PI;
			Lon = PosLLHRec.lon * 180 / PI;
			if (Lat < 0)
			{
				Lat = -Lat;
				SignFlag |= 1;
			}
			if (Lon < 0)
			{
				Lon = -Lon;
				SignFlag |= 2;
			}
			p = LatLonString;
			APPEND_CHAR(p, ',');

		if( (int)((Lat - (int)Lat) * 600000000 + 5) >= 600000000 )
			TempInt = (int)(Lat + 1);
		else
			TempInt = (int)Lat;
		
		p = AppendInteger(p, TempInt, 2);
		TempInt = ((int)((Lat - TempInt) * 600000000) + 5) / 10;	// resolution to 1/1000000 minute, add one digit to round up
		p = AppendInteger(p, TempInt / 1000000, 2);
		APPEND_CHAR(p, '.');
		p = AppendInteger(p, TempInt % 1000000, 6);
		APPEND_CHAR(p, ',');
		APPEND_CHAR(p, (SignFlag & 1) ? 'S' : 'N');
		APPEND_CHAR(p, ',');

		if( (int)((Lon - (int)Lon) * 600000000 + 5) >= 600000000 )
			TempInt = (int)(Lon + 1);
		else
			TempInt = (int)Lon;

			p = AppendInteger(p, TempInt, 3);
			TempInt = ((int)((Lon - TempInt) * 600000000) + 5) / 10;	// resolution to 1/1000000 minute, add one digit to round up
			p = AppendInteger(p, TempInt / 1000000, 2);
			APPEND_CHAR(p, '.');
			p = AppendInteger(p, TempInt % 1000000, 6);
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, (SignFlag & 2) ? 'W' : 'E');
			APPEND_CHAR(p, 0);	
		}else
		{
		    p = LatLonString;
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, ',');
		}
	}

	// if altitute string needed
	if ((NmeaMask & (MSGGGA)) != 0)
	{
		//add geoid undulation for altitute
		p = AppendFloatField(AltituteString, PosLLHRec.hae + g_NmeaDataCP2AP.geoUndu);
		APPEND_CHAR(p, ',');
		APPEND_CHAR(p, 'M');
		APPEND_CHAR(p, ',');
		APPEND_CHAR(p, '0');
		APPEND_CHAR(p, ',');
		APPEND_CHAR(p, 'M');
		APPEND_CHAR(p, 0);
	}

	// if time string needed
	if ((NmeaMask & (MSGGGA | MSGGLL | MSGRMC | MSGZDA | MSGDHV | MSGGST)) != 0)
	{
		// millisecond part add .5 to round at 0.001s, output align with ms, do not add clock error
		p = TimeString;
		APPEND_CHAR(p, ',');
		if(llh_time_flg == 1)
		{
			p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Hour, 2);
			p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Minute, 2);
			p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Second, 2);
			APPEND_CHAR(p, '.');
			p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Millisecond, 3);	
		}

	}

	// if date string needed
	if ((NmeaMask & (MSGRMC)) != 0)
	{
		p = DateString;
		APPEND_CHAR(p, ',');

		if((g_NmeaDataCP2AP.RevUtcTime.Month != 0) || (g_NmeaDataCP2AP.RevUtcTime.Day !=0) || (g_NmeaDataCP2AP.RevUtcTime.Year != 0))
		{
			if (NmeaVersion == NMEA_VER_BD)
			{
				p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Month, 2);
				p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Day, 2);
			}
			else
			{
				p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Day, 2);
				p = AppendInteger(p, g_NmeaDataCP2AP.RevUtcTime.Month, 2);
			}
			p = AppendInteger(p, (g_NmeaDataCP2AP.RevUtcTime.Year % 100), 2);
		}
	}

    // if dop value needed
    if ((NmeaMask & (MSGGGA | MSGGSA)) != 0)
    {
        AppendFloatField(HdopString, g_NmeaDataCP2AP.Hdop);
        AppendFloatField(VdopString, g_NmeaDataCP2AP.Vdop);
        AppendFloatField(PdopString, g_NmeaDataCP2AP.Pdop);
    }

    // if speed and course needed
    if ((NmeaMask & (MSGRMC | MSGVTG | MSGDHV)) != 0)
    {
        Course = g_NmeaDataCP2AP.PosVelRec.Course;
        Speed  = g_NmeaDataCP2AP.PosVelRec.Speed;
        AppendFloatField(SpeedString, g_NmeaDataCP2AP.tempspeed * 3600 / 1852);
        AppendFloatField(CourseString, Course);
    }

     Quality = g_NmeaDataCP2AP.Quality;

	ModeString[0] = ',';
	if( NmeaVersion== NMEA_VER_ZY )
		ModeString[1] = ModeChar_ZY[Quality];
	else
		ModeString[1] = ModeChar[Quality];
	ModeString[2] = 0;
	StatusString[0] = ',';
	StatusString[1] = StatusChar[Quality]; //(ModeString[1] == 'A' || ModeString[1] == 'D') ? 'A' : 'V';
	StatusString[2] = 0;

	for(OutNmeaType = 0; OutNmeaType < MAX_NMEA_TYPE; OutNmeaType++)
	{
		NmeaType = OutputSequence[OutNmeaType];
		if (!(NmeaMask & (1 << NmeaType)))
			continue;

        memset(nmea_string, 0, sizeof(nmea_string));
        StringStart = nmea_string;//@luqiang.liu 2015.06.16 add for every nmea  output once 
		switch (NmeaType)
		{
		case NMEA_GGA:
			p = AppendFormat(StringStart, NmeaType);
			strcpy(p, TimeString);
			strcat(p, LatLonString);
			p += strlen(p);
			if (NmeaVersion == NMEA_VER_BD)
			{
				APPEND_CHAR(p, ',');
				p = AppendInteger(p, Quality, 0);
			}
			else
			{//when the position quality is 0, the message do not output the value.
				if(Quality)
					p = AppendIntegerField(p, Quality);
				else
				{
					APPEND_CHAR(p, ',');
					sprintf(p, "%d", Quality);
					p += strlen(p);
				}
			}
			
			APPEND_CHAR(p, ',');
			sprintf(p, "%02d",g_NmeaDataCP2AP.SatNum);// in order to output SatNum format of GGA is %2d.
			p += strlen(p);

		//	p = AppendIntegerField(p, g_PvtCoreData.SatInUse);
			strcat(p, HdopString);
			strcat(p, AltituteString);

			//add differential information
			
			strcat(p, ",,");

			if (NmeaVersion == NMEA_VER_BD)
				strcat(p, VdopString);
			StringStart = NmeaTerminate(StringStart);
                        s->NmeaReader[0].fix.accuracy = g_NmeaDataCP2AP.GSTParameter.ErrHorPos;

			break;

		case NMEA_GLL:
			p = AppendFormat(StringStart, NmeaType);
			strcpy(p, LatLonString);
			strcat(p, TimeString);
			strcat(p, StatusString);
			strcat(p, ModeString);
			StringStart = NmeaTerminate(StringStart);
			break;

		case NMEA_GSA:
		{
			unsigned int SysID = 0;
			int	PosUseSys = g_NmeaDataCP2AP.PvtSystemUsage; //g_ReceiverInfo.PosFlag & 0xff;
			int satcount = 0;
			int prn = 0;
			while(1)
			{
				if((PosUseSys & PVT_USE_GPS) != 0)
					SysID = GPS;
				else if((PosUseSys & PVT_USE_BD2) != 0)
					SysID = BD2;
				else if((PosUseSys & PVT_USE_GLONASS) != 0)
					SysID = GLO;

				p = AppendFormat(StringStart, NmeaType);
				*p ++ = ',';
				*p ++ = 'A';
				*p ++ = ',';
//				*p ++ = '3';

				//fix bug in the receiver position status of GSA output, SVN15303,hjwu
				if(Quality)
				{
					if(g_NmeaDataCP2AP.PosQuality == UnknownPos)
					{
						*p ++ = '1';
					}
					else if(g_NmeaDataCP2AP.PosQuality == FixedUpPos)
					{
						*p ++ = '2';
					}
					else if(((g_NmeaDataCP2AP.PosQuality >= ExtSetPos)&& (g_NmeaDataCP2AP.PosQuality <= KeepPos))||((g_NmeaDataCP2AP.PosQuality >= FlexTimePos)&& (g_NmeaDataCP2AP.PosQuality <= AccuratePos)))
					{
						*p ++ = '3';
					}
					else
					{
						*p ++ = '1';
					}
				}
				else
				{
					*p ++ = '1';
				}

				for (i = 0; i < g_NmeaDataCP2AP.SatNum; i ++)
				{
					if (satcount >= 12)
						break;
					if(! IS_GPS(g_NmeaDataCP2AP.ChannelList[i].FreqID) && (SysID == GPS) )
						continue;
					else if(! IS_BD2(g_NmeaDataCP2AP.ChannelList[i].FreqID) && (SysID == BD2) )
						continue;
					else if(! IS_GLO(g_NmeaDataCP2AP.ChannelList[i].FreqID) && (SysID == GLO) )
						continue;
					
					if (NmeaVersion == NMEA_VER_ZY)
					{
						if (SysID == BD2)
						{
							prn = g_NmeaDataCP2AP.ChannelList[i].prn - 160 + 50;
						}
						else if (SysID == GPS)
						{
							prn = g_NmeaDataCP2AP.ChannelList[i].prn;
						}
					}
					else
					{
                        if(SysID == GPS && IS_QZSS_PRN(g_NmeaDataCP2AP.ChannelList[i].prn))
				        {
				            prn = g_NmeaDataCP2AP.ChannelList[i].prn - MIN_QZSS_PRN + 193;
				        }
                        else  if(SysID == BD2)
						{
							prn = g_NmeaDataCP2AP.ChannelList[i].prn - MIN_BD2_PRN + 201;
						}
                        else                
				            prn = g_NmeaDataCP2AP.ChannelList[i].prn;                                       						
					}
					p = AppendIntegerField(p, prn);
					satcount++;
				}
				for (; satcount < 12; satcount ++)
				{
					APPEND_CHAR(p, ',');
				}
				*p = 0;

				if (NmeaVersion != NMEA_VER_BD)
					strcat(p, PdopString);
				strcat(p, HdopString);
				strcat(p, VdopString);
				if (NmeaVersion == NMEA_VER_BD)
					strcat(p, PdopString);
				StringStart = NmeaTerminate(StringStart);
				
				//for next GSA
				PosUseSys &= ~(0x01 << SysID); //lint !e502
				satcount = 0;
				if((PosUseSys & (PVT_USE_GPS | PVT_USE_BD2 | PVT_USE_GLONASS)) == 0)
					break;
			}
			break;
		}
		case NMEA_GSV:
          	if (g_NmeaDataCP2AP.PvtSystemUsage & (1 << GPS))
				StringStart = ComposeGsv(GPS, StringStart, g_NmeaDataCP2AP.systemSvNum[GPS], NmeaVersion);

            if(s->workmode == GPS_GLONASS){
			    if (g_NmeaDataCP2AP.PvtSystemUsage & (1 << GLO))
				    StringStart = ComposeGsv(GLO, StringStart, g_NmeaDataCP2AP.systemSvNum[GLO], NmeaVersion);
            }
            else{
			    if (g_NmeaDataCP2AP.PvtSystemUsage & (1 << BD2))
				    StringStart = ComposeGsv(BD2, StringStart, g_NmeaDataCP2AP.systemSvNum[BD2], NmeaVersion);
            }
			break;

		case NMEA_RMC:
			p = AppendFormat(StringStart, NmeaType);
			strcpy(p, TimeString);
			strcat(p, StatusString);
			strcat(p, LatLonString);
			strcat(p, SpeedString);
			strcat(p, CourseString);
			strcat(p, DateString);
			p += strlen(p);
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, ',');		// no magnetic variation
			APPEND_CHAR(p, 'E');

			strcpy(p, ModeString);
			StringStart = NmeaTerminate(StringStart);
			break;

		case NMEA_VTG:
			p = AppendFormat(StringStart, NmeaType);
			strcpy(p, CourseString);
			p += strlen(p);
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, 'T');
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, 'M');
			strcpy(p, SpeedString);
			p = p + strlen(p);
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, 'N');
			p = AppendFloatField(p, Speed * 3.6);
			APPEND_CHAR(p, ',');
			APPEND_CHAR(p, 'K');
			strcpy(p, ModeString);
			StringStart = NmeaTerminate(StringStart);
			break;

		case NMEA_ZDA:
		{
			int local_zone_hour;   //local_zone_minute;
			char temp_string[32];
			p = AppendFormat(StringStart, NmeaType);
			strcat(p, TimeString);

			local_zone_hour = (int)(ABS(PosLLHRec.lon) * 3.819718634205488);	// 12 hours / PI
		
			if (PosLLHRec.lon > 0)
				local_zone_hour = -local_zone_hour;

			sprintf(temp_string, ",%02d,%02d,%04d,%02d,%02d",
					g_NmeaDataCP2AP.RevUtcTime.Day, g_NmeaDataCP2AP.RevUtcTime.Month, g_NmeaDataCP2AP.RevUtcTime.Year, 0, 0);
			strcat(p, temp_string);
			StringStart = NmeaTerminate(StringStart);
		}
			break;

		case NMEA_DHV:
			if (NmeaVersion == NMEA_VER_BD)
			{
				p = AppendFormat(StringStart, NmeaType);
				strcpy(p, TimeString);
				Speed = PosVelRec.vx * PosVelRec.vx;
				Speed += PosVelRec.vy * PosVelRec.vy;
				Speed += PosVelRec.vz * PosVelRec.vz;
				Speed = sqrt(Speed);
				p += strlen(p);
				AppendFloatField(p, Speed * 3.6);
				p += strlen(p);
				AppendFloatField(p, PosVelRec.vx * 3.6);
				p += strlen(p);
				AppendFloatField(p, PosVelRec.vy * 3.6);
				p += strlen(p);
				AppendFloatField(p, PosVelRec.vz * 3.6);
				Speed = PosVelRec.Speed;//sqrt(SpeedE * SpeedE + SpeedN * SpeedN);
				p += strlen(p);
				AppendFloatField(p, Speed * 3.6);
				strcat(p, ",,,,,K");
				StringStart = NmeaTerminate(StringStart);
			}
			break;
		case NMEA_GST:
			p = AppendFormat(StringStart, NmeaType);
			strcpy(p, TimeString);
			sprintf(GstString,",%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",g_NmeaDataCP2AP.GSTParameter.rmsMSR >=100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.rmsMSR,
				g_NmeaDataCP2AP.GSTParameter.ErrSemiMajor >= 100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.ErrSemiMajor,
				g_NmeaDataCP2AP.GSTParameter.ErrSemiMinor >= 100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.ErrSemiMinor,
				g_NmeaDataCP2AP.GSTParameter.ErrOrientation*180/PI,
				g_NmeaDataCP2AP.GSTParameter.stdLat >=100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.stdLat,
				g_NmeaDataCP2AP.GSTParameter.stdLon >=100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.stdLon,
				g_NmeaDataCP2AP.GSTParameter.stdAlt >=100 ? 99.9 : g_NmeaDataCP2AP.GSTParameter.stdAlt);
			strcat(p,GstString);
			StringStart = NmeaTerminate(StringStart);
			break;
		default:
			break;
		}
		
		#if 0
        #ifndef _BASEBAND_CMODEL_
            if(NmeaType != NMEA_GSV)
        {   
            SEND_FIRMWARE_DATA((void *)nmea_string, strlen(nmea_string), GNSS_LIBGPS_DATA_TYPE, GNSS_LIBGPS_NMEA_SUBTYPE);      
            }          
        #else
            OUTPUT_MEAS(nmea_string);
        #endif
        #endif
        
        if(NmeaType != NMEA_GSV)
        {
            //D("=====>>>>>%s",nmea_string);
            parseNmea((unsigned char *)nmea_string,strlen(nmea_string));
        }
	}
	
	*StringStart ++ = 0;
	D("NmeaEncode end");
	return StringStart - nmea_string;
}
void nmea_parse_cp(unsigned char *buff,unsigned short lenth)
{
    D("nmea parse cp begin");
    if(lenth == sizeof(NMEA_CP2AP))
    {
        D("received nmea message,encode it");
        memcpy(&g_NmeaDataCP2AP,buff,lenth);
		gpsTow = g_NmeaDataCP2AP.GpsMsCount;
        NmeaEncode(g_NmeaDataCP2AP.NmeaMask, g_NmeaDataCP2AP.NmeaVersion);
    }
    else if(lenth == sizeof(NMEA_CP2AP)-4)
    {
        D("nmea type is include mscount,but firmwate is not send it");
        memcpy(&g_NmeaDataCP2AP,buff,lenth);
		gpsTow = 0;
        NmeaEncode(g_NmeaDataCP2AP.NmeaMask, g_NmeaDataCP2AP.NmeaVersion);
    }
    else
	{
		E("reived nmea lenth is error");
	}
    return;
    
    
}

