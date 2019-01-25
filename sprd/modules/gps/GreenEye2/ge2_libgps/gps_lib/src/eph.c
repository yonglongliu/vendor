#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>    

#include <sys/stat.h> 
#include <string.h>  
#include "gps_common.h"


void f(int n)
{
	if(n) f(n/2);
	else
	  return;
	printf("%d\n",n%2);
}

double ScaleDouble(int value, int scale)
{
	DOUBLE_INT_UNION data;

	data.d_data = (double)value;
	if (value != 0)
	{
#if defined (__GNUC__)
		data.i_data[1] -= (scale << 20);
#elif defined (_MSC_VER) || defined (__CC_ARM)
		data.i_data[1] -= (scale << 20);
#else
#error unknown compiler
#endif
	}

	return data.d_data;
}
void SET_BIT(unsigned int *outputData, signed int inputData,unsigned int pos, unsigned int len)
{
	signed int temp = 0;
	temp	=  inputData;
	if(temp < 0)
	{
		temp =((((~(-inputData))+1)<<(32-len))>>(32-pos-len))&(ONES(len)<<pos);
	}
	else
	{
		temp = temp<<pos;
	}

	*outputData |= temp;
}
signed long GET_BITS(signed long data, unsigned int pos, unsigned int len)
{
	signed long tempL = 0,tempR = 0;
	signed long IndexL = (32 - pos - len);
	signed long IndexR = 32-len;
	
    tempR =  (((signed long)data) << IndexL)>> IndexR;

	return tempR;
}
int GET_UBITS(unsigned int data, unsigned int pos, unsigned int len)
{
	unsigned int tempL = 0,tempR = 0;
	tempR = (data >> pos)&ONES(len);
	
	return tempR;
}

/*=========================== almanac encode begin ==========================*/
void GPS_IndexFramePage(int svid,int *frameid,int *pageid)
{
    if(svid <25)
	{
	   *frameid =5;
	   *pageid  =svid;	
	}
	else
	{
	   *frameid =4;
	   if(svid >28)
	     *pageid =svid -22;
	   else
	     *pageid =svid -23;	   
	}

}

int EncodeGpsAlmanac(GPSAlmanacElementRRLP_t *pAlm,GPS_ALMANAC_PACKED *pGpsAlmPacked)
{
    int frameid=0,pageid=0;
	
	unsigned int AF0temp1=0,AF0temp2=0;
	
	AF0temp1 =(pAlm->almanacAF0) &ONES(3);
	AF0temp2 =(pAlm->almanacAF0)>>3;
	
	GPS_IndexFramePage((pAlm->satelliteID+1),&frameid,&pageid);
	
//AF0 	
	SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][0],AF0temp1,8,3);
	SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][0],AF0temp2,22,8);
	
//AF1	
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][0],pAlm->almanacAF1,11,11);
	
//M0
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][1],pAlm->almanacM0,6,24);
	
//W 
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][2],pAlm->almanacW,6,24); 

//Omega0
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][3],pAlm->almanacOmega0,6,24); 

//APowerHalf--sqrtA
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][4],pAlm->almanacAPowerHalf,6,24);	

//SVhealth
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][5],pAlm->almanacSVhealth,6,8); 
	
//OmegaDot
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][5],pAlm->almanacOmegaDot,14,16);

//Ksii
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][6],pAlm->almanacKsii,6,16);
	
//Toa
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][6],pAlm->almanacToa,22,8);
	
//E 
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][7],pAlm->almanacE,6,16);
	
//satelliteID
    SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][7],pAlm->satelliteID+1,22,6);
//dataid
	SET_BIT(&pGpsAlmPacked->FramePageWord[frameid-4][pageid-1][7],1,28,2);
	return 1;
}

/*================================ almanac end ==============================*/

