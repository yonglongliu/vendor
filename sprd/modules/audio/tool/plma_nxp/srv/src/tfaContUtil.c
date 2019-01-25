/*
 * tfaContUtil.c
 *
 *  container file utility  functions
 *    used for tools and not necessary for customer integration
 *
 *  Created on: July, 2014
 *      Author: Shankar
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include "dbgprint.h"
#include "tfaContainerWrite.h"
#include "tfaOsal.h"
#include "NXP_I2C.h"
#include "tfa98xx_tfafieldnames.h"
#include "tfa_service.h"
#include "Tfa98API.h"
#include "tfa.h"
#include "config.h"

unsigned char  tfa98xxI2cSlave=TFA_I2CSLAVEBASE; // global for i2c access

void (*timer_func_handler_pntr)(void);

void tfa98xxI2cSetSlave(unsigned char slave) {
	tfa98xxI2cSlave = slave;
}

static void tfa_dump_words(char *name, int offset, uint8_t *p8, int len) {
	int i,j,words, rest;
	uint32_t *p32 = (uint32_t *)p8;

	words = len/4;
	rest = len%4;

	for (i=0;i<words;i++,p32++) {
		PRINT("0x%x (%d) \t0x", (4*i+offset),(4*i+offset));
		p8 = (uint8_t*)p32;
		for (j=0;j<4;j++) { /* print in big endian */
			PRINT("%02x",0xff & *p8++);
		}
		p8 = (uint8_t*)p32;
		PRINT(" ");
		for (j=0;j<4;j++,p8++) { /* print in big endian */
				PRINT("%c",isprint(*p8) ? *p8: ' ');
		}
		if (name && i==0)
			PRINT(" : %s", name);

		PRINT("\n");
	}
	if (rest) {
		PRINT("0x%x (%d) \t0x", (4*i+offset),(4*i+offset));
		p8 = (uint8_t*)p32;
		for (i=0;i<rest;i++) {
			PRINT("%02x", 0xff & *p8++);
		}
		for (i=0;i<4-rest+1+2;i++) {
			PRINT(" ");
		}
		p8 = (uint8_t*)p32;
		for (i=0;i<rest;i++,p8++) {
			PRINT("%c",isprint(*p8) ? *p8: ' ');
		}
		PRINT("\n");
	}

}
static char *dsc_name[]={
		"dscDevice",
		"dscProfile",
		"dscRegister",
		"dscString",
		"dscFile",
		"dscPatch",
		"dscMarker",
		"dscMode",
		"dscSetInputSelect",
		"dscSetOutputSelect",
		"dscSetProgramConfig",
		"dscSetLagW",
		"dscSetGains",
		"dscSetvBatFactors",
		"dscSetSensesCal",
		"dscSetSensesDelay",
		"dscBitfield",
		"dscDefault",
		"dscLiveData",
		"dscLiveDataString",
		"dscGroup",
		"dscCmd"
};
/*
 * this is the sort for the varitems list: nxpTfaDescPtr_t *varitems[]
 *  the input is a pointer to the dsc reference and the sort is done
 *  based on the offset of the dsc.
 *
*/
static int cmpfunc (const void * a, const void * b)
{
	nxpTfaDescPtr_t *A, *B;
	uint32_t aa,bb;

	A = *((nxpTfaDescPtr_t **)a);
	B = *((nxpTfaDescPtr_t **)b);
	aa = A->offset;
	bb = B->offset;
	/* sort the pointers as int value */
	if ( aa <  bb )
		return -1;
	if ( aa == bb )
		return 0;
	if ( aa >  bb )
		return 1;

	return 1;
}

int tfa_cnt_is_varitem(nxpTfaDescriptorType_t type) {
	switch(type) {
	/* NOT varitems are: */
	case dscDevice:
	case dscProfile:
	case dscMarker:
	case dscDefault:
	case dscLiveData:
	case dscGroup:
		return 0;
	default:
		return 1;
	}
}

void tfa_cnt_dump()
{
	nxpTfaContainer_t *cnt = tfa98xx_get_cnt();
	nxpTfaDeviceList_t *dev;
	nxpTfaProfileList_t *prof;
	nxpTfaLiveDataList_t *lData;
	nxpTfaDescPtr_t * dsc;
	uint8_t *p8;
	int i, item=0, offset, len;
	unsigned int j;
	char str[256]; //line
	/* this list will collect all the descriptors pointing to a variable length item */
	nxpTfaDescPtr_t *varitems[1000];//variable length itemlist TODO better guess max length

	if (cnt == NULL){
		PRINT_ERROR("No container file loaded\n");
		return;
	}

	p8 = (uint8_t *)cnt;
	offset=0;
	tfa_dump_words("hdr", offset, p8, sizeof(*cnt));
	// devlist[]
	offset = sizeof(*cnt); /* skip cnt header */
	p8 += offset ;

	for(i=0; i<cnt->ndev; i++) {
		dsc=(nxpTfaDescPtr_t *)p8;
		sprintf(str, "devs[%d] type=%s offset=0x%x (%d)", i, dsc_name[dsc->type], dsc->offset, dsc->offset);
		tfa_dump_words(str, offset, p8, 4);
		offset+=4;
		p8+=4;
	}
	tfa_dump_words("marker", offset, p8, 4);
	offset+=4;
	p8+=4;
	// devs
	for(i=0; i<cnt->ndev; i++) {
			dev = (nxpTfaDeviceList_t *)p8;
			sprintf(str, "device[%d] (%s)", i,  tfaContGetString(&dev->name));
			varitems[item++] = &dev->name;
			tfa_dump_words(str, offset, p8, sizeof(nxpTfaDeviceList_t));
			offset+=sizeof(nxpTfaDeviceList_t);
			p8+=sizeof(nxpTfaDeviceList_t);
			// devlist
			for(j=0; j< dev->length; j++) {
				dsc=(nxpTfaDescPtr_t *)p8;
				sprintf(str, "list[%u] type=%s offset=0x%x (%d)", j, dsc_name[dsc->type], dsc->offset, dsc->offset);
				tfa_dump_words(str, offset, p8, 4);
				offset+=4;
				p8+=4;
				if (tfa_cnt_is_varitem(dsc->type)) {
					varitems[item++] = dsc;
				}
				if ((dsc->type==dscFile)||(dsc->type==dscPatch)) { /* a file name is a varitem as well */
					dsc = (nxpTfaDescPtr_t *)((uint8_t *)cnt+dsc->offset); /* the file name */
					varitems[item++] = dsc;
				}
			}
		}
	//profiles
	for (i=0; i< cnt->nprof; i++) {
		prof = (nxpTfaProfileList_t *)p8;
		sprintf(str, "profile[%i] (%s)", i, tfaContGetString(&prof->name));
		varitems[item++] = &prof->name;
		tfa_dump_words(str, offset, p8, sizeof(nxpTfaProfileList_t));
		offset+=sizeof(nxpTfaProfileList_t);
		p8+=sizeof(nxpTfaProfileList_t);
		// devlist
		for(j=0; j< prof->length-1u; j++) {//note that the proflist length includes the name item
			dsc=(nxpTfaDescPtr_t *)p8;
			sprintf(str, "list[%u] type=%s offset=0x%x (%d)", j, dsc_name[dsc->type], dsc->offset, dsc->offset);
			tfa_dump_words(str, offset, p8, 4);
			offset+=4;
			p8+=4;
			if (tfa_cnt_is_varitem(dsc->type)) {
				varitems[item++] = dsc;
			}
			if ((dsc->type==dscFile)||(dsc->type==dscPatch)) { /* a file name is a varitem as well */
				dsc = (nxpTfaDescPtr_t *)((uint8_t *)cnt+dsc->offset); /* the file name */
				varitems[item++] = dsc;
			}
		}
	}
	tfa_dump_words("marker", offset, p8, 4);
	offset+=4;
	p8+=4;
//	varitems[item++] = p8;
	// livedata
	for (i=0; i< cnt->nliveData; i++) {
		lData = (nxpTfaLiveDataList_t *)p8;
		sprintf(str, "livedata[%i] (%s)", i, tfaContGetString(&lData->name));
		varitems[item++] = &lData->name;
		tfa_dump_words(str, offset, p8, sizeof(nxpTfaLiveDataList_t));
		offset+=sizeof(nxpTfaLiveDataList_t);
		p8+=sizeof(nxpTfaLiveDataList_t);
		// devlist
		for(j=0; j< lData->length-1u; j++) {//note that the list length includes the name item
			dsc=(nxpTfaDescPtr_t *)p8;
			sprintf(str, "list[%u] type=%s offset=0x%x (%d)", j, dsc_name[dsc->type], dsc->offset, dsc->offset);
			tfa_dump_words(str, offset, p8, 4);
			offset+=4;
			p8+=4;
			if (tfa_cnt_is_varitem(dsc->type)) { /* save varitems  */
				varitems[item++] = dsc;
			}
		}
	}
	if (cnt->nliveData) {
		tfa_dump_words("marker", offset, p8, 4);
		offset+=4;
		p8+=4;
	//	varitems[item++] = (uint8_t *)cnt+dsc->offset;
	}
	tfa_dump_words("-0000-", offset, p8, 4);
	offset+=4;
	p8+=4;
	varitems[item]=NULL;

	/* the  varitems list holds all the items from here
	 * this need to be printed but first the list must be sorted
	 * Then we have the order in which they are organized
	 */
	// variable length items
	// sort first
	qsort(varitems, item, sizeof(nxpTfaDescPtr_t * ), cmpfunc);

	for(i=0;i<item-1;i++) {
		len = varitems[i+1]->offset - varitems[i]->offset;
		p8 = (uint8_t*)cnt + varitems[i]->offset;
		tfa_dump_words(dsc_name[varitems[i]->type] , varitems[i]->offset, p8, len); //types disconnected after sort
//		tfa_dump_words("-varitem-", offset, p8, len);
		if(len<0) {
			PRINT_ERROR("illegal data encountered!\n");
			return;
		}
	}
	/* now print the last one */
	len = cnt->size - varitems[i]->offset; /* it should be the remainder of the container */
	p8 = (uint8_t*)cnt + varitems[i]->offset;
	tfa_dump_words(dsc_name[varitems[i]->type] , varitems[i]->offset, p8, len);

}
void tfa_cnt_hexdump()
{
	nxpTfaContainer_t *cnt = tfa98xx_get_cnt();

	if (cnt == NULL){
		PRINT_ERROR("No container file loaded\n");
		return;
	}
	tfa_dump_words(" : container header", 0, (uint8_t*)cnt, cnt->size);
}