int DecodeBD2EphemerisD1(PBDS_EPHEMERIS pEph,PBDS_CLOCK_EPHEMERIS pClockEph, unsigned int *data)
{

	pEph->bdsToe = ((data[10] & 0x3) << 15) | ((data[28] & 0x3ff) << 5) | ((data[27] & 0x3e0000) >> 17);


	//pEph->iodc = iodc;
	pEph->bdsURAI = GET_UBITS(data[8], 0, 4);
	//pEph->week = GET_UBITS(data[7], 9, 13);
	pClockEph->bdsToc = ((data[7] & 0x1ff) << 8) | ((data[6] & 0x3fc000) >> 14);  
	pClockEph->bdsTgd1 =(GET_BITS(data[6], 4, 10));    //lint !e702 !e790
	//pEph->tgd2 = ((GET_BITS(data[6], 0, 4) << 6) | GET_UBITS(data[5], 16, 6)) * 1e-10; //lint !e702

	pClockEph->bdsA0 = (GET_BITS(data[2], 0, 7) << 17) | GET_UBITS(data[1], 5, 17);  //lint !e702        // .33    
	pClockEph->bdsA1 = (GET_BITS(data[1], 0, 5) << 17) | GET_UBITS(data[0], 5, 17); //lint !e702         // .50    
	pClockEph->bdsA2 = GET_BITS(data[2], 7, 11);  //lint !e702        // .66     
	//pClockEph->iode2 = iode;
	pEph->bdsDeltaN = (GET_BITS(data[18], 0, 10) << 6) | GET_UBITS(data[17], 16, 6); //lint !e702        	// .43 * PI 
	pEph->bdsCuc = (GET_BITS(data[17], 0, 16) << 2) | GET_UBITS(data[16], 20, 2);  //lint !e702        // .31  
	pEph->bdsM0 = ((data[16] & 0xfffff) << 12) | GET_UBITS(data[15], 10, 12); //lint !e702         // .31 * PI
	pEph->bdsE = ((data[15] & 0x3ff) << 22) | (data[14] & 0x3fffff); // .33
	pEph->bdsCus = GET_BITS(data[13], 4, 18);  //lint !e702        // .31  
	pEph->bdsCrc = (GET_BITS(data[13], 0, 4) << 14) | GET_UBITS(data[12], 8, 14); //lint !e702         // .6   
	pEph->bdsCrs = (GET_BITS(data[12], 0, 8) << 10) | GET_UBITS(data[11], 12, 10);  //lint !e702        // .6 
	pEph->bdsAPowerHalf = ((data[11] & 0xfff) << 20) | GET_UBITS(data[10], 2, 20);
	pEph->bdsI0 = ((data[27] & 0x1ffff) << 15) | GET_UBITS(data[26], 7, 15); // .31 * PI
	pEph->bdsCic = (GET_BITS(data[26], 0, 7) << 11) | GET_UBITS(data[25], 11, 11); //lint !e702         // .31 
	pEph->bdsOmegaDot = (GET_BITS(data[25], 0, 11) << 13) | GET_UBITS(data[24], 9, 13); //lint !e702         // .43 * PI
	pEph->bdsCis = (GET_BITS(data[24], 0, 9) << 9) | GET_UBITS(data[23], 13, 9); //lint !e702         // .31    //lint !e702
	pEph->bdsIDot = (GET_BITS(data[23], 0, 13) << 1) | GET_UBITS(data[22], 21, 1);  //lint !e702        // .43 * PI
	pEph->bdsOmega0 = ((data[22] & 0x1fffff) << 11) | GET_UBITS(data[21], 11, 11); // .31 * PI  
	pEph->bdsW = ((data[21] & 0x7ff) << 21) | GET_UBITS(data[20], 1, 21); // .31 * PI 

	return 1;
}
int EncodeBD2EphemerisD1(PBDS_EPHEMERIS pEph, PBDS_CLOCK_EPHEMERIS pClockEph, GPS_EPHEMERIS_PACKED *ephPacked)
{
	int ret = 1;
	int tmp1,tmp2;

	memset(ephPacked,0,sizeof(GPS_EPHEMERIS_PACKED));	
	// subframe 1:	
	tmp1 = pClockEph->bdsA1 & 0x1ffff;
	SET_BIT(&ephPacked->word[0],tmp1,5,17);

	tmp1 = pClockEph->bdsA0 & 0x1ffff;
	tmp2 = (pClockEph->bdsA1 >> 17) & ONES(5);
	SET_BIT(&ephPacked->word[1],tmp1,5,17);
	SET_BIT(&ephPacked->word[1],tmp2,0,5);

	tmp1 = (pClockEph->bdsA0 >> 17) & ONES(7);
	SET_BIT(&ephPacked->word[2],tmp1,0,7);
	SET_BIT(&ephPacked->word[2],pClockEph->bdsA2,7,11);

	SET_BIT(&ephPacked->word[6],pClockEph->bdsTgd1,4,10);
	ephPacked->word[6] |= (pClockEph->bdsToc << 14) & 0x3fc000;

	ephPacked->word[7] |= (pClockEph->bdsToc >> 8) & 0x1ff; 

	SET_BIT(&ephPacked->word[8],pEph->bdsURAI,0,4);
	
	tmp1 = pEph->bdsAPowerHalf & 0xfffff;
	ephPacked->word[10] |= (pEph->bdsToe >> 15) & 0x3;
	SET_BIT(&ephPacked->word[10],tmp1,2,20);


	tmp1 = pEph->bdsCrs & 0x3ff;
	SET_BIT(&ephPacked->word[11],tmp1,12,10);
	//ephPacked->word[11] |= (pEph->bdsAPowerHalf & 0xfff00000) >> 20;
    ephPacked->word[11] |= (pEph->bdsAPowerHalf >> 20) & 0xfff;
	tmp1 = pEph->bdsCrc & 0x3fff;
	tmp2 = (pEph->bdsCrs >> 10) & ONES(8);
	SET_BIT(&ephPacked->word[12],tmp1,8,14);
	SET_BIT(&ephPacked->word[12],tmp2,0,8);

	tmp1 = (pEph->bdsCrc >> 14) & ONES(4);
	SET_BIT(&ephPacked->word[13],pEph->bdsCus,4,18);
	SET_BIT(&ephPacked->word[13],tmp1,0,4);

	ephPacked->word[14] |= pEph->bdsE & 0x3fffff;

	tmp1 = pEph->bdsM0 & 0xfff;
	SET_BIT(&ephPacked->word[15],tmp1,10,12);
	//ephPacked->word[15] |= (pEph->bdsE & 0xffc00000) >> 22;
    ephPacked->word[15] |= (pEph->bdsE >> 22) & 0x3ff;  //ff3 to 3ff

	tmp1 = pEph->bdsCuc & 0x3;
	SET_BIT(&ephPacked->word[16],tmp1,20,2);
	//ephPacked->word[16] |= (pEph->bdsM0 & 0xfffff000) >> 12;   
    ephPacked->word[16] |= (pEph->bdsM0 >> 12) & 0xfffff;  

	tmp1 = pEph->bdsDeltaN & 0x3f;
	tmp2 = pEph->bdsCuc >> 2 & ONES(16);
	SET_BIT(&ephPacked->word[17],tmp1,16,6);
	SET_BIT(&ephPacked->word[17],tmp2,0,16);

	tmp1= pEph->bdsDeltaN >> 6;
	SET_BIT(&ephPacked->word[18],tmp1,0,10);

	tmp1 = pEph->bdsW & 0x1fffff;
	SET_BIT(&ephPacked->word[20],tmp1,1,21);

	tmp1 = pEph->bdsOmega0 & 0x7ff;
	SET_BIT(&ephPacked->word[21],tmp1,11,11);
	//ephPacked->word[21] |= (pEph->bdsW & 0xffe00000) >> 21;
    ephPacked->word[21] |= (pEph->bdsW >> 21) & 0x7ff;

	tmp1 = pEph->bdsIDot & 0x1;
	SET_BIT(&ephPacked->word[22],tmp1,21,1);
	//ephPacked->word[22] |= (pEph->bdsOmega0 & 0xfffff100) >> 11;
	ephPacked->word[22] |= (pEph->bdsOmega0 >> 11) & 0x1fffff;
	
	tmp1 = pEph->bdsCis & ONES(9);
	tmp2 = (pEph->bdsIDot >> 1) & ONES(13);
	SET_BIT(&ephPacked->word[23],tmp1,13,9);
	SET_BIT(&ephPacked->word[23],tmp2,0,13);

	tmp1 = (pEph->bdsCis >> 9) & ONES(9);
	tmp2 = pEph->bdsOmegaDot & ONES(13);
	SET_BIT(&ephPacked->word[24],tmp1,0,9);
	SET_BIT(&ephPacked->word[24],tmp2,9,13);

	tmp1 = (pEph->bdsOmegaDot >> 13) & ONES(11);
	tmp2 = pEph->bdsCic & ONES(11);
	SET_BIT(&ephPacked->word[25],tmp1,0,11);
	SET_BIT(&ephPacked->word[25],tmp2,11,11);

	tmp1 = pEph->bdsI0 & 0x7fff;
	tmp2 = (pEph->bdsCic >> 11) & ONES(7);
	SET_BIT(&ephPacked->word[26],tmp1,7,15);
	SET_BIT(&ephPacked->word[26],tmp2,0,7);
	
	
	ephPacked->word[27] |= (pEph->bdsToe << 17) & 0x3e0000;
	ephPacked->word[27] |= (pEph->bdsI0  >> 15)& 0x1ffff;
	
	ephPacked->word[28] |= (pEph->bdsToe >> 5) & 0x3ff;
	return 1;
}
int DecodeBD2EphemerisD2(PBDS_EPHEMERIS pEph,PBDS_CLOCK_EPHEMERIS pClockEph, unsigned int *data)
{

	pEph->bdsURAI = GET_UBITS(data[0], 18, 4);
	pClockEph->bdsToc = ((data[0] & 0x1f) << 12) | ((data[1] & 0x3ffc00) >> 10);
	pClockEph->bdsTgd1 =(GET_BITS(data[1], 0, 10));
	

	pClockEph->bdsA0 = (GET_BITS(data[7], 0, 12) << 12) | GET_UBITS(data[8], 10, 12);  
	pClockEph->bdsA1 = (GET_BITS(data[8], 6, 4) << 18) | GET_UBITS(data[9], 10, 18); 
	pClockEph->bdsA2 = (GET_BITS(data[9], 0, 10) << 1) | GET_UBITS(data[10], 21, 1); 

	pEph->bdsDeltaN = GET_BITS(data[10], 0, 16);
	pEph->bdsCuc = (GET_BITS(data[11], 8, 14) << 4) | GET_UBITS(data[12], 24, 4);
	pEph->bdsM0 = ((data[12] & 0xffffff) << 8) | GET_UBITS(data[13], 14, 8); 
	pEph->bdsCus = (GET_BITS(data[13], 0, 14) << 4) | GET_UBITS(data[14], 18, 4);
	pEph->bdsE = ((data[14] & 0x3ff00) << 14) | GET_UBITS(data[15], 6, 22);
	pEph->bdsAPowerHalf = ((data[15] & 0x3f) << 26) | ( (data[16] & 0x3fffff) << 4) | GET_UBITS(data[17], 18, 4);
	pEph->bdsCic = (GET_BITS(data[17], 8, 10) << 8) | GET_UBITS(data[18], 20, 8);
	pEph->bdsCis = GET_BITS(data[18], 2, 18);
	pEph->bdsToe = ((data[18] & 0x3) << 15) | ((data[19] & 0x3fff80) >> 7);
	pEph->bdsI0 = ((data[19] & 0x7f) << 25) | ((data[20] & 0x3fff00) << 3) | GET_UBITS(data[21], 17, 11); 

	pEph->bdsCrc = (GET_BITS(data[21], 0, 17) << 1) | GET_UBITS(data[22], 21, 1);
	pEph->bdsCrs = GET_BITS(data[22], 3, 18);
	pEph->bdsOmegaDot = (GET_BITS(data[22], 0, 3) << 21) | ((data[23] & 0x3fffc0) >> 1) | GET_UBITS(data[24], 23, 5);
	pEph->bdsOmega0 = ((data[24] & 0x7fffff) << 9) | GET_UBITS(data[25], 13, 9); 
	pEph->bdsW = ((data[25] & 0x1fff) << 19) | ((data[26] & 0x3fff00) >> 3) | GET_UBITS(data[27], 23, 5); 
	pEph->bdsIDot = GET_BITS(data[27], 9, 14);

	return 1;
}
int EncodeBD2EphemerisD2(PBDS_EPHEMERIS pEph, PBDS_CLOCK_EPHEMERIS pClockEph, GPS_EPHEMERIS_PACKED *ephPacked)
{
	int ret = 1;
	int tmp1,tmp2;

	memset(ephPacked,0x0,sizeof(GPS_EPHEMERIS_PACKED));	

	// subframe 1:	
	SET_BIT(&ephPacked->word[0],pEph->bdsURAI,18,4);
	ephPacked->word[0] |= (pClockEph->bdsToc >> 12) & 0x1f;

	SET_BIT(&ephPacked->word[1],pClockEph->bdsTgd1,0,10);
	ephPacked->word[1] |= (pClockEph->bdsToc << 10) & 0x3ffc00;

	tmp1= pClockEph->bdsA0 >> 12;
	SET_BIT(&ephPacked->word[7],tmp1,0,12);

	SET_BIT(&ephPacked->word[8],pClockEph->bdsA0,10,12);
	tmp1 = pClockEph->bdsA1 >> 18;
	SET_BIT(&ephPacked->word[8],tmp1,6,4);

	SET_BIT(&ephPacked->word[9],pClockEph->bdsA1,10,18);
	tmp1 = pClockEph->bdsA2 >> 1;
	SET_BIT(&ephPacked->word[9],tmp1,0,10);

	SET_BIT(&ephPacked->word[10],pClockEph->bdsA2,21,1);
	SET_BIT(&ephPacked->word[10],pEph->bdsDeltaN,0,16);

	tmp1 = pEph->bdsCuc >> 4;
	SET_BIT(&ephPacked->word[11],tmp1,8,14);

	SET_BIT(&ephPacked->word[12],pEph->bdsCuc,24,4);
	ephPacked->word[12] |= (pEph->bdsM0 >> 8) & 0xffffff;

	SET_BIT(&ephPacked->word[13],pEph->bdsM0,14,8);
	tmp1 = pEph->bdsCus >> 4;
	SET_BIT(&ephPacked->word[13],tmp1,0,14);

	SET_BIT(&ephPacked->word[14],pEph->bdsCus,18,4);
	ephPacked->word[14] |= (pEph->bdsE >> 14) & 0x3ff00;

	SET_BIT(&ephPacked->word[15],pEph->bdsE,6,22);
	ephPacked->word[15] |= (pEph->bdsAPowerHalf >> 26) & 0x3f; 
	
	ephPacked->word[16] |= (pEph->bdsAPowerHalf >> 4) & 0x3fffff;

	SET_BIT(&ephPacked->word[17],pEph->bdsAPowerHalf,18,4);
	tmp1 = pEph->bdsCic >> 8;
	SET_BIT(&ephPacked->word[17],tmp1,8,10);
	
	SET_BIT(&ephPacked->word[18],pEph->bdsCic,20,8);
	SET_BIT(&ephPacked->word[18],pEph->bdsCis,2,18);
	ephPacked->word[18] |= (pEph->bdsToe >> 15) & 0x3;

	ephPacked->word[19] |=(pEph->bdsToe << 7) & 0x3fff80;
	ephPacked->word[19] |=(pEph->bdsI0 >> 25) & 0x7f;

	ephPacked->word[20] |=(pEph->bdsI0 >> 3)  & 0x3fff00;

	SET_BIT(&ephPacked->word[21],pEph->bdsI0,17,11);
	tmp1 = pEph->bdsCrc >> 1;
	SET_BIT(&ephPacked->word[21],tmp1,0,17);

	SET_BIT(&ephPacked->word[22],pEph->bdsCrc,21,1);
	SET_BIT(&ephPacked->word[22],pEph->bdsCrs,3,18);
	tmp1 = pEph->bdsOmegaDot >> 21;
	SET_BIT(&ephPacked->word[22],tmp1,0,3);
	
	ephPacked->word[23] |= (pEph->bdsOmegaDot << 1) & 0x3fffc0;

	SET_BIT(&ephPacked->word[24],pEph->bdsOmegaDot,23,5);
	ephPacked->word[24] |= (pEph->bdsOmega0 >> 9) & 0x7fffff;

	SET_BIT(&ephPacked->word[25],pEph->bdsOmega0,13,9);
	ephPacked->word[25] |= (pEph->bdsW >> 19) & 0x1fff;

	ephPacked->word[26] |= (pEph->bdsW << 3) & 0x3fff00;

	SET_BIT(&ephPacked->word[27],pEph->bdsIDot,9,14);
	SET_BIT(&ephPacked->word[27],pEph->bdsW,23,5);

	return 1;
}
int Trans_Ephpacked(TCgAgpsEphemeris_t *epacked,TEphemeris_t *eph)
{
	if((epacked == NULL) || (eph == NULL))
	{
		E("eph ptr maybe null");
		return 0;
	}
	eph->SatID = epacked->SatID;
	eph->SatelliteStatus = 0;
	eph->IODE = epacked->IODE;
	eph->toe = epacked->toe;
	eph->week_no = epacked->week_no;
	eph->C_rc = epacked->C_rc;
	eph->C_rs = epacked->C_rs;
	eph->C_ic = epacked->C_ic;
	eph->C_is = epacked->C_is;
	eph->C_uc = epacked->C_uc;
	eph->C_us = epacked->C_us;
	eph->e = epacked->e;
	eph->M_0 = epacked->M_0;
	eph->SquareA = epacked->SquareA;
	eph->Del_n = epacked->Del_n;
	eph->OMEGA_0 = epacked->OMEGA_0;
	eph->OMEGA_dot = epacked->OMEGA_dot;
	eph->I_0 = epacked->I_0;
	eph->Idot = epacked->Idot;
	eph->omega = epacked->omega;
	eph->tgd = epacked->tgd;
	eph->t_oc = epacked->t_oc;
	eph->af0 = epacked->af0;
	eph->af1 = epacked->af1;
	eph->af2 = epacked->af2; 
	return 0;
}