void tfaContShowSpeaker(nxpTfaSpeakerFile_t *spk) {
	tfaContShowHeader( &spk->hdr);

	PRINT("Speaker Info: name=%.8s vendor=%.16s type=%.8s\n", spk->name, spk->vendor, spk->type);
	if(spk->ohm_secondary < 1) {
		PRINT("  dimensions: height=%dmm width=%dmm depth=%dmm primary=%dohm \n", 
				spk->height, spk->width ,spk->depth, spk->ohm_primary);
	} else {
		PRINT("  dimensions: height=%dmm width=%dmm depth=%dmm primary=%dohm secondary=%dohm\n", 
				spk->height, spk->width ,spk->depth, spk->ohm_primary, spk->ohm_secondary);
	}

}

void tfaContGetHdr(char *inputname, nxpTfaHeader_t *hdrbuffer)
{
	nxpTfaHeader_t *localbuffer = 0;

	tfaReadFile(inputname, (void**) &localbuffer);
	
	if(localbuffer) {
		memcpy(hdrbuffer, localbuffer, sizeof(nxpTfaHeader_t));
		free(localbuffer);
	}
}

void tfa_cnt_get_spk_hdr(char *inputname, struct nxpTfaSpkHeader *hdrbuffer)
{
	struct nxpTfaSpeakerHdr *localbuffer = 0;

	tfaReadFile(inputname, (void**) &localbuffer);
	
	if(localbuffer) {
		memcpy(hdrbuffer, localbuffer, sizeof(struct nxpTfaSpkHeader));
		free(localbuffer);
	}
}


/*
 * show the contents of the local container
 * Takes length and char pointer to store the ini file contents
 * Return error code
 */
Tfa98xx_Error_t tfaContShowContainer(char *strings, int maxlength )
{
	nxpTfaContainer_t *cnt = tfa98xx_get_cnt();
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	nxpTfaDeviceList_t *dev;
	nxpTfaProfileList_t *prof;
	nxpTfaLiveDataList_t *lData;
	char str[NXPTFA_MAXLINE];
	int i, k, devidx=0;
	unsigned int j;

	if (cnt == NULL){
		PRINT_ERROR("No container file loaded\n");
		return Tfa98xx_Error_Bad_Parameter;
	}

	sprintf(str, "[system]\n" "customer=%.8s\n" "application=%.8s\n" "type=%.8s\n", cnt->customer, cnt->application, cnt->type);
	err = tfa_append_substring(str, strings, maxlength);

	for(i=0; i<cnt->ndev; i++) {
		if ( (dev=tfaContDevice(i)) !=NULL) {
			sprintf(str, "device=%s\n", tfaContGetString(&dev->name));
			err = tfa_append_substring(str, strings, maxlength);
		}
	}

	sprintf(str, "\n");
	err = tfa_append_substring(str, strings, maxlength);

	// show devices
	for(i=0; i<cnt->ndev; i++) {
		if ( (dev=tfaContDevice(i)) !=NULL) {

			tfaContShowDevice(i, strings, maxlength);

			for (j=0; j < dev->length; j++ ) {
				err = tfaContShowItem(&dev->list[j], strings, maxlength, i);
				if(err != Tfa98xx_Error_Ok)
					return err;
			}

			sprintf(str, "\n");
			err = tfa_append_substring(str, strings, maxlength);
		}
	}

	// show all profiles
	prof=tfaContGet1stProfList(cnt);
	for (i=0; i<cnt->nprof; i++) {		
		sprintf(str, "[%s]\n", tfaContGetString(&prof->name));
		err = tfa_append_substring(str, strings, maxlength);

		/* We need to look up the devidx for each profile */
		for(k=0; k<cnt->ndev; k++) {
			if ( (dev=tfaContDevice(k)) !=NULL) {
				for (j=0; j < dev->length; j++ ) {
					/* Compare device item with profile name */
					if(tfa_search_profile(&dev->list[j], tfaContGetString(&prof->name)) == 1) {
						devidx = k;
						goto next_profile; /* Only way possible to break a nested loop? */
					}
				}
			}
		}

next_profile:
		// next profile
		for(j=0; j< prof->length-1u; j++) { //note that the proflist length includes the name item
			err = tfaContShowItem(&prof->list[j], strings, maxlength, devidx);
			if(err != Tfa98xx_Error_Ok)
				return err;
		}

		sprintf(str, "\n");
		err = tfa_append_substring(str, strings, maxlength);
		prof = tfaContNextProfile(prof);
	}

	// show all livedata items
	lData=tfaContGet1stLiveDataList(cnt);
	for (i=0; i<cnt->nliveData; i++) {
		sprintf(str, "[%s]\n", tfaContGetString(&lData->name));
		err = tfa_append_substring(str, strings, maxlength);

		/* We need to look up the devidx for each profile */
		for(k=0; k<cnt->ndev; k++) {
			if ( (dev=tfaContDevice(k)) !=NULL) {
				for (j=0; j < dev->length; j++ ) {
					/* Compare device item with profile name */
					if(tfa_search_profile(&dev->list[j], tfaContGetString(&lData->name)) == 1) {
						devidx = k;
						goto next_livedata; /* Only way possible to break a nested loop? */
					}
				}
			}
		}

next_livedata:

		// next livedata item
		for(j=0; j<lData->length-1u; j++) {
			err = tfaContShowItem(&lData->list[j], strings, maxlength, devidx);
			if(err != Tfa98xx_Error_Ok)
				return err;
		}

		sprintf(str, "\n");
		err = tfa_append_substring(str, strings, maxlength);
		lData = tfaContNextLiveData(lData);
	}

	return err;
}