int EncodeSvData(TEphemeris_t *eph, GPS_EPHEMERIS_PACKED *ephPacked)
{
	int ret = 1;

	memset(ephPacked,0x0,sizeof(GPS_EPHEMERIS_PACKED));

	// subframe 1:
	SET_BIT(&ephPacked->word[0],eph->af0,8,22);

	SET_BIT(&ephPacked->word[1],eph->af1,6,16);
	SET_BIT(&ephPacked->word[1],eph->af2,22,8);

	SET_BIT(&ephPacked->word[2],eph->t_oc,6,16);

	SET_BIT(&ephPacked->word[3],eph->tgd,6,8);
	SET_BIT(&ephPacked->word[4],0,8,22);//reserved
	SET_BIT(&ephPacked->word[5],0,8,22);//reserved
	SET_BIT(&ephPacked->word[6],0,8,22);//reserved
	
	SET_BIT(&ephPacked->word[7],0,6,2);//iodc
	SET_BIT(&ephPacked->word[7],eph->SatelliteStatus,8,6);
	SET_BIT(&ephPacked->word[7],0,14,4);//URA
	SET_BIT(&ephPacked->word[7],0,18,2);//CAorP
	SET_BIT(&ephPacked->word[7],eph->week_no,20,10);

	SET_BIT(&ephPacked->word[8],0,8,22);//HOW
	SET_BIT(&ephPacked->word[9],0,8,22);//TLM
	
	// subframe 2:
	SET_BIT(&ephPacked->word[10],0,8,5);//AODO
	SET_BIT(&ephPacked->word[10],0,13,1);//fit interval flag
	SET_BIT(&ephPacked->word[10],eph->toe,14,16);

	unsigned int SquareA24bit = 0,SquareA8bit = 0;
	SquareA24bit = (eph->SquareA)&ONES(24);
	SquareA8bit = (eph->SquareA)>>24;

	SET_BIT(&ephPacked->word[11],SquareA24bit,6,24);

	SET_BIT(&ephPacked->word[12],SquareA8bit,6,8);
	SET_BIT(&ephPacked->word[12],eph->C_us,14,16);

	unsigned int e24bit = 0,e8bit = 0;
    e24bit = (eph->e)&ONES(24);
    e8bit = (eph->e)>>24;
	SET_BIT(&ephPacked->word[13],e24bit,6,24);

	SET_BIT(&ephPacked->word[14],e8bit,6,8);
    SET_BIT(&ephPacked->word[14],eph->C_uc,14,16);

	unsigned int M024bit = 0,M08bit = 0;
	M024bit = (eph->M_0)&ONES(24);
	M08bit = (eph->M_0)>>24;

	SET_BIT(&ephPacked->word[15],M024bit,6,24);

    SET_BIT(&ephPacked->word[16],M08bit,6,8);
	SET_BIT(&ephPacked->word[16],eph->Del_n,14,16);

	SET_BIT(&ephPacked->word[17],eph->C_rs,6,16);
	SET_BIT(&ephPacked->word[17],eph->IODE,22,8);

	SET_BIT(&ephPacked->word[18],0,8,22);//HOW
	SET_BIT(&ephPacked->word[19],0,8,22);//TLM

	// subframe 3:
	SET_BIT(&ephPacked->word[20],eph->Idot,8,14);
	SET_BIT(&ephPacked->word[20],eph->IODE,22,8);

	SET_BIT(&ephPacked->word[21],eph->OMEGA_dot,6,24);

	unsigned int w024bit = 0,w08bit = 0;
	w024bit = (eph->omega)&ONES(24);
    w08bit = (eph->omega)>>24;

	SET_BIT(&ephPacked->word[22],w024bit,6,24);

	SET_BIT(&ephPacked->word[23],w08bit,6,8);
	SET_BIT(&ephPacked->word[23],eph->C_rc,14,16);

	unsigned int i024bit = 0,i08bit = 0;
	i024bit = (eph->I_0)&ONES(24);
	i08bit = (eph->I_0)>>24;

	SET_BIT(&ephPacked->word[24],i024bit,6,24);

	SET_BIT(&ephPacked->word[25],i08bit,6,8);
	SET_BIT(&ephPacked->word[25],eph->C_is,14,16);

	unsigned int OMEGA024bit = 0,OMEGA08bit = 0;
	OMEGA024bit = (eph->OMEGA_0)&ONES(24);
	OMEGA08bit =  (eph->OMEGA_0)>>24;

	SET_BIT(&ephPacked->word[26],OMEGA024bit,6,24);
	SET_BIT(&ephPacked->word[27],OMEGA08bit,6,8);
	SET_BIT(&ephPacked->word[27],eph->C_ic,14,16);

	SET_BIT(&ephPacked->word[28],0,8,22);//HOW
	SET_BIT(&ephPacked->word[29],0,8,22);//TLM
	
	return ret;
}
int DecodeSvData(/*input*/GPS_EPHEMERIS_PACKED *ephPacked,/*output*/TEphemeris_t *eph)
{
	int ret = 1;

	memset(eph,0x0,sizeof(TEphemeris_t));
    // subframe 1:
	eph->af0 = GET_BITS(ephPacked->word[0],8,22);
	eph->af1 = GET_BITS(ephPacked->word[1],6,16);
	eph->af2 = GET_BITS(ephPacked->word[1],22,8);

	eph->t_oc = GET_UBITS(ephPacked->word[2],6,16);

	eph->tgd = GET_BITS(ephPacked->word[3],6,8);
	GET_UBITS(ephPacked->word[4],8,22);//reserved
	GET_UBITS(ephPacked->word[5],8,22);//reserved
	GET_UBITS(ephPacked->word[6],8,22);//reserved
	
	eph->IODE = GET_UBITS(ephPacked->word[7],6,2);//iodc
	eph->SatelliteStatus = GET_UBITS(ephPacked->word[7],8,2);
	GET_UBITS(ephPacked->word[7],14,4);//URA
	GET_UBITS(ephPacked->word[7],18,2);//CAorP
	eph->week_no = GET_UBITS(ephPacked->word[7],20,10);

	GET_UBITS(ephPacked->word[8],8,22);//HOW
	GET_UBITS(ephPacked->word[9],8,22);//TLM

	// subframe 2:
	GET_BITS(ephPacked->word[10],8,5);//AODO
	GET_BITS(ephPacked->word[10],13,1);//fit interval flag

	eph->toe = GET_UBITS(ephPacked->word[10],14,16);

	unsigned int SquareA24bit = 0,SquareA8bit = 0;

	SquareA24bit = GET_UBITS(ephPacked->word[11],6,24);

	SquareA8bit = GET_UBITS(ephPacked->word[12],6,8);

	eph->SquareA = (SquareA8bit<<24)|SquareA24bit;

	eph->C_us = GET_BITS(ephPacked->word[12],14,16);

	unsigned int e24bit = 0,e8bit = 0;

	e24bit = GET_UBITS(ephPacked->word[13],6,24);

	e8bit = GET_UBITS(ephPacked->word[14],6,8);

	eph->e =(e8bit<<24)|e24bit; 

    eph->C_uc = GET_BITS(ephPacked->word[14],14,16);

    unsigned int M024bit = 0,M08bit = 0;

	M024bit =GET_UBITS(ephPacked->word[15],6,24);

    M08bit = GET_UBITS(ephPacked->word[16],6,8);

	eph->M_0 = (M08bit<<24)|M024bit; 

	eph->Del_n = GET_BITS(ephPacked->word[16],14,16);

	eph->C_rs = GET_BITS(ephPacked->word[17],6,16);
	eph->IODE = GET_UBITS(ephPacked->word[17],22,8);


	GET_BITS(ephPacked->word[18],8,22);//HOW
	GET_BITS(ephPacked->word[19],8,22);//TLM
	// subframe 3:
	eph->Idot = GET_BITS(ephPacked->word[20],8,14);
	eph->IODE = GET_UBITS(ephPacked->word[20],22,8);

	eph->OMEGA_dot = GET_BITS(ephPacked->word[21],6,24);

	unsigned int w024bit = 0,w08bit = 0;

	w024bit = GET_BITS(ephPacked->word[22],6,24);

	w08bit = GET_BITS(ephPacked->word[23],6,8);
	eph->omega = (w08bit<<24)|w024bit;

	eph->C_rc = GET_BITS(ephPacked->word[23],14,16);

    unsigned int i024bit = 0,i08bit = 0;

	i024bit = GET_UBITS(ephPacked->word[24],6,24);

	i08bit = GET_UBITS(ephPacked->word[25],6,8);
	eph->I_0 = (i08bit<<24)|i024bit;

	eph->C_is = GET_BITS(ephPacked->word[25],14,16);

	unsigned int OMEGA024bit = 0,OMEGA08bit = 0;

	OMEGA024bit = GET_BITS(ephPacked->word[26],6,24);
	OMEGA08bit = GET_BITS(ephPacked->word[27],6,8);
	eph->OMEGA_0 = (OMEGA08bit<<24)|OMEGA024bit;

	eph->C_ic = GET_BITS(ephPacked->word[27],14,16);

	GET_BITS(ephPacked->word[28],8,22);//HOW
	GET_BITS(ephPacked->word[29],8,22);//TLM

	return ret;
}