/*
 * show file info
 */
void tfaContShowFile(nxpTfaHeader_t *hdr) {
	tfaContShowHeader( hdr);
}
/*
 * show the file from the loaded container
 */
void tfaContShowFileDsc(nxpTfaFileDsc_t *f) {
	PRINT("name=%s, size=%d\n", tfaContGetString(&f->name), f->size);
	tfaContShowHeader( (nxpTfaHeader_t*)&f->data);
}

/*
 * display the contents of the devicelist from the container global
 *  return Tfa98xx_Error_Ok if ok
 */
enum Tfa98xx_Error tfaContShowDevice(int idx, char *strings, int maxlength) 
{
	enum Tfa98xx_Error err;
	nxpTfaDeviceList_t *dev;
	char str[NXPTFA_MAXLINE];

	if ( idx >= tfa98xx_cnt_max_device() )
		return Tfa98xx_Error_Bad_Parameter;

	if ( (dev = tfaContDevice(idx)) == NULL )
		return Tfa98xx_Error_Bad_Parameter;

	sprintf(str, "[%s]\n",  tfaContGetString(&dev->name));
	err = tfa_append_substring(str, strings, maxlength);

	sprintf(str, "bus=%d\n" "dev=0x%02x\n",    dev->bus, dev->dev);
	err = tfa_append_substring(str, strings, maxlength);

	if ( dev->devid) {
		sprintf(str, "devid=0x%08x\n", dev->devid);
		err = tfa_append_substring(str, strings, maxlength);
	}

	return err;
}        

enum Tfa98xx_Error tfaGetDspFWAPIVersion(int devidx, char *buffer, int maxlength)
{
	enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
        char str[NXPTFA_MAXLINE];
		int FW_version=0, length=0;
		
		/* 72 device */
		if((tfa98xx_dev_revision(devidx) & 0xff ) == 0x72) {
			err = tfa_dsp_cmd_id_write_read(devidx,MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION,3,(unsigned char *)str);
			FW_version = (str[2]<<6) | (str[1]<<8) |(str[0] << 16);
			if (err) {
				PRINT("%s Error: Unable to get API Version from DSP \n", __FUNCTION__);
				return err;
			}
		} else {
			/* Get XML ID */
			err = mem_read(devidx, FW_VAR_API_VERSION, 1, &FW_version);
			if (err) {
				PRINT("%s Error: Unable to get API Version from DSP \n",__FUNCTION__);
				return err;
			}
		}

        sprintf(str, "FW API version: %d.%d.%d\n", (FW_version>>16) & 0xff, 
                                                (FW_version>>8) & 0xff, ((FW_version>>6) & 0x03));

		err = tfa_append_substring(str, buffer, maxlength);
        if (  length > maxlength )
                return Tfa98xx_Error_Bad_Parameter; // max length too short

        return err;
}

/*
 * get tag
 *
 */
enum Tfa98xx_Error tfaGetDspTag(int devidx, char *string, int *size)
{
    enum Tfa98xx_Error err87;
    enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
    int i;
    unsigned char tag[TFA98XX_MAXTAG];

    *size = 0;

    // interface should already be opened
    err87 = tfa_dsp_cmd_id_write_read(devidx, MODULE_FRAMEWORK, SB_PARAM_GET_TAG, TFA98XX_MAXTAG, tag);

    PRINT_ASSERT(err87);

    if (err87 == Tfa98xx_Error_Ok) {
        // the characters are in every 3rd byte
        for ( i=2 ; i<TFA98XX_MAXTAG ; i+=3) 
        {
                if ( isprint(tag[i]) ) {
                        *string++ = tag[i];    // only printable chars
                        (*size)++;
                        if ( tag[i] == '>' && tfa98xx_dev_family(devidx) == 2) {
			        break;
			}
                }
        }
        *string = '\0';
    }

    if (err87 == Tfa98xx_Error_DSP_not_running)
        return Tfa98xx_Error_DSP_not_running;
    else if (err87 == Tfa98xx_Error_Bad_Parameter)
        return Tfa98xx_Error_Bad_Parameter;

    return err;
}
/*
 * return version strings
 */
enum Tfa98xx_Error tfaVersions( Tfa98xx_handle_t *handlesIn, char *buffer, int maxlength )
{
	enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
	char str[NXPTFA_MAXLINE],str1[NXPTFA_MAXLINE];
	int length=0, i;
	unsigned short reg;

	// tfa device and hal layer rev
	err = tfa_append_substring(str, buffer, maxlength);

	// chip rev
	err = reg_read(handlesIn[0], 0x03, &reg); //TODO define a rev somewhere
	PRINT_ASSERT(err);	

	sprintf(str, "Tfa98xx HW  rev: 0x%04x\n", reg);
        err = tfa_append_substring(str, buffer, maxlength);

        // coolflux ROM rev
        /*clear the buffer*/
		memset(&str[0], 0, sizeof(str));
		err = tfaGetDspTag(handlesIn[0], str, &i);
        if ( err != Tfa98xx_Error_Ok)
                return err;
        
        length += i;

        if ( length > maxlength )
                return Tfa98xx_Error_Bad_Parameter; // max length too short

        sprintf(str1, "DSP revstring: \"%s\"\n", str);
        err = tfa_append_substring(str1, buffer, maxlength);
		
		tfaGetRegMapVersion(handlesIn[0], (char*)str);
        sprintf(str1, "I2C register map version: %s", str);
        err = tfa_append_substring(str1, buffer, maxlength);
	if ( err != Tfa98xx_Error_Ok )
			return err; // max length too short

		return err;
}
/*
 * prints I2C register map version
 */
int tfaGetRegMapVersion(int devidx, char *str)
{
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	unsigned short revid=0;
	revid = tfa98xx_get_device_revision(devidx);
	switch(revid & 0xff){
	/*9890*/
	case 0x80:
		sprintf(str, "%d\n",TFA9890_I2CVERSION );
		break;
	/*9895*/
	case 0x12:
		sprintf(str, "%d\n",TFA9895_I2CVERSION);
		break;
	/*9897*/
	case 0x97:
		sprintf(str, "%d\n",TFA9897_I2CVERSION);
		break;
	/*9896*/
	case 0x96:
		sprintf(str, "%d\n",TFA9896_I2CVERSION);
		break;
	/*9891*/
	case 0x92:
		sprintf(str, "%d\n",TFA9891_I2CVERSION);
		break;
	/*9888*/
	case 0x88:
		sprintf(str, "%d\n",TFA9888_I2CVERSION);
		break;
	/*9872*/
	case 0x72:
		switch(revid) {
		case 0x1a72:
		case 0x2a72:
			sprintf(str, "%d\n",TFA9872_I2CVERSION_N1A);
			break;
		case 0x1b72:
			sprintf(str, "%d\n",TFA9872_I2CVERSION_N1B);
			break;
		case 0x2b72:
		case 0x3b72:
			sprintf(str, "%d\n",TFA9872_I2CVERSION_N1B2);
		break;
		default:
			sprintf(str, "invalid revision-id \n");
		}
	/*9912*/
	case 0x13:
		sprintf(str, "%.2f\n",TFA9912_I2CVERSION);
		break;
	default:
		sprintf(str, "device unknown\n");
		break;
	};
	return err;
}

/*
 * save dedicated device files. Depends on the file extension
 */
int tfa98xxSaveFileWrapper(Tfa98xx_handle_t handle, char *filename)
{
	char *ext;
	nxpTfa98xxParamsType_t ftype = tfa_no_params;

	/* get filename extension */
	ext = strrchr(filename, '.');

	if ( ext == NULL ) {
	    PRINT("Cannot find file %s type requested.\n" , filename);
	    PRINT("Example:<filename>.vstep or <filename>.speaker \n");
	    return Tfa98xx_Error_Other;
	} else {
		/* Look for supported type */
		if ( strstr(filename, ".speaker") != NULL ) {
			ftype = tfa_speaker_params;
		} else if ( strstr(filename, ".vstep") != NULL ) {
			ftype = tfa_vstep_params;
		} else if ( strstr(filename, ".drc") != NULL ) {
			ftype = tfa_drc_params;
		} else if ( strstr(filename, ".eq") != NULL ) {
			ftype = tfa_equalizer_params;
		} else if ( strstr(filename, ".config") != NULL ) {
			ftype = tfa_config_params;
		} else if ( strstr(filename, ".preset") != NULL ) {
			ftype = tfa_preset_params;
		} else if ( strstr(filename, ".algo") != NULL ) {
			ftype = tfa_algo_params;
		} else {
			ftype = tfa_no_params;
		}
	}

	if ( ftype == tfa_no_params )
		return Tfa98xx_Error_Other;

	return tfa98xxSaveFile(handle, filename, ftype);
}
/*
 * save dedicated params
 */
int tfa98xxSaveFile(Tfa98xx_handle_t handle, char *filename, nxpTfa98xxParamsType_t params)
{
	enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
	char buffer[TFA2_ALGOPARAMETER_LENGTH*5] = {0}; //TODO properly fix size
	unsigned char buffer_9912[TFA2_ALGOPARAMETER_LENGTH*5] = {0};
	char *headerInfo[12];
	int c=0, FW_version=0, value, length=0, offset, supportDrc, fileLength, argc;
	int algo_length=0;
	nxp_vstep_msg_t *nxp_vstep_msg;
	FILE *f;

	/*
	 * call the specific setter functions
	 * length = the file length for the specific device family
	 * offset = the buffer offset (where to begin)
	 */
	switch ( params ) {
		case tfa_speaker_params:
			if ( tfa98xx_dev_family(handle) == 2) {
				/* 72 device */
				if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION,3,(unsigned char *)buffer);
					if (err) {
						PRINT("%s Error: Unable to get API Version from DSP \n", __FUNCTION__);
						return err;
					}
				} else {
					/* Get XML ID */
					err = mem_read(handle, FW_VAR_API_VERSION, 1, &FW_version);
					buffer[0] = (FW_version>>16) & 0xff;
					buffer[1] = (FW_version>>8) & 0xff;
					buffer[2] = (FW_version>>6) & 0x03;
					if (err) {
						PRINT("%s Error: Unable to get API Version from DSP \n", __FUNCTION__);
						return err;
					}

				}

				/* Add CMD ID */
				buffer[3] = 0;
				buffer[4] = MODULE_SPEAKERBOOST + 128;
				buffer[5] = SB_PARAM_SET_LSMODEL;

				/* 72 device stere length is same as 88 */
				if(((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) && (tfa98xx_cnt_max_device() == 1)) {
					length = TFA2_SPEAKERPARAMETER_LENGTH; /* Mono */
				} else {
					if ((tfa98xx_dev_revision(handle) & 0xff )==0x13){
						length = tfa_speaker_info(handle);
						if(length==0){
							printf("Speaker file missing from cnt file");
							return Tfa98xx_Error_Other;
						}
					}
					else{
						length = TFA2_SPEAKERPARAMETER_LENGTH * 2;
					}
				}
			

				offset = 6; /* 3 bytes xml_id + 3 bytes cmd_id */
			} else {
				length = TFA1_SPEAKERPARAMETER_LENGTH;
				offset = 0;
			}

			/* Get payload from both channels */
			err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_LSMODEL,length, (unsigned char *)buffer + offset);
			if (err){
				PRINT("%s Error: Getting LS Model \n", __FUNCTION__);
				return err;
			}
			f = fopen( filename, "wb");
			if (!f) {
				PRINT("Unable to open %s\n", filename);
				return Tfa98xx_Error_Other;
			}
			c = (int)(fwrite(buffer, (length + offset), 1, f));
			fclose(f);
			break;
		case tfa_vstep_params:
			if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72 || (tfa98xx_dev_revision(handle) & 0xff ) == 0x88) {
			if ( tfa98xx_dev_family(handle) == 2) {
				/* 72 device */
				if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_FRAMEWORK,FW_PAR_ID_GET_API_VERSION,3,(unsigned char *)buffer);
					if (err) {
						PRINT("%s Error: Unable to get API Version from DSP \n", __FUNCTION__);
						return err;
					}
				} else {
					/* Get XML ID */
					err = mem_read(handle, FW_VAR_API_VERSION, 1, &FW_version);
					buffer[0] = (FW_version>>16) & 0xff;
					buffer[1] = (FW_version>>8) & 0xff;
					buffer[2] = (FW_version>>6) & 0x03;
				}

				/* Number of vsteps -> (always 1 in this case!) */
				buffer[3] = 1;

				/* 72 device */
				if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
					/* Number of registers -> (0 for 72) */
					buffer[4] = 0;

					err = tfa98xx_dsp_support_drc(handle, &supportDrc);
					if (err){
						PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
						return err;
					}
					/* Number of messages -> (default is 2, can be 3 if DRC enabled) */
					if(supportDrc)
						buffer[5] = 3;
					else
						buffer[5] = 2;

					/* Message 0: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer[6] = 0;

					/* Message 0: message length (3 bytes)
					 * Add 3 bytes because cmd id is part of length
					 */
					if(tfa98xx_cnt_max_device() > 1) { /* Stereo */
						algo_length = TFA72_ALGOPARAMETER_LENGTH_STEREO;
					} else { /* Mono */
						algo_length = TFA72_ALGOPARAMETER_LENGTH_MONO;
					}

					buffer[7] = (((algo_length+3)/3)>>16) & 0xff;
					buffer[8] = (((algo_length+3)/3)>>8) & 0xff;
					buffer[9] = ((algo_length+3)/3) & 0xff;

					/* Message 0: command id  */
					if(tfa98xx_cnt_max_device() > 1) {
						buffer[10] = 00;
					} else { /* Mono */		
						buffer[10] = 04;
					}

					buffer[11] = MODULE_SPEAKERBOOST + 128;
					buffer[12] = SB_PARAM_SET_ALGO_PARAMS;
					length = algo_length;

					/* Message 0: data (message_length bytes) -> GetAlgoParam */
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_ALGO_PARAMS,length,(unsigned char *)buffer + 13);
					if (err) {
						PRINT("Error: Unable to get algoparams data from DSP \n");
						return err;
					}

					/* Message 1: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer[length + 13] = 1;

					/* Message 1: message length (3 bytes) */
					buffer[length + 14] = (((TFA72_FILTERCOEFSPARAMETER_LENGTH+6)/3)>>16) & 0xff;
					buffer[length + 15] = (((TFA72_FILTERCOEFSPARAMETER_LENGTH+6)/3)>>8) & 0xff;
					buffer[length + 16] = ((TFA72_FILTERCOEFSPARAMETER_LENGTH+6)/3) & 0xff;

					/* Stereo */
					if(tfa98xx_cnt_max_device() > 1) {
						/* Message 1: command id (6 bytes)  */
						buffer[length + 17] = 0;
						
					} else { /* Mono */
						/* Message 1: command id (6 bytes)  */
						buffer[length + 17] = 04;
					}

					buffer[length + 18] = MODULE_BIQUADFILTERBANK + 128;
					buffer[length + 19] = BFB_PAR_ID_SET_COEFS;

					/* Second command id is always zero (for now) */
					buffer[length + 20] = 0;
					buffer[length + 21] = 0;
					buffer[length + 22] = 0;

					/* Message 1: data (message_length bytes) -> GetCoeff 
					 * GetCoeffs for 72 is same size for mono and stereo
					 */
					err = tfa_dsp_cmd_id_coefs(handle,MODULE_BIQUADFILTERBANK,BFB_PAR_ID_GET_COEFS, 
					TFA72_FILTERCOEFSPARAMETER_LENGTH, (unsigned char *)buffer + length + 23);

					if (err) {
						PRINT("%s Error: Unable to get Coeff data from DSP \n", __FUNCTION__);
						return err;
					}

					if(supportDrc) {
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 23] = 2;

						/* Message 2: message length (3 bytes)
						 * Add 6 bytes because cmd id is part of length */
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 24] = (((TFA72_MBDRCPARAMETER_LENGTH+6)/3)>>16) & 0xff;
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 25] = (((TFA72_MBDRCPARAMETER_LENGTH+6)/3)>>8) & 0xff;
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 26] = ((TFA72_MBDRCPARAMETER_LENGTH+6)/3) & 0xff;

						/* Message 2: command id  */
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 27] = 0;
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 28] = MODULE_SPEAKERBOOST + 128;
						buffer[algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 29] = SB_PARAM_SET_MBDRC;

						/* Message 2: data (message_length bytes) -> MBDrc */
						err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_MBDRC, 
						   TFA72_MBDRCPARAMETER_LENGTH, (unsigned char *)buffer + algo_length + TFA72_FILTERCOEFSPARAMETER_LENGTH + 30);
						if (err) {
							PRINT("%s Error: Unable to get MBDrc data from DSP \n", __FUNCTION__);
							return err;
						}
					}
				} else {
					/* Number of registers -> (always 2) */
					buffer[4] = 2;

					/* The name + value (FIXED TO VOLSEC) */
					buffer[5] = 90;
					buffer[6] = 07;
					value = TFA_GET_BF(handle, VOLSEC);
					buffer[7] = (value>>8) & 0xff;
					buffer[8] = value & 0xff;

					/* The name + value (FIXED TO VOL) */
					buffer[9] = 81;
					buffer[10] = 135;
					value = TFA_GET_BF(handle, VOL);
					buffer[11] = (value>>8) & 0xff;
					buffer[12] = value & 0xff;

					err = tfa98xx_dsp_support_drc(handle, &supportDrc);
					if (err){
						PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
						return err;
					}
					/* Number of messages -> (default is 2, can be 3 if DRC enabled) */
					if(supportDrc)
						buffer[13] = 3;
					else
						buffer[13] = 2;

					/* Message 0: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer[14] = 0;

					/* Message 0: message length (3 bytes)
					 * Add 3 bytes because cmd id is part of length
					 */
					buffer[15] = (((TFA2_ALGOPARAMETER_LENGTH+3)/3)>>16) & 0xff;
					buffer[16] = (((TFA2_ALGOPARAMETER_LENGTH+3)/3)>>8) & 0xff;
					buffer[17] = ((TFA2_ALGOPARAMETER_LENGTH+3)/3) & 0xff;

					/* Message 0: command id  */
					buffer[18] = 0;
					buffer[19] = MODULE_SPEAKERBOOST + 128;
					buffer[20] = SB_PARAM_SET_ALGO_PARAMS_WITHOUT_RESET;

					/* Message 0: data (message_length bytes) -> GetAlgoPara */
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_ALGO_PARAMS, 
									TFA2_ALGOPARAMETER_LENGTH, (unsigned char *)buffer + 21);
					if (err) {
						PRINT("%s Error: Unable to get algoparams data from DSP \n", __FUNCTION__);
						return err;
					}

					/* Message 1: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer[TFA2_ALGOPARAMETER_LENGTH + 21] = 1;

					/* Message 1: message length (3 bytes)
					 * Add 6 bytes because cmd id is part of length */
					buffer[TFA2_ALGOPARAMETER_LENGTH + 22] = (((TFA2_FILTERCOEFSPARAMETER_LENGTH+6)/3)>>16) & 0xff;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 23] = (((TFA2_FILTERCOEFSPARAMETER_LENGTH+6)/3)>>8) & 0xff;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 24] = ((TFA2_FILTERCOEFSPARAMETER_LENGTH+6)/3) & 0xff;

					/* Message 1: command id (6 bytes)  */
					buffer[TFA2_ALGOPARAMETER_LENGTH + 25] = 0;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 26] = MODULE_BIQUADFILTERBANK + 128;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 27] = BFB_PAR_ID_SET_COEFS;

					/* Second command id is always zero (for now) */
					buffer[TFA2_ALGOPARAMETER_LENGTH + 28] = 0;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 29] = 0;
					buffer[TFA2_ALGOPARAMETER_LENGTH + 30] = 0;

					/* Message 1: data (message_length bytes) -> GetCoeff */
					err = tfa_dsp_cmd_id_coefs(handle,MODULE_BIQUADFILTERBANK,BFB_PAR_ID_GET_COEFS, 
						TFA2_FILTERCOEFSPARAMETER_LENGTH, (unsigned char *)buffer + TFA2_ALGOPARAMETER_LENGTH + 31);
					if (err) {
						PRINT("%s Error: Unable to get Coeff data from DSP \n", __FUNCTION__);
						return err;
					}

					if(supportDrc) {
						/* Message 2: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 31] = 2;

						/* Message 2: message length (3 bytes)
						 * Add 6 bytes because cmd id is part of length */
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 32] = (((TFA2_MBDRCPARAMETER_LENGTH+6)/3)>>16) & 0xff;
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 33] = (((TFA2_MBDRCPARAMETER_LENGTH+6)/3)>>8) & 0xff;
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 34] = ((TFA2_MBDRCPARAMETER_LENGTH+6)/3) & 0xff;

						/* Message 2: command id (6 bytes)  */
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 35] = 0;
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 36] = MODULE_SPEAKERBOOST + 128;
						buffer[TFA2_ALGOPARAMETER_LENGTH + TFA2_FILTERCOEFSPARAMETER_LENGTH + 37] = SB_PARAM_SET_MBDRC;

						/* Message 2: data (message_length bytes) -> MBDrc */
						err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_MBDRC, 
						   TFA2_MBDRCPARAMETER_LENGTH, (unsigned char *)buffer + TFA2_ALGOPARAMETER_LENGTH + 
													TFA2_FILTERCOEFSPARAMETER_LENGTH + 38);
						if (err) {
							PRINT("%s Error: Unable to get MBDrc data from DSP \n", __FUNCTION__);
							return err;
						}
					}
				}
				
				f = fopen(filename, "wb");
				if (!f) {
					PRINT("Unable to open %s\n", filename);
					return Tfa98xx_Error_Other;
				}

				/* 72 device */
				if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
					if(supportDrc) {
						c = (int)(fwrite(buffer, TFA72_FILTERCOEFSPARAMETER_LENGTH + length + TFA72_MBDRCPARAMETER_LENGTH + 30, 1, f));		
					} else {
						c = (int)(fwrite(buffer, TFA72_FILTERCOEFSPARAMETER_LENGTH + length + 23, 1, f));
					}			
				} else {
					if(supportDrc) {
						c = (int)(fwrite(buffer, TFA2_FILTERCOEFSPARAMETER_LENGTH +
							TFA2_ALGOPARAMETER_LENGTH + TFA2_MBDRCPARAMETER_LENGTH + 38, 1, f ));			
					} else {
						c = (int)(fwrite(buffer, TFA2_FILTERCOEFSPARAMETER_LENGTH+TFA2_ALGOPARAMETER_LENGTH + 31, 1, f));
					}
				}

				fclose(f);
			} else {
				/* Max1 vstep file = preset + eq */
				PRINT("Error: Is is not possible to get save the VSTEP file for max1 \n");
				return Tfa98xx_Error_Bad_Parameter;
			}
			}else
				if ( (tfa98xx_dev_revision(handle) & 0xff ) == 0x13) {
					uint16_t i, counter=0;
					nxp_vstep_msg = tfa_vstep_info(handle);
					if (nxp_vstep_msg==NULL){
						printf("Missing vstep file from container file\n");
						return Tfa98xx_Error_Bad_Parameter;
					}

					/* Get XML ID */
					err = mem_read(handle, FW_VAR_API_VERSION, 1, &FW_version);
					if(FW_version!=nxp_vstep_msg->fw_version){
						printf("Firmware version retrieved from cnt file doesnot match with FW\n");
						return Tfa98xx_Error_Other;
					}
					buffer_9912[counter++] = (FW_version>>16) & 0xff;
					buffer_9912[counter++] = (FW_version>>8) & 0xff;
					buffer_9912[counter++] = (FW_version>>6) & 0x03;


					/* Number of vsteps -> (always 1 in this case!) */
					buffer_9912[counter++] = nxp_vstep_msg->no_of_vsteps;

					/* Number of registers */
					buffer_9912[counter++] = (unsigned char)nxp_vstep_msg->reg_no;
					for(i=0; i<nxp_vstep_msg->reg_no;i++){
						uint16_t bitfield;
						int result;
						buffer_9912[counter++] = nxp_vstep_msg->msg_reg[(i*4)];
						buffer_9912[counter++] = nxp_vstep_msg->msg_reg[(i*4)+1];
						bitfield = (nxp_vstep_msg->msg_reg[(i*4)] <<8) + nxp_vstep_msg->msg_reg[(i*4)+1];
						result = tfa_get_bf( handle, bitfield);
						buffer_9912[counter++] = (result>>8) & 0xff;
						buffer_9912[counter++] = result & 0xff;
					}
					err = tfa98xx_dsp_support_drc(handle, &supportDrc);
					if (err){
						PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
						return err;
					}
					/* Number of messages -> (default is 2, can be 3 if DRC enabled) */
					if(nxp_vstep_msg->mbdrc_length!=0)
						buffer_9912[counter++] = 3;
					else
						buffer_9912[counter++] = 2;

					/* Message 0: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer_9912[counter++] = 0;

					/*re adjusting lengths, converting into bytes from word*/
					/*first byte is command id*/
					/* Message 0: message length (3 bytes)
					* Add 3 bytes because cmd id is part of length
					*/
					buffer_9912[counter++] = (((nxp_vstep_msg->algo_param_length+3)/3)>>16) & 0xff;
					buffer_9912[counter++] = (((nxp_vstep_msg->algo_param_length+3)/3)>>8) & 0xff;
					buffer_9912[counter++] = ((nxp_vstep_msg->algo_param_length+3)/3) & 0xff;

					/* Message 0: command id  */
					buffer_9912[counter++] = 0;
					buffer_9912[counter++] = MODULE_SPEAKERBOOST + 128;
					buffer_9912[counter++] = SB_PARAM_SET_ALGO_PARAMS_WITHOUT_RESET;

					/* Message 0: data (message_length bytes) -> GetAlgoPara */
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_ALGO_PARAMS, 
													(nxp_vstep_msg->algo_param_length), (unsigned char *)buffer_9912 + counter);
					if (err) {
						PRINT("%s Error: Unable to get algoparams data from DSP \n", __FUNCTION__);
						return err;
					}

					/* Message 1: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
					buffer_9912[(nxp_vstep_msg->algo_param_length) +counter++] = 1;

					/* Message 1: message length (3 bytes)
					* Add 6 bytes because cmd id is part of length */
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = (((nxp_vstep_msg->filter_coef_length+6)/3)>>16) & 0xff;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = (((nxp_vstep_msg->filter_coef_length+6)/3)>>8) & 0xff;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = ((nxp_vstep_msg->filter_coef_length+6)/3) & 0xff;

					/* Message 1: command id (6 bytes)  */
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = 0;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = MODULE_BIQUADFILTERBANK + 128;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = BFB_PAR_ID_SET_COEFS;

					/* Second command id is always zero (for now) */
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = 0;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = 0;
					buffer_9912[nxp_vstep_msg->algo_param_length + counter++] = 0;

					/* Message 1: data (message_length bytes) -> GetCoeff */
					err = tfa_dsp_cmd_id_coefs(handle,MODULE_BIQUADFILTERBANK,BFB_PAR_ID_GET_COEFS, 
												nxp_vstep_msg->filter_coef_length, (unsigned char *)buffer_9912 + (nxp_vstep_msg->algo_param_length) + counter);
					if (err) {
						PRINT("%s Error: Unable to get Coeff data from DSP \n", __FUNCTION__);
						return err;
					}

					if(supportDrc) {
					/* Message 2: message type -> (0 if SetAlgoConfig, 1 if setCoeffs, 2 is drc */
						buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = 2;

					/* Message 2: message length (3 bytes)
					* Add 6 bytes because cmd id is part of length */
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = (((nxp_vstep_msg->mbdrc_length+3)/3)>>16) & 0xff;
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = (((nxp_vstep_msg->mbdrc_length+3)/3)>>8) & 0xff;
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = ((nxp_vstep_msg->mbdrc_length+3)/3) & 0xff;

					/* Message 2: command id (3 bytes)  */
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = 0;
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = MODULE_SPEAKERBOOST + 128;
					buffer_9912[nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter++] = SB_PARAM_SET_MBDRC;

					/* Message 2: data (message_length bytes) -> MBDrc */
					err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_MBDRC, 
					nxp_vstep_msg->mbdrc_length, (unsigned char *)buffer_9912 + (nxp_vstep_msg->algo_param_length)  + (nxp_vstep_msg->filter_coef_length) + counter);
					if (err) {
						PRINT("%s Error: Unable to get MBDrc data from DSP \n", __FUNCTION__);
						return err;
					}
					}


					f = fopen(filename, "wb");
					if (!f) {
						PRINT("Unable to open %s\n", filename);
						return Tfa98xx_Error_Other;
					}

					if(supportDrc) {
						c = (int)(fwrite(buffer_9912, nxp_vstep_msg->mbdrc_length +
						nxp_vstep_msg->algo_param_length  + nxp_vstep_msg->filter_coef_length + counter, 1, f ));
					} else {
						c = (int)(fwrite(buffer_9912, nxp_vstep_msg->filter_coef_length + nxp_vstep_msg->algo_param_length + counter, 1, f));
					}
					fclose(f);
					free(nxp_vstep_msg);
				}

			break;
		case tfa_drc_params:
			err = tfa98xx_dsp_support_drc(handle, &supportDrc);
			if (err){
				PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
				return err;
			}
			if(supportDrc) {
				if ( tfa98xx_dev_family(handle) == 2) {
					/* Add CMD ID */
					buffer[0] = 0;
					buffer[1] = MODULE_SPEAKERBOOST + 128;
					buffer[2] = SB_PARAM_SET_MBDRC;

					/* Get payload */
					if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
						err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_MBDRC, 
								TFA72_MBDRCPARAMETER_LENGTH, (unsigned char *)buffer + 3);

						length = TFA72_MBDRCPARAMETER_LENGTH + 3;
					} else {
						err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_MBDRC, 
								TFA2_MBDRCPARAMETER_LENGTH, (unsigned char *)buffer + 3);

						length = TFA2_MBDRCPARAMETER_LENGTH + 3;
					}
					if (err){
						PRINT("%s Error: Getting MBDRC \n", __FUNCTION__);
						return err;
					}
				} else {			
					err = tfa98xx_dsp_read_drc(handle, 127, (unsigned char *)buffer);
					if (err){
						PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
						return err;
					}
					length = TFA1_DRC_LENGTH;
				}

				f = fopen( filename, "wb");
				if (!f)
				{
					PRINT("Unable to open %s\n", filename);
					err = Tfa98xx_Error_Other;
					return err;
				}
				c = (int)(fwrite( buffer, length, 1, f ));
				fclose(f);
			} else {
				PRINT("Error: MBDrc is not supported on the current device! \n");
			}
			break;
		case tfa_equalizer_params:
			if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {
				/* Stereo */
				if(tfa98xx_cnt_max_device() > 1) {
					buffer[0] = 0;
				} else { /* Mono */
					buffer[0] = 04;
				}

				buffer[1] = MODULE_BIQUADFILTERBANK + 128;
				buffer[2] = BFB_PAR_ID_SET_COEFS;

				/* Second command id is always zero (for now) */
				buffer[3] = 0;
				buffer[4] = 0;
				buffer[5] = 0;

				/* GetCoeffs for 72 is same size for mono and stereo */
				err = tfa_dsp_cmd_id_coefs(handle,MODULE_BIQUADFILTERBANK,BFB_PAR_ID_GET_COEFS, 
				TFA72_FILTERCOEFSPARAMETER_LENGTH, (unsigned char *)buffer + 6);
				if (err) {
					PRINT("%s Error: Unable to get Coeff data from DSP \n", __FUNCTION__);
					return err;
				}

				f = fopen( filename, "wb");
				if (!f)
				{
					PRINT("Unable to open %s\n", filename);
					err = Tfa98xx_Error_Other;
					return err;
				}
				c = (int)(fwrite( buffer, TFA72_FILTERCOEFSPARAMETER_LENGTH + 6, 1, f ));
				fclose(f);
			} else {
				PRINT("Error: Is is not possible to save the EQ file \n");
				return Tfa98xx_Error_Bad_Parameter;
			}
			break;
		case tfa_algo_params:
			if((tfa98xx_dev_revision(handle) & 0xff ) == 0x72) {

				if(tfa98xx_cnt_max_device() > 1) { /* Stereo */
					buffer[0] = 0;
					length = TFA72_ALGOPARAMETER_LENGTH_STEREO;
				} else { /* Mono */
					buffer[0] = 04;
					length = TFA72_ALGOPARAMETER_LENGTH_MONO;
				}

				buffer[1] = MODULE_SPEAKERBOOST + 128;
				buffer[2] = SB_PARAM_SET_ALGO_PARAMS;
				
				/* Message 0: data (message_length bytes) -> GetAlgoParam */
				err = tfa_dsp_cmd_id_write_read(handle,MODULE_SPEAKERBOOST,SB_PARAM_GET_ALGO_PARAMS,length,(unsigned char *)buffer + 3);
				if (err) {
					PRINT("Error: Unable to get algoparams data from DSP \n");
					return err;
				}

				f = fopen( filename, "wb");
				if (!f)
				{
					PRINT("Unable to open %s\n", filename);
					err = Tfa98xx_Error_Other;
					return err;
				}
				c = (int)(fwrite( buffer, length+3, 1, f ));
				fclose(f);
			}
			break;
		case tfa_config_params:
			if ( tfa98xx_dev_family(handle) == 1) {
				err = tfa98xx_dsp_config_parameter_count(handle, &fileLength);
				fileLength *= 3; /* 3bytes */			
				err = tfa98xx_dsp_read_config(handle, fileLength, (unsigned char *)buffer);
				if (err) {
					PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
					return err;
				}
				f = fopen( filename, "wb");
				if (!f) {
					PRINT("Unable to open %s\n", filename);
					return Tfa98xx_Error_Other;
				}
				c = (int)(fwrite( buffer, fileLength, 1, f ));
				fclose(f);
			} else {
				PRINT("Error: Is is not possible to save the config file for max2 \n");
				return Tfa98xx_Error_Bad_Parameter;
			}
			break;
		case tfa_preset_params:
			if ( tfa98xx_dev_family(handle) == 1) {
				err = tfa98xx_dsp_config_parameter_count(handle, &fileLength);
				fileLength *= 3; /* 3bytes */
				err = tfa98xx_dsp_read_preset(handle, fileLength+TFA1_PRESET_LENGTH, (unsigned char *)buffer);
				if (err){
					PRINT("%s %d Error: \n", __FUNCTION__, __LINE__);
					return err;
				}
				f = fopen( filename, "wb");
				if (!f) {
					PRINT("Unable to open %s\n", filename);
					return Tfa98xx_Error_Other;
				}
				c = (int)(fwrite(buffer, TFA1_PRESET_LENGTH, 1, f));
				fclose(f);
			} else {
				PRINT("Error: Is is not possible to save the preset file for max2 \n");
				return Tfa98xx_Error_Bad_Parameter;
			}
			break;
		default:
			PRINT_ERROR("%s Error: bad parameter:%d\n", __FUNCTION__, params) ;
			break;
	}

	if (c != 1) {
		PRINT("Unable to handle the file %s\n", filename);
		return Tfa98xx_Error_Other;
	} else {
		/* Add a header to the file */
		headerInfo[0] = "NXP";		/* Customer */
		headerInfo[1] = "save";		/* Application */
		headerInfo[2] = "tfa98xx";	/* Type */
		argc = 3;

		if(params == tfa_speaker_params) {
			headerInfo[3] = "nickName";	/* name */
			headerInfo[4] = "myVendor";	/* vendor */
			headerInfo[5] = "spkType";	/* type */
			headerInfo[6] = "0";		/* height */
			headerInfo[7] = "0";		/* width */
			headerInfo[8] = "0";		/* depth */
			headerInfo[9] = "8";		/* primary */
			headerInfo[10] = "8";		/* secondary */
			argc = 11;
		}

		return tfaContBin2Hdr(filename, argc, headerInfo);
	}
}

enum Tfa98xx_Error tfa98xx_verify_speaker_range(int idx, float imp[2], int spkr_count)
{
        enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
        nxpTfaSpeakerFile_t *spkr;
        float Rtypical_left;
	float Rtypical_right;

        spkr = (nxpTfaSpeakerFile_t *)tfacont_getfiledata(idx, 0, speakerHdr);
	if (spkr==0)
		return Tfa98xx_Error_Other;

	Rtypical_left = spkr->ohm_primary;
	Rtypical_right = spkr->ohm_secondary;

	/* We always have atleast one speaker */
	if (Rtypical_left==0) {
		PRINT("Warning: Speaker impedance (primary) not defined in spkr file, assuming 8 Ohm!\n");
		Rtypical_left = 8;
	}

	/* If we have multiple speakers also check the secondary */
	if (spkr_count == 2) {
		if (Rtypical_right==0) {
			PRINT("Warning: Speaker impedance (secondary) not defined in spkr file, assuming 8 Ohm!\n");
			Rtypical_right = 8;
		}
	}

        /* 15% variation possible */
	if ( imp[0] < (Rtypical_left*0.85) || imp[0] > (Rtypical_left*1.15))
		PRINT("Warning: Primary Speaker calibration value is not within expected range! (%2.2f Ohm) \n", imp[0]);

	if(spkr_count > 1) {
		if ( imp[1] < (Rtypical_right*0.85) || imp[1] > (Rtypical_right*1.15))
			PRINT("Warning: Secondary Speaker calibration value is not within expected range! (%2.2f Ohm) \n", imp[1]);
	}

        return err;
}

void tfa_soft_probe_all(nxpTfaContainer_t* p_cnt)
{
	int dev, devcount = p_cnt->ndev;
	int revid;

	for(dev=0; dev < devcount; dev++) {
		revid = tfa_cnt_get_devid(p_cnt, dev);
		if (revid)
			tfa_soft_probe(dev, revid);
		else
			PRINT_ERROR("No revid in patch\n");
	}
}

enum Tfa98xx_Error tfa_probe_all(nxpTfaContainer_t* p_cnt)
{
	enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
	int dev, devcount = p_cnt->ndev;
	uint8_t slave;
	Tfa98xx_handle_t handle;

	for(dev=0; dev < devcount; dev++) {
		tfaContGetSlave(dev, &slave);
		err = tfa_probe(slave*2, &handle);
		if (err) {
			PRINT_ASSERT(err);
			return err;
		}
	}

	return err;
}

void extern_msleep_interruptible(int msec)
{
	msleep_interruptible(msec);
}

#ifdef __linux__
struct itimerval timervalue;
struct sigaction new_handler, old_handler;
void timer_sig_handler();

int start_timer(int msec, void (*timer_func_handler)(void))
{
	timer_func_handler_pntr = timer_func_handler;

	timervalue.it_interval.tv_sec	= (msec / 1000);
	timervalue.it_interval.tv_usec	= (msec % 1000) * 1000;
	timervalue.it_value.tv_sec	= (msec / 1000);
	timervalue.it_value.tv_usec	= (msec % 1000) * 1000;
	if(setitimer(ITIMER_REAL, &timervalue, NULL)) {
		PRINT("Error: setitimer() error \n");
		return Tfa98xx_Error_Fail;
	}

	new_handler.sa_handler	= &timer_sig_handler;
	new_handler.sa_flags	= SA_NOMASK;
	if(sigaction(SIGALRM, &new_handler, &old_handler)) {
		PRINT("Error: sigaction() error \n");
		return Tfa98xx_Error_Fail;
	}

	return Tfa98xx_Error_Ok;
}

void timer_sig_handler()
{
	timer_func_handler_pntr();
}

void stop_timer(void)
{
	timervalue.it_interval.tv_sec	= 0;
	timervalue.it_interval.tv_usec	= 0;
	timervalue.it_value.tv_sec	= 0;
	timervalue.it_value.tv_usec	= 0;

	setitimer(ITIMER_REAL, &timervalue, NULL);
	sigaction(SIGALRM, &old_handler, NULL);
}

#else

HANDLE win_timer;
VOID CALLBACK timer_sig_handler(void);

int start_timer(int mSec, void (*timer_func_handler)(void))
{
	timer_func_handler_pntr = timer_func_handler;

	if(CreateTimerQueueTimer(&win_timer, NULL, (WAITORTIMERCALLBACK)timer_sig_handler, NULL, mSec, mSec, WT_EXECUTEINTIMERTHREAD) == 0) {
		PRINT("Error: CreateTimerQueueTimer() error \n");
		return Tfa98xx_Error_Fail;
	}

	return Tfa98xx_Error_Ok;
}

VOID CALLBACK timer_sig_handler(void)
{
	timer_func_handler_pntr();
}

void stop_timer(void)
{
	  DeleteTimerQueueTimer(NULL, win_timer, NULL);
	  CloseHandle(win_timer);
}

#endif


int compare_strings(char *buffer, char *name, int buffersize)
{
	int i=0;
	char new_buf[50];

	while(i < buffersize) {
		new_buf[i] = (char)tolower(buffer[i]);
		i++;
	}

	if (strcmp(new_buf, name) == 0) {
		return 1;
	} else {
		return 0;
	}
}