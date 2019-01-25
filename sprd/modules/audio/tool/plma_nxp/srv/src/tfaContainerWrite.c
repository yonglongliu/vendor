/*
 * tfaContainer.c
 *
 *    functions for creating container file and parsing of ini file
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#if defined(WIN32) || defined(_X64)
#include <direct.h>
#else
#include <libgen.h> /* basename */
#include <unistd.h>
#include <strings.h>
#endif

#include "tfa98xx_parameters.h"
#include "tfaContUtil.h"
#include "dbgprint.h"
#include "tfaContainerWrite.h"
#include "tfaOsal.h"
#include "tfa98xx_tfafieldnames.h"
#include "minIni.h"
#include "Tfa98API.h" /* legacy API */
#include "tfa.h"
#include "config.h" /* crc32_le */
#include "tfa98xxDRC.h"

#define tfaContCRC32(buf,size,crc) ~crc32_le(~crc, buf, size)

/*
 * tracing global
 */
static nxpTfaContainer_t *gCont=NULL; /* container file */
int tfa98xx_cnt_write_verbose;
static int tfa98xx_family_type = 2; /* Set the default to tfa9888 (tfa2) */
static char nostring[]="Undefined string";
static char *profile_order[64]; //Max 64 profiles supported
static int tfa_dev_type[TFACONT_MAXDEVS]; /* store device type from patchfile or overload with --tfa>3*/
#define VERBOSE if ( tfa98xx_cnt_write_verbose )

#define CMD_ID_CONFIG_MASK (0x0f<<16)
#define CMD_ID_DISABLE_COMMON    (1<<16)
#define CMD_ID_DISABLE_PRIMARY   (1<<17)
#define CMD_ID_DISABLE_SECONDARY (1<<18)
#define CMD_ID_SAME_CONFIG       (1<<19)

/*
 * mapping of command name to id's
 *
 * Note: it is assumed that the length of the command name
 *       is no longer then 31 chars including the modifiers
 *       (excluding the trailing \0).
 */
struct cmd_id {
	char *name;  /* command name */
	uint32_t id; /* 24 bit id */
};

static const struct cmd_id cmds[] = {
	{ "SetvBatFactors",     0x008002 },
	{ "SetSensesDelay",    0x008004 },
	{ "SetSensesCal",      0x008005 },
	{ "SetInputSelector",  0x008006 },
	{ "SetOutputSelector", 0x008008 },
	{ "SetProgramConfig",  0x008009 },
	{ "SetGains",          0x00800a },
	{ "SetLagW",           0x008101 },
	{ NULL,                0xFFFFFF } /* end marker */
};


/* Tfa98xx_Mode_t to string mapping */
static const char *tfa98xx_mode_str[] = {
	"normal",
	"rcv",
	NULL   /* end marker */
};

/* biquad filter type names
 *  need to be aligned with enum nxpTfaFilterType
 */
static const char *tfa98xx_eqtype_str[] = {
		"flat", 	// fFlat           //Vary only gain
		"lowpass", 	// fLowpass        //2nd order Butterworth low pass
		"highpass",	// fHighpass       //2nd order Butterworth high pass
		"lowshelf",	// fLowshelf
		"highshelf",	// fHighshelf
		"notch", 	// fNotch
		"peak", 	// fPeak
		"bandpass",	// fBandpass
		"1stlp", 	// f1stLP
		"1sthp", 	// f1stHP
		"elliptic",
		"integrator1stlp", 
		"integrator1sthp",
		NULL 
};

void free_gCont() {
	if(gCont != NULL)
		free(gCont);
}

const char *tfa_cont_eqtype_name(enum nxpTfaFilterType t) {
	return tfa98xx_eqtype_str[t];
}

void tfa_cont_write_verbose(int verbose) {
	tfa98xx_cnt_write_verbose = verbose;
}
/* devtype when set from the command line
 *  if < 5 then it's the family type only
 *  else it will use this for all devices as device revid
 * */
void tfa_cont_dev_type(int type) {
	if (type<5)
		tfa98xx_family_type = type;
	else {
		tfa_dev_type[0]=tfa_dev_type[1]=tfa_dev_type[2]=tfa_dev_type[3]=type;
		tfa98xx_family_type = type!=0x88 ? 1 : 2;
	}
}

//#define DUMP
#ifdef DUMP
static void dumpDsc1st(nxpTfaDescPtr_t * dsc);
static void dumpStrings(void);
static void dumpDevs(nxpTfaContainer_t * cont);
static void dump(nxpTfaContainer_t * cont, int len);
#endif

const char *tfaContGetStringPass1(nxpTfaDescPtr_t * dsc);

static int fsize(const char *name)
{
	struct stat st;
	if( stat(name, &st) < 0 )
		return -1;
	return st.st_size;
}
static void fillSpeaker(nxpTfaSpeakerFile_t *spk, int argc, char *argv[]) {
	int i=3;

	if (argc > 3) strncpy(spk->name, argv[i++], sizeof(spk->name));
	if (argc > 4) strncpy(spk->vendor, argv[i++], sizeof(spk->vendor));
	if (argc > 5) strncpy(spk->type, argv[i++], sizeof(spk->type));
	if (argc > 6) spk->height=(uint8_t)atoi(argv[i++]);
	if (argc > 7) spk->width=(uint8_t)atoi(argv[i++]);
	if (argc > 8) spk->depth=(uint8_t)atoi(argv[i++]);
	if (argc > 9) spk->ohm_primary=(uint8_t)atoi(argv[i]);
	if (argc > 10) spk->ohm_secondary=(uint8_t)atoi(argv[i++]);
	else {
		PRINT("Only 1 speaker impedance is given, assuming mono configuration! \n");
		spk->ohm_secondary=0;
	}
}

#if ( defined( TFA9888 ) || defined( TFA98XX_FULL ))
static void fillSpeakerMax2(struct nxpTfaSpeakerFileMax2 *spk, int argc, char *argv[]) {
	int i=3;

	if (argc > 3) strncpy(spk->name, argv[i++], sizeof(spk->name)-1);
	if (argc > 4) strncpy(spk->vendor, argv[i++], sizeof(spk->vendor)-1);
	if (argc > 5) strncpy(spk->type, argv[i++], sizeof(spk->type)-1);
	if (argc > 6) spk->height=(uint8_t)atoi(argv[i++]);
	if (argc > 7) spk->width=(uint8_t)atoi(argv[i++]);
	if (argc > 8) spk->depth=(uint8_t)atoi(argv[i++]);
	if (argc > 9) spk->ohm_primary=(uint8_t)atoi(argv[i]);
	if (argc > 10) spk->ohm_secondary=(uint8_t)atoi(argv[i++]);
	else {
		PRINT("Only 1 speaker impedance is given, assuming mono configuration! \n");
		spk->ohm_secondary=0;
	}
}
#endif

static int tfaRename(char *fname, const char *ext) {
	char oldfname[FILENAME_MAX];
	/* rename the old file */
	strncpy(oldfname, fname, FILENAME_MAX-1);
	strncat(oldfname, ext, (FILENAME_MAX - strlen(oldfname)-1));
	if(rename(fname, oldfname) == 0)
		PRINT("Renamed %s to %s.\n", fname, oldfname);
	else {
		PRINT("Warning: unable to rename %s to %s  (%s) \n", fname, oldfname, strerror(errno));
		return 1;
	}

	return 0;
}
//TODO fix trunk
#define BIQUAD_COEFF_SIZE 6
/*
 * This function will be deprecated.
 * Floating point calculations must be done in user-space
 *
 * NOTE: that this function is used by a Windows tool
 */
Tfa98xx_Error_t tfa98xxCalcBiquadCoeff(float b0, float b1, float b2,
                float a1, float a2, unsigned char *bytes)
{
        float max_coeff;
        int headroom;
        int coeff_buffer[BIQUAD_COEFF_SIZE];
        /* find max value in coeff to define a scaler */
        max_coeff = (float)fabs(b0);
        if (fabs(b1) > max_coeff)
                max_coeff = (float)fabs(b1);
        if (fabs(b2) > max_coeff)
                max_coeff = (float)fabs(b2);
        if (fabs(a1) > max_coeff)
                max_coeff = (float)fabs(a1);
        if (fabs(a2) > max_coeff)
                max_coeff = (float)fabs(a2);
        /* round up to power of 2 */
        headroom = (int)ceil(log(max_coeff + pow(2.0, -23)) / log(2.0));

        /* some sanity checks on headroom */
        if (headroom > 8)
                return Tfa98xx_Error_Bad_Parameter;
        if (headroom < 0)
                headroom = 0;
        /* set in correct order and format for the DSP */
        coeff_buffer[0] = headroom;
        coeff_buffer[1] = (int) (-a2 * (1 << (23 - headroom)));
        coeff_buffer[2] = (int) (-a1 * (1 << (23 - headroom)));
        coeff_buffer[3] = (int) (b2 * (1 << (23 - headroom)));
        coeff_buffer[4] = (int) (b1 * (1 << (23 - headroom)));
        coeff_buffer[5] = (int) (b0 * (1 << (23 - headroom)));
/*
        coeff_buffer[1] = (int) (-a2 * pow(2.0, 23 - headroom));
        coeff_buffer[2] = (int) (-a1 * pow(2.0, 23 - headroom));
        coeff_buffer[3] = (int) (b2 * pow(2.0, 23 - headroom));
        coeff_buffer[4] = (int) (b1 * pow(2.0, 23 - headroom));
        coeff_buffer[5] = (int) (b0 * pow(2.0, 23 - headroom));
*/

        /* convert to fixed point and then bytes suitable for transmission over I2C */
        tfa98xx_convert_data2bytes(BIQUAD_COEFF_SIZE, coeff_buffer, bytes);
        return Tfa98xx_Error_Ok;
}

enum TFA98XX_SAMPLERATE{
    TFA98XX_SAMPLERATE_08000 = 0,
    TFA98XX_SAMPLERATE_11025 = 1,
    TFA98XX_SAMPLERATE_12000 = 2,
    TFA98XX_SAMPLERATE_16000 = 3,
    TFA98XX_SAMPLERATE_22050 = 4,
    TFA98XX_SAMPLERATE_24000 = 5,
    TFA98XX_SAMPLERATE_32000 = 6,
    TFA98XX_SAMPLERATE_44100 = 7,
    TFA98XX_SAMPLERATE_48000 = 8
};
uint8_t  tfa98xx_fs2enum(int rate) {
	uint8_t value = 0;

	switch (rate) {
	case 48000:
		value =  TFA98XX_SAMPLERATE_48000;
		break;
	case 44100:
		value =  TFA98XX_SAMPLERATE_44100;
		break;
	case 32000:
		value =  TFA98XX_SAMPLERATE_32000;
		break;
	case 24000:
		value =  TFA98XX_SAMPLERATE_24000;
		break;
	case 22050:
		value =  TFA98XX_SAMPLERATE_22050;
		break;
	case 16000:
		value =  TFA98XX_SAMPLERATE_16000;
		break;
	case 12000:
		value =  TFA98XX_SAMPLERATE_12000;
		break;
	case 11025:
		value =  TFA98XX_SAMPLERATE_11025;
		break;
	case 8000:
		value =  TFA98XX_SAMPLERATE_08000;
		break;
	default:
		ERRORMSG("unsupported samplerate : %d\n", rate);
	}
	return value;
}

int HeaderMatches (nxpTfaHeader_t *hdr, nxpTfaHeaderType_t t) {
	return hdr->id == t && hdr->version[1] == '_';
}

/*
 * Generate a file with header from input binary <bin>.<type> [customer] [application] [type]
 */

int tfaContBin2Hdr(char *inputname, int argc, char *argv[]) {
	nxpTfaHeader_t *hdr = 0, existingHdr;
	nxpTfaContainer_t *thisCont = {0};
	nxpTfaSpeakerFile_t head; // note that this is the largest header
	struct nxpTfaSpeakerFileMax2 spkMax2;
	char fname[FILENAME_MAX];
	nxpTfa98xxParamsType_t paramsType;
	uint8_t *base;
	int i, size, hdrsize=0, cntFile=0;
	FILE *f;
	memset(&spkMax2, 0, sizeof(spkMax2));
	memset(&head, 0, sizeof(head));

	strncpy(fname, inputname, FILENAME_MAX-1);

	size = fsize(fname);
	if ( !size ) {
		ERRORMSG("Can't open %s.\n", fname);
		return Tfa98xx_Error_Bad_Parameter;
	}
	
	/*  Read the first x bytes to see if the file already has a header
	 *  If a header exists, don't add a new one, just replace some fields and recalculate the CRC
	 */
	f=fopen(fname, "rb");
	if ( !f ) {
		ERRORMSG("Can't open %s (%s).\n", fname, strerror(errno));
		return Tfa98xx_Error_Bad_Parameter;
	}

	fread(&existingHdr , sizeof(existingHdr), 1, f);
	fclose(f);

	PRINT("Creating header for %s with args [", fname);
	for (i=0; i<argc; i++)
		PRINT("%s, ", argv[i]); 
	PRINT("]\n");
	
	paramsType = cliParseFiletype(fname);
	switch ( paramsType ) {
	case tfa_speaker_params:
		head.hdr.id = (uint16_t)speakerHdr;
		head.hdr.version[0] = NXPTFA_SP_VERSION;
		hdrsize = HeaderMatches(&existingHdr, speakerHdr) ? 0 : sizeof(nxpTfaSpeakerFile_t);
		if(tfa98xx_family_type == 2) {		/* max2 */
			spkMax2.hdr.id = (uint16_t)speakerHdr;
			spkMax2.hdr.version[0] = NXPTFA_SP3_VERSION; // use other version number in case of patch files..?
			spkMax2.hdr.version[1] = '_';
			spkMax2.hdr.subversion[0] = '0';
			spkMax2.hdr.subversion[1] = '0';
			spkMax2.hdr.size = (uint16_t)(hdrsize+size); // add filesize
		}
		break;
	case tfa_patch_params:
		head.hdr.id = (uint16_t)patchHdr;
        head.hdr.version[0] = NXPTFA_PA_VERSION;
		hdrsize = HeaderMatches(&existingHdr, patchHdr) ? 0 : sizeof(nxpTfaPatch_t);
		break;
	case tfa_config_params:
		head.hdr.id = (uint16_t)configHdr;
                if( tfa98xx_family_type == 2 )
                        head.hdr.version[0] = NXPTFA_CO3_VERSION;
                else
		        head.hdr.version[0] = NXPTFA_CO_VERSION;
		hdrsize = HeaderMatches(&existingHdr, configHdr) ? 0 : sizeof(nxpTfaConfig_t);
		break;
	case tfa_msg_params:
		head.hdr.id = (uint16_t)msgHdr;
		head.hdr.version[0] = NXPTFA_MG_VERSION;
		hdrsize = HeaderMatches(&existingHdr, msgHdr) ? 0 : sizeof(nxpTfaMsgFile_t);
		break;
	case tfa_equalizer_params:
		head.hdr.id = (uint16_t)equalizerHdr;
		head.hdr.version[0] = NXPTFA_EQ_VERSION;
		hdrsize = HeaderMatches(&existingHdr, equalizerHdr) ? 0 : sizeof(nxpTfaHeader_t)+1;
		break;
        case tfa_drc_params:
		head.hdr.id = (uint16_t)drcHdr;
		if( tfa98xx_family_type == 2 ) {
                        head.hdr.version[0] = NXPTFA_DR3_VERSION;
		} else {
			if ( HeaderMatches(&existingHdr, drcHdr) == 0 ) {
				FILE *fdrc;
				char *drcFile;
				int fs;
				float vCal;
				/* the header is not present, so this is old DRC file format */
				FILE *in = fopen (fname, "rb");
				drcFile = (char *)malloc(2 * sizeof(struct drcParamBlock));
				if( drcFile == NULL ) {
					PRINT("Couldn't able to allocate requested memory\n");
					fclose(in);
					return Tfa98xx_Error_Bad_Parameter;
				}

				if (fread(drcFile, sizeof(struct drcParamBlock), 1, in) == 1) {
					fclose(in);
					vCal = (float)9.95;
					fs = 48000;
	
					/* if we have a vlsCal we can compensate the thresholds if needed */
					if (vCal > 0) {
						drc_undo_threshold_compensation((drcParamBlock_t *)drcFile, vCal);
					}

					drc_generic_to_specific(drcFile, (2 * sizeof(struct drcParamBlock)), vCal, fs);

					// save old file as .old
					tfaRename(fname, ".old");
				} else {
					fclose(in);
					PRINT("Unable to read %s\n", fname);
					free(drcFile);
					return Tfa98xx_Error_Bad_Parameter;
				}

				/* overwrite the "old" drc file with new drc data */		
				fdrc = fopen(fname, "wb");
				if (!fdrc) {
					PRINT("Unable to open %s\n", fname);
					free(drcFile);
					return Tfa98xx_Error_Bad_Parameter;
				}

				if ((int)fwrite(drcFile, 2 * sizeof(struct drcParamBlock), 1, fdrc) != 1)
					PRINT("fwrite error\n");
				fclose(fdrc); 
				free(drcFile);
				size = 2 * sizeof(struct drcParamBlock);
			}
			head.hdr.version[0] = NXPTFA_DR_VERSION;
		}

		hdrsize = HeaderMatches(&existingHdr, drcHdr) ? 0 : sizeof(nxpTfaDrc_t);
		break;
	case tfa_vstep_params:
		head.hdr.id = (uint16_t)volstepHdr;
                head.hdr.version[0] = NXPTFA_VP_VERSION;
		hdrsize = HeaderMatches(&existingHdr, volstepHdr) ? 0 : sizeof(nxpTfaHeader_t);
                if ( tfa98xx_family_type == 2 ) {
                        head.hdr.version[0] = NXPTFA_VP3_VERSION;
                }
		break;
	case tfa_preset_params:
		head.hdr.id = (uint16_t)presetHdr;
		head.hdr.version[0] = NXPTFA_PR_VERSION;
		hdrsize = HeaderMatches(&existingHdr, presetHdr) ? 0 : sizeof(nxpTfaHeader_t);
		break;
	case tfa_cnt_params:
		cntFile = 1;
		break;
	case tfa_info_params:
		head.hdr.id = (uint16_t)infoHdr;
                head.hdr.version[0] = NXPTFA_VP_VERSION;
		hdrsize = HeaderMatches(&existingHdr, infoHdr) ? 0 : sizeof(nxpTfaHeader_t);
                if ( tfa98xx_family_type == 2 ) {
                        head.hdr.version[0] = NXPTFA_VP3_VERSION;
                }
		break;
	default:
		ERRORMSG("Unknown file type:%s\n", fname) ;
		return Tfa98xx_Error_Bad_Parameter;
		break;
	}

	if(cntFile) {
		thisCont = malloc(size);
		thisCont->size = size;
	} else {
		head.hdr.version[1] = '_';
		head.hdr.subversion[0] = '0';
		head.hdr.subversion[1] = '0';

		head.hdr.size = (uint16_t)(hdrsize+size); // add filesize
		hdr = malloc(hdrsize + size);
		if ( !hdr ) {
			ERRORMSG("Can't allocate %d bytes.\n", hdrsize + size);
			return Tfa98xx_Error_Bad_Parameter;
		}
	}

	/* Copy formatted header to buffer
         * Only copy this when there is no header, else we get 2 headers
         */
	if (hdrsize != 0) {
		if(tfa98xx_family_type == 2 && paramsType == tfa_speaker_params)
			memcpy(hdr, &spkMax2, hdrsize);	
		else if(!cntFile) {
			memcpy(hdr, &head, hdrsize);
		}
	}

	if(!cntFile)
		hdr->size = (uint16_t)(hdrsize + size);

	f=fopen(fname, "r+b");
	if(!f) {
	    ERRORMSG("Can't open %s (%s).\n", fname, strerror(errno));
        if(hdr != 0) 
            free(hdr);
		return Tfa98xx_Error_Bad_Parameter;
	}

	if(cntFile)
		fread((uint8_t *)thisCont, thisCont->size, 1, f);
	else
		fread((uint8_t *)hdr+hdrsize, hdr->size, 1, f);

	fclose(f);
	
	if(paramsType == tfa_vstep_params) {
		if(hdr->size != (hdrsize + size)) {
			PRINT("\nWarning: The size: %d is not correct. Size is overwritten with new size: %d \n\n",
						hdr->size, (hdrsize + size));
			hdr->size = (uint16_t)(hdrsize + size);
		}
	}

	/* If there is already a header, we still want to be able to change the version */
	if(hdrsize == 0 && hdr != 0) {
		if(tfa98xx_family_type == 2 && paramsType != tfa_patch_params)
			hdr->version[0] = '3';
		else
			hdr->version[0] = '1';
	}

	/* For the 88 device we have 2 versions of vsteps 
	 * Therefor we can have subversion 0 and 1.
	 * To know if we have the "new" subversion 1 vstep file we need to parse it 
	 * If the vstep contains a message type 3 the subversion should become 1
	 * We can have files without header, or files that already have a header!
	 */
	if((tfa98xx_family_type == 2) && (paramsType == tfa_vstep_params)) {
		nxpTfaHeader_t *buf = 0;
		int new_vstep_version;
		tfaReadFile(fname, (void**) &buf);
	
		new_vstep_version = parse_vstep((nxpTfaVolumeStepMax2_1File_t *) buf, hdrsize);

		if(new_vstep_version) {
			head.hdr.subversion[1] = '1';
			memcpy(hdr, &head, sizeof(nxpTfaHeader_t));
		}

		free(buf);
	}

	i=0;
	if(cntFile) {
		if (argc > 0) strncpy(thisCont->customer, argv[i++], sizeof(thisCont->customer));
		if (argc > 1) strncpy(thisCont->application, argv[i++], sizeof(thisCont->application));
		if (argc > 2) strncpy(thisCont->type, argv[i++], sizeof(thisCont->type));
	} else {
		if (argc > 0) strncpy(hdr->customer, argv[i++], sizeof(hdr->customer));
		if (argc > 1) strncpy(hdr->application, argv[i++], sizeof(hdr->application));
		if (argc > 2) strncpy(hdr->type, argv[i++], sizeof(hdr->type));
	}
	if (paramsType == tfa_speaker_params) {
		if(tfa98xx_family_type == 2)
			fillSpeakerMax2((struct nxpTfaSpeakerFileMax2 *) hdr, argc, argv);
		else if(!cntFile)
			fillSpeaker((nxpTfaSpeakerFile_t *) hdr, argc, argv);
	}


	/*
	 * calc CRC over bytes following the CRC field
	 */
	if(cntFile) {
		base = (uint8_t *)&thisCont->CRC + 4; // ptr to bytes following the CRC field
		size = thisCont->size - (int)(base-(uint8_t *)thisCont); // nr of bytes following the CRC field
	} else {
		base = (uint8_t *)&hdr->CRC + 4; // ptr to bytes following the CRC field
		size = hdr->size - (int)(base-(uint8_t *)hdr); // nr of bytes following the CRC field
	}

	if(size > 0) 
	{
		if(cntFile)
			thisCont->CRC = tfaContCRC32(base, size, 0u); // nr of bytes following the CRC field, 0);
		else
			hdr->CRC = tfaContCRC32(base, size, 0u); // nr of bytes following the CRC field, 0);

		if(hdrsize == 0)
			PRINT("The existing header is modified for %s.\n", fname);
		else
			PRINT("A new Header is created for %s.\n", fname);

		tfaRename(fname, ".old");
		
		if(cntFile)
			tfaContSave((nxpTfaHeader_t *) thisCont, fname);
		else
			tfaContSave(hdr, fname);
	}
	else
		PRINT("Error: The size is not correct -> %d\n", size);

	if(cntFile)
		free(thisCont);
	else
		free(hdr);

	return Tfa98xx_Error_Ok;
}

/*
 * Get the absolute path of the file
 */
void tfaGetAbsolutePath(char *fileName, nxpTfaLocationInifile_t *loc) 
{
	char cntFile[FILENAME_MAX];
	int strLength;

	//Get the file basename
	strncpy(cntFile,  basename(fileName), FILENAME_MAX-1);

#if defined(WIN32) || defined(_X64)
	{
		char full[FILENAME_MAX];
		//Get the absolute path of the ini file
		_fullpath(full, fileName, FILENAME_MAX);
		//Get the path and basename length
		strLength = (int)strlen(full);
		strLength -= (int)strlen(cntFile);
		//Only copy the path (without filename)
		strncpy(loc->locationIni, full, strLength);
		loc->locationIni[strLength] = 0;
	}
#else
	char *abs_path = realpath(fileName, NULL);
        if(abs_path) {
	        //Get the path and basename length
	        strLength = (int)strlen(abs_path);
	        strLength -= (int)strlen(cntFile);
	        //Only copy the path (without filename)
	        strncpy(loc->locationIni, abs_path, strLength);
        }
#endif
}


/*
 * Generate the container file from an ini file <this>.ini to <this>.cnt
 *
 */
int tfaContIni2Container(char *iniFile) {
	nxpTfaLocationInifile_t *loc = malloc(sizeof(nxpTfaLocationInifile_t));
	char cntFile[FILENAME_MAX], *dot;
	char completeName[FILENAME_MAX];
	int size = 0;

        if(loc == 0) {
                PRINT("Memory allocation failed, No container is created!\n");
                return 0;
        }

	/* Verify the file extension */
	if (strstr(iniFile, ".ini") == NULL ) {
		PRINT("Error: Incorrect file extension! Only .ini is allowed! \n");
		free(loc);
		return 0;
	}

	memset(loc->locationIni, '\0', sizeof(loc->locationIni));

	//Get the file basename
	strncpy(cntFile, basename(iniFile), FILENAME_MAX-1);

	tfaGetAbsolutePath(iniFile, loc);

	strncpy(completeName, loc->locationIni, FILENAME_MAX-1);
	strncat(completeName, cntFile, (FILENAME_MAX - strlen(completeName)-1));

	PRINT("Generating container using %s \n", completeName);

	dot  = strrchr(cntFile, '.');
	if(dot) {
		*dot='\0';
	}
	strcat(cntFile,  ".cnt");

	size = tfaContParseIni(iniFile, cntFile, loc);
	if(size > 0) {
		PRINT("Created container %s of %d bytes.\n", cntFile, size);
	} else {
		PRINT("No container is created!\n");
	}
	free(loc);
	return size;
}


/* Calculating ZIP CRC-32 in 'C'
   =============================
   Reference model for the translated code */




#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

nxpTfaDescriptorType_t parseKeyType(char *key)
{
#if 1 // TODO remove me
	if (strcmp("SetInputSelector", key) == 0)
		return dscSetInputSelect;
	if (strcmp("SetOutputSelector", key) == 0)
		return dscSetOutputSelect;
	if (strcmp("SetProgramConfig", key) == 0)
		return dscSetProgramConfig;
	if (strcmp("SetLagW", key) == 0)
		return dscSetLagW;
	if (strcmp("SetGains", key) == 0)
		return dscSetGains;
	if (strcmp("SetvBatFactors", key) == 0)
		return dscSetvBatFactors;
	if (strcmp("SetSensesCal", key) == 0)
		return dscSetSensesCal;
	if (strcmp("SetSensesDelay", key) == 0)
		return dscSetSensesDelay;
	if (strcmp("dscSetMBDrc", key) == 0)
		return dscSetMBDrc;
#endif
	if (strcmp("mode", key) == 0)
		return dscMode;
	if (strcmp("file", key) == 0)
		return dscFile;
        if (strcmp("default", key) == 0)
		return dscDefault;
	if (strcmp("msg", &key[4]) == 0)
		return dscFile;
	if (strcmp("patch", key) == 0)
		return dscPatch;
	if (strcmp("config", key) == 0)
		return dscFile;
	if (strcmp("preset", key) == 0)
		return dscFile;
	if (strcmp("speaker", key) == 0)
		return dscFile;
	if (strcmp("drc", key) == 0)
		return dscFile;
	if (strcmp("eq", key) == 0)
		return dscFile;
	if (strcmp("volstep", key) == 0)
		return dscFile;
	if (strcmp("vstep", key) == 0)
		return dscFile;
	if (strcmp("info", key) == 0)
		return dscFile;
	if (strcmp("device", key) == 0)
		return dscDevice;
	if (strncmp("filter",key, 6) == 0)
		return dscFilter;
	if (strcmp("noinit", key) == 0)
		return dscNoInit;
        if (strcmp("features", key) == 0)
		return dscFeatures;
	if (key[0] == '-')
		return dscGroup;
	if (key[0] == '&')
		return dscProfile;
        if (key[0] == '+')
		return dscLiveData;
	if (key[0] == '*')
		return dscCmd;
	if (key[0] == '^')
		return dscPatch;
	if (key[0] == '$')
		return dscRegister;
	if (key[0] == '@')
		return dscCfMem;
	if (key[0] == '_')
		return dscBitfield; //return dscBitfieldBase;
	if (strcmp("bus", key) == 0)
		return -1;	// skip
	if (strcmp("dev", key) == 0)
		return -1;	// skip
	if (strcmp("devid", key) == 0)
		return -1;	// skip
	if (strcmp("func", key) == 0)
		return -1;	// skip

	return dscString;	//unknown, assume dscString
}

/*
 * file processing key tables
 */

#define MAXKEYLEN 64
#define MAXLINELEN 256

static char *inifile; // global used in browse functions
typedef struct namelist {
	int n;
	int len;
	char names[64][MAXKEYLEN];
} namelist_t;

static int findDevices(const char *section, const char *key, const char *value, 
				const void *userdata)
{
	namelist_t *keys;

	keys = (namelist_t *) userdata;
	keys->len++;
	// PRINT("    [%s]\t%s=%s\n", section, key, value);
	if (strcmp(section, "system") == 0 && strcmp(key, "device") == 0) {
		strncpy(keys->names[keys->n], value, MAXKEYLEN-1);
		keys->n++;
		keys->len--;	//don't count system items
	}

	return 1;
}

static char *currentSection;
static int findLivedata(const char *section, const char *key, const char *value, 
			const void *userdata)
{
	namelist_t *keys;
#define INI_MAXKEY 64
	char tmp[INI_MAXKEY];
	int i;

	keys = (namelist_t *) userdata;
	keys->len++;
	//PRINT("    [%s]\t%s=%s\n", section, key, value);
	if (strcmp(section, currentSection) == 0 && strcmp(&key[4], "livedata") == 0) {	// skip preformat
		// check if it exists
		if (ini_getkey(value, 0, tmp, 10, inifile) == 0) {
			PRINT("no livedata section:%s\n", value);
			return 0;
		}
		// avoid duplicates
		for(i=0; i<keys->n;i++) {
			if (strcmp(value, keys->names[i])==0)
				return 1;
		}
		strncpy(keys->names[keys->n], value, MAXKEYLEN-1);
		keys->n++;
		keys->len--;	//don't count system items
		return 1;
	} else
		return 1;
}

static char *currentSection;
static int findProfiles(const char *section, const char *key, const char *value, 
			const void *userdata)
{
	namelist_t *keys;
#define INI_MAXKEY 64
	char tmp[INI_MAXKEY];
	int i;

	keys = (namelist_t *) userdata;
	keys->len++;
	//PRINT("    [%s]\t%s=%s\n", section, key, value);
	if (strcmp(section, currentSection) == 0 && strcmp(&key[4], "profile") == 0) {	// skip preformat
		// check if it exists
		if (ini_getkey(value, 0, tmp, 10, inifile) == 0) {
			PRINT("no profile section:%s\n", value);
			return 0;
		}
		// avoid duplicates
		for(i=0; i<keys->n;i++) {
			if (strcmp(value, keys->names[i])==0)
				return 1;
		}
		strncpy(keys->names[keys->n], value, MAXKEYLEN-1);
		keys->n++;
		keys->len--;	//don't count system items
		return 1;
	} else
		return 1;
}

static char **stringList;
static int *offsetList;		// for storing offsets of items to avoid duplicates
static uint16_t listLength = 0;
static char noname[] = "NONAME";
//static char nostring[] = "NOSTRING";

/*
 *  add string to list
 *   search for duplicate, return index if found
 *   malloc mem and copy string
 *   return index
 */
static int addString(char *strIn)
{
	int len;
	uint16_t idx;
	char *str;

	// skip pre format
	if (strIn[0] == '$' || strIn[0] == '&')
		str = &strIn[4];
	else
		str = strIn;

	len = (int)strlen(str);
	for (idx = 0; idx < listLength; idx++) {
		/* Compare with the length, and without the length! */
		if (strncmp(stringList[idx], str, len) == 0) {
			if (strcmp(stringList[idx], str) == 0)
				return idx;
		}
	}
	// new string
	idx = listLength++;
	stringList[idx] = (char *) malloc(len + 1);	//include \0
	assert(stringList[idx] != 0);
	strcpy(stringList[idx], str);

	return idx;
}
#ifdef DUMP
/*
 * dump a descriptor depending on it's type
 *  after 1st pass processing
 */
static void dumpDsc1st(nxpTfaDescPtr_t * dsc)
{
	switch (dsc->type) { 
	case dscDevice:	// device list
		PRINT("device\n");
		break;
	case dscProfile:	// profile list
		PRINT("profile: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscMode:	// use case
		PRINT("use case: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscRegister:	// register patch
		PRINT("register patch: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscCfMem:	// coolflux mem
		PRINT("coolflux mem: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscString:	// ascii: zero terminated string
		PRINT("string: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscFile:		// filename + file contents
	case dscPatch:		// filename + file contents
		PRINT("file: %s\n", tfaContGetStringPass1(dsc));
		break;
	case dscBitfield:	// use case
		PRINT("bitfield: %s\n", tfaContGetStringPass1(dsc));
		break;
	}

}
static void dumpStrings(void)
{
	uint16_t idx;
	for (idx = 0; idx < listLength; idx++) {
		PRINT("[0x%0x] = %s\n", idx, stringList[idx]);
	}

}
static void dumpDevs(nxpTfaContainer_t * cont)
{
	nxpTfaDeviceList_t *dev;
	nxpTfaDescPtr_t *dsc;
	int i, d = 0;

	while ((dev = tfaContGetDevList(cont, d++)) != NULL) {
		dsc = &dev->name;
		if (dsc->type != dscString)
			PRINT("expected string: %s:%d\n", __FUNCTION__,
			       __LINE__);
		else
			PRINT("[%s] ", tfaContGetStringPass1(dsc));
		PRINT("devnr:%d length:%d bus=%d dev=0x%02x func=%d\n",	//TODO strings
		       dsc->offset, dev->length, dev->bus, dev->dev, dev->func);
		for (i = 0; i < dev->length; i++) {
			dsc = &dev->list[i];
			dumpDsc1st(dsc);
		}
	}
}

/*
 * dump container file
 */
static void dump(nxpTfaContainer_t * cont, int len)
{
	nxpTfaDescPtr_t *dsc = (nxpTfaDescPtr_t *) cont->index;
	uint32_t *ptr;
	int i;

	PRINT("id:%.2s version:%.2s subversion:%.2s\n", cont->id, cont->version, cont->subversion);
	PRINT("size:%d CRC:0x%08x rev:%d\n", cont->size, cont->CRC, cont->rev);
	PRINT("customer:%.8s application:%.8s type:%.8s\n", cont->customer, cont->application, cont->type);
	PRINT("base=%p\n", cont);

	if ( len<0) {
	len =  cont->size - sizeof(nxpTfaContainer_t);
	len /= sizeof(nxpTfaDescPtr_t);
	len++; // roundup
	}

	for (i = 0; i < len; i++) {
		ptr = (uint32_t *) dsc;
		PRINT("%p=dsc[%d]:type:0x%02x 0x%08x\n", ptr, i, dsc->type, *ptr);
		dsc++;
	}
}
#endif //DUMP
/*
 * process system section from ini file
 */
static int systemSection(nxpTfaContainer_t * head)
{
	int error = 0;
	char buf[sizeof(head->type)+1] = { 0 };
	head->id[0] = (char)(paramsHdr & 0xFF);
	head->id[1] = (char)(paramsHdr >> 8);
        if( tfa98xx_family_type == 2 )
                head->version[0] = NXPTFA_PM3_VERSION;
        else
                head->version[0] = NXPTFA_PM_VERSION;
		                	
	head->version[1] = '_';
	head->subversion[1] = NXPTFA_PM_SUBVERSION;
	head->subversion[0] = '0';

	head->customer[0] = 0;
	head->application[0] = 0;
	head->type[0] = 0;

	head->rev = (uint16_t) ini_getl("system", "rev", 0, inifile);
	ini_gets("system", "customer", "",  buf, sizeof(buf), inifile);
        memcpy(head->customer, buf, sizeof(head->customer));

	ini_gets("system", "application", "", buf, sizeof(buf), inifile);
        memcpy(head->application, buf,	 sizeof(head->application));

	ini_gets("system", "type", "", buf, sizeof(buf), inifile);
        memcpy(head->type, buf, sizeof(head->type));
		
	if(head->customer[0] == 0 || head->application[0] == 0 || head->type[0] == 0) {
		PRINT("Error: There is a empty key in the system section. \n");
		error = 1;
	}

	return error;
}

/*
 * fill the offset(
 */
nxpTfaDescPtr_t *tfaContSetOffset(nxpTfaDescPtr_t * dsc, int idx)
{
	int offset = sizeof(nxpTfaContainer_t) + idx * sizeof(nxpTfaDescPtr_t);

	dsc->offset = offset;

	return dsc;
}



/*
 * lookup the string in the stringlist and return a ptr
 *   this if for pass1 handling only
 *
 */
const char *tfaContGetStringPass1(nxpTfaDescPtr_t * dsc)
{
	if (dsc->offset >= listLength)
		return noname;
	return stringList[dsc->offset];
}

nxpTfaProfileList_t *tfaContFindProfile(nxpTfaContainer_t * cont,
					const char *name)
{
	int idx;
	nxpTfaProfileList_t *prof;
	const char *profName;

	prof = tfaContGet1stProfList(cont);

	for(idx=0;idx<cont->nprof;idx++) {
		profName = tfaContGetStringPass1(&prof->name);
		if (strcmp(profName, name) == 0) {
			return prof;
		}
		// next
		prof = tfaContNextProfile(prof);
		if (prof==NULL) {
			ERRORMSG("Illegal profile nr:%d\n", idx);
			return NULL;
		}
	}

	return NULL;
}

nxpTfaLiveDataList_t *tfaContFindLiveData(nxpTfaContainer_t * cont,
					const char *name)
{
	int idx;
	nxpTfaLiveDataList_t *lData;
	const char *LiveDataName;

	lData = tfaContGet1stLiveDataList(cont);

	for(idx=0;idx<cont->nliveData;idx++) {
		LiveDataName = tfaContGetStringPass1(&lData->name);
		if (strcmp(LiveDataName, name) == 0) {
			return lData;
		}
		// next
		lData = tfaContNextLiveData(lData);
		if (lData==NULL) {
			ERRORMSG("Illegal livedata nr:%d\n", idx);
			return NULL;
		}
	}

	return NULL;
}

/*
 * all lists are parsed now so the offsets to profile lists can be filled in
 *  the device list has a dsc with the profile name, this need to become the byte
 *  offset of the profile list
 */
static int tfaContFixProfOffsets(nxpTfaContainer_t * cont)
{
	int i = 0, profIdx;
	nxpTfaDeviceList_t *dev;
	nxpTfaProfileList_t *prof;

	// walk through all device lists
	while ((dev = tfaContGetDevList(cont, i++)) != NULL) {
		// find all profiles in this dev
		for (profIdx = 0; profIdx < dev->length; profIdx++) {
			if (dev->list[profIdx].type == dscProfile) {
				//dumpDsc1st(&dev->list[profIdx]);
				prof = tfaContFindProfile(cont, tfaContGetStringPass1(&dev->list[profIdx]));	// find by name
				if (prof) {
					dev->list[profIdx].offset = (uint32_t)((uint8_t *)prof - (uint8_t *)cont);	// fix the offset into the container
				} else {
					PRINT("Can't find profile:%s \n", tfaContGetStringPass1(&dev->list[profIdx]));
					return 1;
				}
			}
		}
	}

	return 0;
}

static int tfaContFixLiveDataOffsets(nxpTfaContainer_t * cont)
{
	int i = 0, lDataIdx;
	nxpTfaDeviceList_t *dev;
	nxpTfaLiveDataList_t *lData;

	// walk through all device lists
	while ((dev = tfaContGetDevList(cont, i++)) != NULL) {
		// find all livedata in this dev
		for (lDataIdx = 0; lDataIdx < dev->length; lDataIdx++) {
			if (dev->list[lDataIdx].type == dscLiveData) {
				lData = tfaContFindLiveData(cont, tfaContGetStringPass1(&dev->list[lDataIdx]));	// find by name
				if (lData) {
					dev->list[lDataIdx].offset = (uint32_t)((uint8_t *)lData - (uint8_t *)cont);	// fix the offset into the container
				} else {
					PRINT("Can't find livedata:%s \n", tfaContGetStringPass1(&dev->list[lDataIdx]));
					return 1;
				}
			}
		}
	}

	return 0;
}

/*
 * check if this item is already in the list
 */
static int tfaContHaveItem(nxpTfaDescPtr_t * dsc){
	unsigned int i;

	if ( !(dsc->type == dscBitfield) )
		for(i=0;i<listLength;i++)
			if ((unsigned int)offsetList[i]==dsc->offset)
				return 1;
	return 0;
}

// ; anti_alias 10..12
//;  type: low pass = lpf (high pass is not supported)
//;  cutOffFreq:  float Hertz
//;  rippleDb:    range: [0.1 3.0]
//;  rolloff:     range: [-1.0 1.0]
//;  samplingFreq: float Hertz (if not upplied the container file context will provide this)
//filter[10]=lpf,4000.0,0.4,-0.5,48000; anti_alias[0]
//filter[11]=lpf,1000.0,0.6,0.9,48000; anti_alias[0]
//filter[12]=lpf,500,2.9,0,48000; anti_alias[0]
//; integrator 13
//;  type: lpf|hpf = low/high pass
//;  cutOffFreq:  float Hertz
//;  leakage: range: ?
//;  samplingFreq: float Hertz (if not upplied the container file context will provide this)
//filter[12]=hpf,2000,0.2,48000

static int tfa_cont_filter_parse(uint8_t *dest ,char *line) 
{
	nxpTfaContBiquad_t		bq; /* binary data for container storage */
	nxpTfaAntiAliasFilter_t		anti_alias;
	nxpTfaEqFilter_t		equalizer;
	nxpTfaIntegratorFilter_t	integrator;
	char *strptr, eqtype[16],*cp;
	int i=0, cnt, type=0;
        char result=0;
	
	/* get filter index */
	if (sscanf(line,"filter[%hhd]", &bq.aa.index) != 1) /* for all variants */
		return -1; /* ini syntax error */

	/* Get the primary/secondary select */
	strptr = strchr(line, ',');
	if ( strptr==NULL )
		return -1; /* ini syntax error */
	strptr++;
	/* Get the primary/secondary select */
	if (sscanf(strptr,"%c]=", &result) != 1)
		return -1; /* ini syntax error */

	strptr = strchr(line, '=');
	if ( strptr==NULL )
		return -1; /* ini syntax error */
	strptr++;

	memcpy(eqtype, strptr, sizeof(eqtype));
	cp = strchr(eqtype, ',');
	if ( cp==NULL ) {
		PRINT_ERROR("illegal filter line:%s\n", line);
		return -1; /* ini syntax error */
	}
	*cp = '\0';
	/* get filter type */
	while (tfa98xx_eqtype_str[i] != NULL) {
		if (strcmp(eqtype,tfa98xx_eqtype_str[i]) == 0) {
			type = i;
			break;
		}
		i++;
	}
	if (tfa98xx_eqtype_str[i] == NULL) {
		PRINT_ERROR("unknown filter type in filter line:%s\n", line);
		return -1;/* ini syntax error, wrong filter type */
	}

	/* point to 1st float in argument list */
	strptr = strchr(strptr, ',');
	if ( strptr==NULL )
		return -1; /* ini syntax error */
	strptr++;
	//if (line)

	bq.aa.type = (uint8_t)type;
	switch (bq.aa.index) {
	case TFA_BQ_ANTI_ALIAS_INDEX:
	case TFA_BQ_ANTI_ALIAS_INDEX+1:
	case TFA_BQ_ANTI_ALIAS_INDEX+2:

	if (bq.aa.type == fElliptic) {
		cnt = sscanf(strptr, "%f,%f,%f,%f", &anti_alias.cutOffFreq, &anti_alias.rippleDb, &anti_alias.rolloff, &anti_alias.samplingFreq);
		if ( cnt < 3 ) {
			cnt++;
			PRINT_ERROR("The Elliptic filter type should have at least 3 parameters!: < %s > \n",line);
			return -1;
		}

		if ( cnt == 3) {
			anti_alias.samplingFreq = 48000;
		}

		filter_coeff_anti_alias(&anti_alias, tfa98xx_family_type);
		/* data for container storage */
		bq.aa.samplingFreq = anti_alias.samplingFreq;
		bq.aa.cutOffFreq = anti_alias.cutOffFreq;
		bq.aa.rippleDb = anti_alias.rippleDb;
		bq.aa.rolloff = anti_alias.rolloff;
		tfa98xx_convert_data2bytes(5, &anti_alias.biquad.a2, bq.aa.bytes);
	} else { /* eq */
		equalizer.type = (uint8_t)type;
		cnt = sscanf(strptr, "%f,%f,%f,%f", &equalizer.cutOffFreq, &equalizer.Q, &equalizer.gainDb, &equalizer.samplingFreq);
		if ( cnt < 3 ) {
			// TODO equalizer.samplingFreq = get fs from cnt
			cnt++;
			PRINT_ERROR("The EQ filter type should have at least 3 parameters!: < %s >\n",line);
			return -1;/* ini syntax error */
		}

		if ( cnt == 3) {
			equalizer.samplingFreq = 48000;
		}
		filter_coeff_equalizer(&equalizer, tfa98xx_family_type);
		/* data for container storage */
		bq.eq.samplingFreq = equalizer.samplingFreq;
		bq.eq.cutOffFreq = equalizer.cutOffFreq;
		bq.eq.Q = equalizer.Q;
		bq.eq.gainDb = equalizer.gainDb;
		tfa98xx_convert_data2bytes(5, &equalizer.biquad.a2, bq.eq.bytes);
	}
	break;
	case TFA_BQ_INTEGRATOR_INDEX:
		integrator.type = (uint8_t)type;
		cnt = sscanf(strptr, "%f,%f,%f", &integrator.cutOffFreq, &integrator.leakage, &integrator.samplingFreq);
		if ( cnt < 2 ) {
			PRINT_ERROR("The Integrator filter type should have at least 2 parameters!: < %s >\n",line);
			return -1;/* ini syntax error */
		}

		if ( cnt == 2) {
			integrator.samplingFreq = 8000;
		}

		filter_coeff_integrator(&integrator, tfa98xx_family_type);
		/* data for container storage */
		bq.in.samplingFreq = integrator.samplingFreq;
		bq.in.cutOffFreq = integrator.cutOffFreq;
		bq.in.leakage = integrator.leakage;
		tfa98xx_convert_data2bytes(5, &integrator.biquad.a2, bq.in.bytes);
		break;
	default:
		//	case TFA_BQ_EQ_INDEX ... TFA_BQ_EQ_INDEX+9:
		return -1; /* ini syntax error */
		break;

	}

	/* Get the primary/secondary select 
	 * When the user adds ,P the index is increased by 50 (primary)
	 * When the user adds ,S the index is increased by 100 (secondary)
	 */
	if(result == 'P') {
		bq.aa.index += 50;
	} else if(result == 'S') {
		bq.aa.index += 100;
	}

	memcpy(dest, &bq, sizeof(bq));
	return sizeof(bq);
}

/*
 *   Make sure no memory leak is created after using realloc
 *	 Always frees the buffer if realloc failed.
 */
static enum Tfa98xx_Error safe_realloc(char ***buffer, int size)
{
	void *temp_buffer=NULL;
	temp_buffer = realloc(*buffer, size);

	if(temp_buffer == NULL) {
		if(*buffer != NULL)
			free(*buffer);
		return Tfa98xx_Error_Fail;
	} else {
		*buffer = temp_buffer;
	}
	return Tfa98xx_Error_Ok;
}

/*
 *   append and fix the offset for this item
 *   the item will be appended to the container at 'size' offset
 *   returns the byte size of the item added
 */
static int tfaContAddItem(nxpTfaContainer_t *cont, nxpTfaDescPtr_t *dsc, nxpTfaLocationInifile_t *loc, int devidx)
{
	nxpTfaHeader_t existingHdr, *buf;
	nxpTfa98xxParamsType_t paramsType;
	nxpTfaRegpatch_t pat;
	nxpTfaMode_t cas;
	nxpTfaNoInit_t noinit;
	nxpTfaFeatures_t features;
	nxpTfaBitfield_t bitF;
	nxpTfaDescPtr_t *pDsc;
	nxpTfaMsg_t msg; 
	nxpTfaGroup_t grp;
	nxpTfaLiveData_t liveD;
	char fname[FILENAME_MAX], strLocation[FILENAME_MAX], profilename[64], cmd_str[32];
	int size = 0, ret, stringOffset = dsc->offset;
	int n_spaces=0, n_items_added=0, i=0;
	unsigned int j=0;
	uint8_t memtype, cnt;
	uint16_t address;
	char *s, *p, *cptr;
	const char *str;
	char **res = NULL;	
	uint8_t *dest = (uint8_t *) cont + cont->size;
	uint32_t value;
	FILE *f;
	memset(profilename, 0, sizeof(profilename));
	memset(&msg, 0, sizeof(msg));
	memset(&grp, 0, sizeof(grp));
	memset(&liveD, 0, sizeof(liveD));

	/* family is needed in case of datasheet name table lookups */
	if(tfa_dev_type[devidx] == 0) {
		if(tfa98xx_family_type == 2) {
			PRINT("Warning: No device type found in the patch file. Assuming 88 \n");
			tfa_dev_type[devidx] = 0x88;
		}
	}

	str = tfaContGetStringPass1(dsc);
	strncpy(fname, str, FILENAME_MAX-1);

	memset(strLocation, '\0', sizeof(strLocation));

	if(dsc->type == dscFile || dsc->type == dscPatch) {
#if defined(WIN32) || defined(_X64)
		char full[FILENAME_MAX];

		if(strcmp(_fullpath(full, str, FILENAME_MAX),str) != 0) strncpy(strLocation, loc->locationIni, FILENAME_MAX-1);
			strncat(strLocation, str, (FILENAME_MAX - strlen(strLocation)-1));
#else
		char *abs_path = realpath(str, NULL);

		if(abs_path != NULL) {
			if(strcmp(abs_path,str) != 0)
				strcpy(strLocation, loc->locationIni);
			strcat(strLocation, str);
		} else {
			strcpy(strLocation, loc->locationIni);
			strcat(strLocation, str);
			abs_path = realpath(strLocation, NULL);
		}
		free(abs_path);
#endif
	}

	switch (dsc->type) {
	case dscLiveDataString:
		p = strtok(fname, "="); /* Using fname instead of str because str is const */
		sscanf(p, "%24s", liveD.name);

		/* Reallocate strtok so that the first ([0]) element of the array is the cmdId */
		p = strtok (NULL, ", ");

		/* split string and append tokens to 'res' */ 
		while (p) {
			if(safe_realloc(&res, sizeof (char*) * ++n_spaces) != Tfa98xx_Error_Ok)
				return 1; // memory allocation failed */

			res[n_spaces-1] = p;

			p = strtok (NULL, ", ");
		}

		if(safe_realloc(&res, sizeof (char*) * (n_spaces+1)) != Tfa98xx_Error_Ok)
			return 1; // memory allocation failed */

		res[n_spaces] = 0;
		sscanf(res[0], "%24s", liveD.addrs);
		sscanf(res[1], "0x%x", &liveD.tracker);
		sscanf(res[2], "0x%x", &liveD.scalefactor);

		/* Take the number of words (used to read the cnt file)*/
		size = sizeof(liveD.name) + sizeof(liveD.addrs) + sizeof(liveD.tracker) + sizeof(liveD.scalefactor);
		memcpy(dest, &liveD, size);
		free (res);
		break;

	case dscSetInputSelect:
	case dscSetOutputSelect:
	case dscSetProgramConfig:
	case dscSetLagW:
	case dscSetGains:
	case dscSetvBatFactors:
	case dscSetSensesCal:
	case dscSetSensesDelay:
	case dscSetMBDrc:
		p = strtok(fname, "="); /* Using fname instead of str because str is const */

		/* Reallocate strtok so that the first ([0]) element of the array is the cmdId */
		p = strtok (NULL, ", ");

		/* split string and append tokens to 'res' */
		while (p) {
			if(safe_realloc(&res, sizeof (char*) * ++n_spaces) != Tfa98xx_Error_Ok)
				return 1; // memory allocation failed */

			res[n_spaces-1] = p;
			p = strtok (NULL, ", ");
		}

		/* realloc one extra element for the last NULL */
		if(safe_realloc(&res, sizeof (char*) * (n_spaces+1)) != Tfa98xx_Error_Ok)
			return 1; // memory allocation failed */
		res[n_spaces] = 0;

		sscanf(res[0], "0x%x", (int*)&(msg.cmdId));

		for (i = 1; i < n_spaces; ++i) {
			if(res[i] != NULL)
				sscanf(res[i], "0x%x", &(msg.data[i-1]));

			if(i-1 > 9) {
				PRINT("\nError: The payload of ");
				PRINT("%s is to big. \n", tfaContGetCommandString(dsc->type));

				free (res);
				return 1;
			}
		}

		/* Take the number of words (used to read the cnt file)*/
		msg.msg_size = (uint8_t)n_spaces - 1;

		size = sizeof(msg.cmdId) + ( sizeof(msg.data[0]) * (n_spaces-1)) + sizeof(msg.msg_size);
		memcpy(dest, &msg, size);

		free (res);
		break;

	case dscCmd:
		p = strtok(fname, "="); /* Using fname instead of str because str is const */

		/* Reallocate strtok so that the first ([0]) element of the array is the cmdId */
		p = strtok (NULL, ", ");
		if (p == NULL) {
			PRINT("cmd: command not specified\n");
			return -1;
		}

		/* first check for opcode in hex format */
		ret = sscanf(p, "0x%x", &value);
		if (ret <= 0) {
			/* check for opcode string descriptor */
			ret = sscanf(p, "%31s", cmd_str);
			if (ret <= 0) {
				PRINT("cmd: no or invalid opcode found\n");
				return -1;
			}

			i = 0;
			while (cmds[i].name != NULL) {
				if (strncmp(cmds[i].name, cmd_str, strlen(cmds[i].name)) == 0) {
					break;
				}
				i++;
			}
			if (cmds[i].name == NULL) {
				PRINT("cmd: unsupported command: %s\n", cmd_str);
				return -1;
			}

			value = cmds[i].id;

			/* sanity check on size of modifiers (should be modulo 3) */
			if (((strlen(cmd_str) - strlen(cmds[i].name)) % 3) != 0) {
				PRINT("cmd: error in modifiers in %s\n", cmd_str);
				return -1;
			}

			/* check for modifiers */
			for(j=(unsigned int)strlen(cmds[i].name); j<(unsigned int)strlen(cmd_str); j+=3) {
				if (strncmp(&cmd_str[j], "_DC", 3) == 0)
					value |= CMD_ID_DISABLE_COMMON;
				else if (strncmp(&cmd_str[j], "_DP", 3) == 0)
					value |= CMD_ID_DISABLE_PRIMARY;
				else if (strncmp(&cmd_str[j], "_DS", 3) == 0)
					value |= CMD_ID_DISABLE_SECONDARY;
				else if (strncmp(&cmd_str[j], "_SC", 3) == 0)
					value |= CMD_ID_SAME_CONFIG;
				else {
					PRINT("cmd: unsupported modifier in %s\n", cmd_str);
					return -1;
				}

			}

		}

		size = 2; /* reserve 2 bytes for the size */

		/* write command id */
		dest[size++] = (uint8_t)((value >> 16) & 0xff);
		dest[size++] = (uint8_t)((value >> 8) & 0xff);
		dest[size++] = (uint8_t)(value & 0xff);

		while (p) {
			p = strtok(NULL, ", ");
			if (p != NULL) {
				ret = sscanf(p, "0x%x", &value);
				if (ret <= 0) {
					PRINT("cmd: no or invalid value found\n");
					return -1;
				}

				/* write data */
				dest[size++] = (uint8_t)((value >> 16) & 0xff);
				dest[size++] = (uint8_t)((value >> 8) & 0xff);
				dest[size++] = (uint8_t)(value & 0xff);
			}
		}

		/* Size in first 2 bytes */
		*((uint16_t *)&dest[0]) = (uint16_t)(size - 2);
		break;

	case dscGroup:
		p = strtok(fname, "="); /* Using fname instead of str because str is const */
		p = strtok (NULL, ", ");

		/* split string and append tokens to 'res' */ 
		while (p) {
			if(safe_realloc(&res, sizeof (char*) * ++n_spaces) != Tfa98xx_Error_Ok)
				return 1; // memory allocation failed */

			res[n_spaces-1] = p;
			p = strtok (NULL, ", ");
		}

		/* realloc one extra element for the last NULL */
		if(safe_realloc(&res, sizeof (char*) * (n_spaces+1)) != Tfa98xx_Error_Ok)
			return 1; // memory allocation failed */

		res[n_spaces] = 0;

		/* number of items */
		for (i = 0; i < n_spaces; ++i) { 
			/* Temporarily store the profile name*/
			if(res[i] != NULL) {
				sscanf(res[i], "%63s", (char *)&profilename);
			}

			for(j=0; j<64; j++) {
				if(profile_order[j] == NULL) {
					break;
				} else if(strcmp(profilename, profile_order[j]) == 0) {
					/* Store the profile numbers */
					grp.profileId[i] = (uint8_t)j;
					n_items_added++;
					break;
				}
			}
		}

		if(n_spaces != n_items_added) {
			PRINT("Error: There are unrecognized profiles found inside a group! \n");
			free (res);
			return 1;
		}

        /* Take the number of words (used to read the cnt file)*/
		grp.msg_size = (uint8_t)n_spaces;

		size = (sizeof(grp.profileId[0]) * n_spaces) + sizeof(grp.msg_size);
		memcpy(dest, &grp, size);

		free (res);
		break;
	case dscFilter:
		size = tfa_cont_filter_parse(dest ,(char *)str);
		if( size <= 0 ){
			ERRORMSG("Wrong filter parameter syntax: %s\n", str);
			return 1;
		}
		break;
	case dscCfMem: /* @x[add]=val0[,val1,..valn] */
		switch (str[1]) {
		case 'p':
			memtype = Tfa98xx_DMEM_PMEM;
			break;
		case 'x':
			memtype = Tfa98xx_DMEM_XMEM;
			break;
		case 'y':
			memtype = Tfa98xx_DMEM_YMEM;
			break;
		case 'i':
			memtype = Tfa98xx_DMEM_IOMEM;
			break;
		default:
			ERRORMSG("Wrong coolflux mem key memtype syntax: %s\n", str);
			return 1;
		}
		/* get the address !look for 0x first */
		if ( (sscanf(&str[2], "[0x%x]=", (unsigned int *)&address)==1) || (sscanf(&str[2], "[%u]=", (unsigned int *)&address)==1) ) 
		{
			nxpTfaDspMem_t *cfmem=(nxpTfaDspMem_t *)dest;
			cfmem->address = address;
			cfmem->type = memtype;
			cfmem->size = 0;
			/* get the values */
			cnt=0;
			cptr = strchr(str, '='); // '=' is always there
			while(cptr != NULL) {
				cptr++;
				/* value !look for 0x first */
				if ( (sscanf(cptr, "0x%x", &value)==1) || (sscanf(cptr, "%d", &value)==1) )
				{
					cfmem->words[cnt++] = (value<<8)>>8; /* sign extend for two's complement */
					cfmem->size = cnt;
				}
				cptr = strchr(cptr, ','); // get next value if any
			}
			cfmem->size = cnt;
			size = sizeof(nxpTfaDspMem_t) + cnt*sizeof(int);
		} else {
			ERRORMSG("Illegal coolflux mem key memaddress syntax: %s\n", str);
			return 1;
		}
		break;
    case dscRegister:	// register patch : "$53=0x070,0x050"
		//PRINT("register patch: %s\n", str);
		if (sscanf(str, "$%hhx=%hx,%hx", &(pat.address), &(pat.value), &(pat.mask)) == 3) {
			size = sizeof(nxpTfaRegpatch_t);
			memcpy(dest, &pat, size);
		} else if (sscanf(str, "$%hhx=%hx", &(pat.address), &(pat.value)) == 2) {
			pat.mask = 0xffff;
			size = sizeof(nxpTfaRegpatch_t);
			memcpy(dest, &pat, size);
		} else 
			return 1;
		break;
	case dscBitfield:	//Bitfield : "_001RST=1"
		p = strtok(fname, "="); /* Using fname instead of str because str is const */
		p = strtok (NULL, ", ");

		/* split string and append tokens to 'res' */ 
		if(safe_realloc(&res, sizeof (char*)) != Tfa98xx_Error_Ok)
			return 1; // memory allocation failed */

		res[0] = p;
		/*checks of the value of bitfield in hex*/
		if((strncmp (res[0],"0x",2)==0)) {
			bitF.value = (uint16_t)strtol(res[0], NULL, 16);
		} else {
			sscanf(res[0], "%d", (int*)&(bitF.value));
		}

		strcpy(fname, str);
		s = strstr(fname, "=");
		/* Remove everything behind and including "=" to find the register field */
		fname[s - fname] = '\0';

		/* if not in any list */
		if ( tfaContBfEnumAny(&fname[4])==0xffff ) { //TODO get device idx for type
			PRINT("unknown bitfield: %s \n", fname);
			free(res);
			return 1;
		}

		bitF.field = tfaContBfEnum(&fname[4], (unsigned short)tfa_dev_type[devidx]);

		size = sizeof(nxpTfaBitfield_t);
		memcpy(dest, &bitF, size);

		free (res);
		break;
	case dscMode:
		if (sscanf(str, "mode=%d\n", &(cas.value)) == 1) {
			size = sizeof(nxpTfaMode_t);
			memcpy(dest, &cas, size);
		} else {
			int i=0;
			while (tfa98xx_mode_str[i] != NULL) {
				if (strcmp(&str[5],tfa98xx_mode_str[i]) == 0) {
					cas.value = i;
					size = sizeof(nxpTfaMode_t);
					memcpy(dest, &cas, size);
					break;
				}
				i++;
			}
			if (tfa98xx_mode_str[i] == NULL)
				return 1;
		}
		break;
	case dscNoInit:
		if (sscanf(str, "noinit=%d\n", (int *)&(noinit.value)) == 1) {
			size = sizeof(nxpTfaNoInit_t);
			memcpy(dest, &noinit, size);
		}
		break;
	case dscFeatures:
		p = strtok(fname, "="); /* Using fname instead of str because str is const */

		/* Reallocate strtok so that the first ([0]) element of the array is the cmdId */
		p = strtok (NULL, ", ");

		/* split string and append tokens to 'res' */
		while (p) {
			if(safe_realloc(&res, sizeof (char*) * ++n_spaces) != Tfa98xx_Error_Ok)
				return 1; // memory allocation failed */

			res[n_spaces-1] = p;
			p = strtok (NULL, ", ");
		}

		/* realloc one extra element for the last NULL */
		if(safe_realloc(&res, sizeof (char*) * (n_spaces+1)) != Tfa98xx_Error_Ok)
			return 1; // memory allocation failed */

		if (res != NULL) {
			res[n_spaces] = 0;
		}

		// if no 3 values (plus NULL), give error.
        if( n_spaces != 3 ) {
			PRINT("Error: %s is expected to contain 3 values. \n", fname);
			if (res != NULL)
				free(res);
			return 1;
		}

		sscanf(res[0], "0x%x", (int*)&(features.value[0]));
		sscanf(res[1], "0x%x", (int*)&(features.value[1]));
		sscanf(res[2], "0x%x", (int*)&(features.value[2]));

		size = sizeof(nxpTfaFeatures_t);
		memcpy(dest, &features, size);

		free (res);
		break;
	case dscString:	// ascii: zero terminated string
		strcpy((char*)dest, str);
		size = (int)strlen(str) + 1;	// include \n
		break;
	case dscFile:		// filename.dsc + size + file contents + filename.string
	case dscPatch:
		pDsc = (nxpTfaDescPtr_t * )dest; // for the filename
		dest += 4;
		size = fsize(strLocation);	// get the filesize
		*((uint32_t *)dest) = size; // store the filesize
		dest += 4;
		f=fopen(strLocation, "rb");
		if (!f) {
			PRINT("Unable to open %s\n", strLocation);
			return 1;
		}
		size = (int)fread(dest, 1, TFA_MAX_CNT_LENGTH - cont->size, f);
		rewind(f);
		fread(&existingHdr , sizeof(existingHdr), 1, f);
		fclose(f);

		/* We want to check CRC of every file! */
		tfaReadFile(strLocation, (void**) &buf);
		if (tfaContCrcCheck(buf)) {
			free(buf);
			ERRORMSG("CRC error in %s\n", fname);
			return 1;
	    }
        free(buf);
   
		paramsType = cliParseFiletype(fname);

		/* Check the header of each file */
		switch ( paramsType ) {
		case tfa_speaker_params:
			if(!(HeaderMatches(&existingHdr, speakerHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_patch_params:
			if(!(HeaderMatches(&existingHdr, patchHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_config_params:
			if(!(HeaderMatches(&existingHdr, configHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_vstep_params:
			if(!(HeaderMatches(&existingHdr, volstepHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_equalizer_params:
			if(!(HeaderMatches(&existingHdr, equalizerHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_drc_params:
			if(!(HeaderMatches(&existingHdr, drcHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_msg_params:
			if(!(HeaderMatches(&existingHdr, msgHdr))) {
				PRINT("Error: %s has no header! \n", fname);
				return 1;
			}
			break;
		case tfa_preset_params:
		case tfa_no_params:
        case tfa_cnt_params:
        default:
			/* Only to remove warnings in Linux */
			break;
		}
		pDsc->type = dscString;
		pDsc->offset = cont->size + size + sizeof(nxpTfaDescPtr_t)+4; // the name string is after file
		dest += size;
		str = basename((char*)tfaContGetStringPass1(dsc)); // Only save the filename
		strcpy((char*)dest, str);
		size += (int)(strlen(str) + 1 +sizeof(nxpTfaDescPtr_t)+4);	// include \n
		break;
	default:
		return 0;
		break;
	}
	offsetList[stringOffset] = dsc->offset = cont->size;
	cont->size += size;

	return 0;
}


/*
 * walk through device lists and profile list
 *  if to-be fixed
 */
static int tfaContFixItemOffsets(nxpTfaContainer_t * cont, nxpTfaLocationInifile_t *loc)
{
	int i, j = 0, maxdev = 0, error = 0;
	unsigned int idx=0;
	nxpTfaDeviceList_t *dev;
	nxpTfaProfileList_t *prof;
        nxpTfaLiveDataList_t *lData;

	offsetList = malloc(sizeof(int) * listLength);
	bzero(offsetList, sizeof(int) * listLength);	// make all entries 0 first
	
	// walk through all device lists
	while ((dev = tfaContGetDevList(cont, maxdev)) != NULL) 
	{
		// fix name first
		error = tfaContAddItem(cont, &dev->name, loc, maxdev);
		for (idx = 0; idx < dev->length; idx++) {
			error = tfaContAddItem(cont, &dev->list[idx], loc, maxdev);
			if(error != 0) {
				free(offsetList);
				return 1;
			}
		}
        maxdev++;
	}
	
	for (i = 0; i <= maxdev; i++) {
                j=0;
                // walk through all livedata lists
                while ((lData = tfaContGetDevLiveDataList(cont, i, j++)) != NULL) {
			// fix name first
			if (!tfaContHaveItem(&lData->name)){
				error = tfaContAddItem(cont, &lData->name, loc, i);
				for (idx = 0; idx < lData->length-1u; idx++) {
					error = tfaContAddItem(cont, &lData->list[idx], loc,i);
					if(error != 0) {
						free(offsetList);
						return 1;
					}
				}
			}
		}

		j=0;

                // walk through all profile lists
		while ((prof = tfaContGetDevProfList(cont, i, j++)) != NULL) {
			// fix name first
			if (!tfaContHaveItem(&prof->name)){
				error = tfaContAddItem(cont, &prof->name, loc,i);
				for (idx = 0; idx < prof->length; idx++) {
					error = tfaContAddItem(cont, &prof->list[idx], loc,i);
					if(error != 0) {
						free(offsetList);
						return 1;
					}
				}
			}
		}
	}

	free(offsetList);
	return error;
}

/* find device type from the ini file */
static int tfa_cnt_get_devidx_from_ini(const char *inifile, char *profile_name)
{
	FILE *in;
	char buf[MAXLINELEN], *ptr;
	int device = -1;
	char profile_name_found[MAXLINELEN];

	in = fopen(inifile, "r");
	if (in == 0) {
		PRINT("error: can't open %s for reading\n", inifile);
		return(1);
	}

	ptr = buf;
	while (!feof(in)) {
		if ( NULL==fgets(ptr, sizeof(buf), in))
			break;

		/* Find the device section of the container file */
		if (strncmp(ptr, "dev=", 4) == 0) {
			device++;
		}

		/* Look for the profiles */
		if (strncmp(ptr, "profile", 7) == 0) {	
			/* Remove the profile= part of the profile to make comparing easier */
			if (sscanf(ptr,"profile=%255s\n", profile_name_found)==1) {
				/* Compare both strings */
				if(strcmp(profile_name_found, profile_name) == 0) {
					fclose(in);
					return device;			
				}

				/* Clear the profile name each time */
				memset(&profile_name_found[0], 0, sizeof(profile_name_found));	
			}
		}

		ptr++;
	}
	fclose(in);

	/* if nothing is found, Take device 0 as default.. */
	return 0;
}

/*
 * pre-format ini file
 *  This is needed for creating unique key names for bitfield
 *  and register keys.
 *  It simplifies the processing of multiple entries.
 */
static int preFormatInitFile(const char *infile, const char *outfile)
{
	FILE *in, *out;
	char buf[MAXLINELEN], *ptr;
	int cnt = 0;
        char *bfName;
        char p[MAXLINELEN];

	in = fopen(infile, "r");
	if (in == 0) {
		PRINT("error: can't open %s for reading\n", infile);
		return 1;
	}
	out = fopen(outfile, "w");
	if (out == 0) {
		fclose(in);
		PRINT("error: can't open %s for writing\n", outfile);
		return 1;
	}

	ptr = buf;
	while (!feof(in)) {
		if ( NULL==fgets(ptr, sizeof(buf), in))
			break;
		// skip space
		while (isblank((unsigned)*ptr))
			ptr++;
		// ch pre format
		if (ptr[0] == '$')	// register
			PRINT_FILE(out, "$%03d%s", cnt++, ptr); 
		else if (strncmp(ptr, "profile", 6 ) == 0)
			PRINT_FILE(out, "&%03d%s", cnt++, ptr);
		else if (strncmp(ptr, "patch", 3 ) == 0)
			PRINT_FILE(out, "^%03d%s", cnt++, ptr);
		else if (strncmp(ptr, "msg", 3 ) == 0)
			PRINT_FILE(out, ".%03d%s", cnt++, ptr);
		else if (strncmp(ptr, "livedata", 8 ) == 0)
			PRINT_FILE(out, "+%03d%s", cnt++, ptr);
		else if (strncmp(ptr, "cmd", 3 ) == 0)
			PRINT_FILE(out, "*%03d%s", cnt++, ptr);
		else if (strncmp(ptr, "group", 5 ) == 0)
			PRINT_FILE(out, "-%03d%s", cnt++, ptr);
		else {
			strcpy(p, ptr);
			bfName = strtok(p, "=");
			/* check for any datasheet or bitfield name */
			if ( tfaContBfEnumAny(bfName) == 0xffff )
				fputs(ptr, out);
			else
				PRINT_FILE(out, "_%03d%s", cnt++, ptr);
		}
	}
	fclose(out);
	fclose(in);

	return 0;
}

/*
 *
 */
int tfaContSave(nxpTfaHeader_t *hdr, char *filename)
{
	FILE *f;
	int c, size;

	f = fopen(filename, "wb");
	if (!f) {
		PRINT("Unable to open %s\n", filename);
		return 0;
	}

	// check type
	if ( hdr->id == paramsHdr ) { // if container use other header
		size = ((nxpTfaContainer_t*)hdr)->size;
	} else
		size = hdr->size;

	c = (int)fwrite(hdr, size, 1, f);
	fclose(f);

	return c;
}

/*
 * create a big buffer to hold the entire container file
 *  return final file size
 */
int tfaContCreateContainer(nxpTfaContainer_t * contIn, char *outFileName, nxpTfaLocationInifile_t *loc)
{
	nxpTfaContainer_t *contOut;
	int size, error;
	char strLocation[FILENAME_MAX];
	uint8_t *base;

	contOut = (nxpTfaContainer_t *) malloc(TFA_MAX_CNT_LENGTH);
	if (contOut == 0) {
		PRINT("Can't allocate %d bytes.\n", TFA_MAX_CNT_LENGTH);
                return 0;
	}

	size = sizeof(nxpTfaContainer_t) + contIn->size * sizeof(nxpTfaDescPtr_t);	// still has the list count
	memcpy(contOut, contIn, size);
	//dump(contOut, contOut->size);
	contOut->size = size;	// now it's the actual size
	/*
	 * walk through device lists and profile list
	 *  if to-be fixed
	 */

	error = tfaContFixItemOffsets(contOut, loc);
	/*
	 * calc CRC over bytes following the CRC field
	 */
	if(size > 0 && error == 0) {
		base = (uint8_t *)&contOut->CRC + 4; // ptr to bytes following the CRC field
		size = (int)(contOut->size - (base-(uint8_t *)contOut)); // nr of bytes following the CRC field
		contOut->CRC = tfaContCRC32(base, size, 0u); // nr of bytes following the CRC field, 0);

		strncpy(strLocation, loc->locationIni, FILENAME_MAX-1);
		strncat(strLocation, outFileName, (FILENAME_MAX - strlen(strLocation)-1));
		tfaContSave((nxpTfaHeader_t *)contOut, strLocation);
		//dump(contOut, -1);
		size = contOut->size;
	} else {
		size = 0;
	}
	free(contOut);

	return size;
}
/*
 * read the first byte from the patchfile payload and store in
 * the module global to be used later for family tyoe selection
 *
 * no error checking done at this point:
 *   assume devidx is ok
 *   patch file will be checked later
 */
int tfa_cont_set_patch_dev(char *configpath, char *patchfile, int devidx) {
	nxpTfaLocationInifile_t *loc = malloc(sizeof(nxpTfaLocationInifile_t));
	char filename[FILENAME_MAX], patchfilename[FILENAME_MAX];
	char cwdname[FILENAME_MAX];
	FILE *f;
	int dev=-1;

	memset(loc->locationIni, '\0', sizeof(loc->locationIni));

	/* Get the basename */
	strcpy(filename,  basename(patchfile));

	/* Set the path to the folder */
	getcwd(cwdname, FILENAME_MAX); /* save current */
	chdir(configpath);

	/* Get the absolute path */
	tfaGetAbsolutePath(patchfile, loc);

	strcpy(patchfilename, loc->locationIni);
	strcat(patchfilename, filename);

	/* check if valid */
	if (Tfa98xx_Error_Ok != tfa98xx_cnt_loadfile(patchfilename, 0) ) {
		PRINT_ERROR("rejected patchfile:%s\n", patchfilename);
		goto error_exit;
	}
	f=fopen(patchfilename, "r+b");
	if ( !f ) {
		ERRORMSG("Error when reading %s.\n", patchfilename);
		goto error_exit;
	}

	fseek(f, sizeof(nxpTfaPatch_t), SEEK_SET); /* point to payload */

	dev = fgetc(f); /* get the 1st byte */
	fclose(f);

error_exit:

	if (tfa98xx_cnt_write_verbose)
		PRINT("dev[%d]:patch file %s = 0x%02x\n\n", devidx,patchfile, dev);

	tfa_dev_type[devidx] = dev;

	free(loc);
	chdir(cwdname); /* back home */

	if( dev<0 || dev>0x254) {
		PRINT("Error: Unknown device type :%d\n", dev);
		dev=-1;
	} else {
		tfa98xx_family_type = tfa98xx_dev2family(dev);
	}

	return dev;
}

int tfaContParseIni( char *iniFile, char *outFileName, nxpTfaLocationInifile_t *loc) 
{
	int k, i, j, error = 0, idx = 0;
	int keyCount[10] = {0};
	char value[100], key[100];
	nxpTfaContainer_t *headAndDescs;
	namelist_t keys, profiles, livedata;
	nxpTfaDescPtr_t *dsc;
	nxpTfaDeviceList_t *dev;
	nxpTfaLiveDataList_t *lData;
	nxpTfaProfileList_t *profhead;
	char preformatIni[FILENAME_MAX];
	char tmp[INI_MAXKEY];

	/* Grouping */
	/* First array are the number of profiles, Second is the string (profile names in the group) */
	char group_names[64][FILENAME_MAX];
	static int group_number = 1; /* Group 0 is default (no grouping) */

	strncpy(preformatIni, loc->locationIni, FILENAME_MAX-1);
	strncat(preformatIni, outFileName, (FILENAME_MAX - strlen(preformatIni)-1));
	strncat(preformatIni,  "_preformat.ini", (FILENAME_MAX - strlen(preformatIni)-1));
	if(preFormatInitFile(iniFile, preformatIni))
		return 0;

	inifile = preformatIni; // point to the current inifile (yes, this is dirty ..)
	VERBOSE PRINT("preformatted ini=%s\n", inifile);
	/*
	 * process system section
	 */
	// find devices in system section
	keys.n = 0;		// total nr of devices
	profiles.n = 0;
	livedata.n = 0;
	keys.len = 0;		// maximum length of all desc lists
	ini_browse(findDevices, &keys, preformatIni);	//also counts entries
	// get storage for container header and dsc lists
	headAndDescs = (nxpTfaContainer_t *)malloc(sizeof(nxpTfaContainer_t) + keys.n*2 * keys.len * 4+4*keys.n*4);	//this should be enough
	if(headAndDescs == 0)
		return 0;
	bzero(headAndDescs, sizeof(nxpTfaContainer_t) + keys.n*2 * keys.len * 4+4*keys.n*4);
	stringList = malloc(fsize(preformatIni));	//strings can't be longer the the ini file
	assert(headAndDescs != 0);
	/*
	 * system settings
	 */
	error = systemSection(headAndDescs);
	if(error) {
		if (!tfa98xx_cnt_write_verbose)
			remove(preformatIni);

		free(headAndDescs);
		for (j = 0; j < listLength; j++)
			free(stringList[j]);
		free(stringList);
		listLength = 0;
		return 0;
	}
	
	/*
	 * next is the creation of device and profile initlists
	 *  these lists consists of index into a stringtable (NOT the final value!)
	 *  processing to actual values will be the last step
	 */

	// devices
	headAndDescs->ndev = (uint16_t)keys.n; // record nr of devs
	dsc = (nxpTfaDescPtr_t *) headAndDescs->index;

	// create idx initlist with names
	for (i = 0; i < keys.n; i++) {
		dsc->type = dscDevice;
		dsc->offset = addString(keys.names[i]);
		dsc++;
	}
	dsc->type = dscMarker;	// mark end of devlist
	dsc->offset = 0xa5a5a5;	// easy to read
	dsc++;

	/* For every device section find the patch and set device type */
	for (i = 0; i < keys.n; i++) {
		dev = (nxpTfaDeviceList_t *) dsc;	//1st device starts after idx list

		// get the dev keys
		for (k = 0; ini_getkey(keys.names[i], k, key, sizearray(key), preformatIni) > 0; k++) {
			dsc->type = parseKeyType(key);

			/* Find the patch type */
			if((int)dsc->type == dscPatch) {				
				ini_gets(keys.names[i], key, "", value,sizeof(value), preformatIni);

				/* record the device type from the patch file for this device */
				if(tfa_cont_set_patch_dev(loc->locationIni, value, i) < 0)
					error = 1;		
			}
		}
	}

	// create device initlist
	for (i = 0; i < keys.n; i++) {
		dev = (nxpTfaDeviceList_t *) dsc;	//1st device starts after idx list
		currentSection = keys.names[i];
		VERBOSE PRINT("dev:%s\n", currentSection);
		dev->bus = (uint8_t) ini_getl(keys.names[i], "bus", 0, preformatIni);
		dev->dev = (uint8_t) ini_getl(keys.names[i], "dev", 0, preformatIni);
		dev->func = (uint8_t) ini_getl(keys.names[i], "func", 0, preformatIni);
		dsc++;
		dev->devid = (uint32_t) ini_getl(keys.names[i], "devid", 0, preformatIni);
		dsc++;
		// add the name
		dev->name.type = dscString;
		dev->name.offset = addString(currentSection);
		dsc++;
		// get the dev keys
		for (k = 0; ini_getkey(keys.names[i], k, key, sizearray(key), preformatIni) > 0; k++) {
			VERBOSE PRINT("\tkey: %d %s \n", k, key);
			keyCount[i]++;
			dsc->type = parseKeyType(key);
			switch ((int)dsc->type) {
			case dscString:	// ascii: zero terminated string
				PRINT("Warning: In section: [%s] the key: %s is used as a string! \n", currentSection, key);
				break;
			case dscFile:	// filename + file contents
			case dscProfile:
			case dscLiveData:
				if (ini_gets(keys.names[i], key, "", value,sizeof(value), preformatIni)) {
					dsc->offset = addString(value);
					dsc++;
				}
				break;
			case dscPatch:
				if (ini_gets(keys.names[i], key, "", value,sizeof(value), preformatIni)) { 
					dsc->offset = addString(value);
					dsc++;
				}
				break;
			case dscBitfield:
				/* Search bitfields in the list for the rev-id from the patch! */
				if (tfaContBfEnum(&key[4], (unsigned short)tfa_dev_type[i]) == 0xffff) {
					PRINT("unknown bitfield: %s \n", key);
					error = 1;
				}
				/* no break (deliberate) */
			case dscSetInputSelect:
			case dscSetOutputSelect:
			case dscSetProgramConfig:
			case dscSetLagW:
			case dscSetGains:
			case dscSetvBatFactors:
			case dscSetSensesCal:
			case dscSetSensesDelay:
			case dscSetMBDrc:
			case dscCmd:
			case dscMode:
			case dscCfMem:
			case dscFilter:
			case dscNoInit:
			case dscFeatures:
			case dscRegister:	// register patch e.g. $53=0x070,0x050
				strcpy(value, key);	//store value with key
				strcat(value, "=");
				if (ini_gets(keys.names[i], key, "", &value[strlen(key) + 1], sizeof(value) - (int)(strlen(key)) - 1, preformatIni)) {
					dsc->offset = addString(value);
					dsc++;
				}
				break;
			case dscGroup:
				strcpy(value, key);	//store value with key
				strcat(value, "=");
				if (ini_gets(keys.names[i], key, "", &value[strlen(key) + 1], sizeof(value) - (int)(strlen(key)) - 1, preformatIni)) {
					dsc->offset = addString(value);
					dsc++;

					/* Save the string for each group */
					strcpy(group_names[group_number], value);
					group_number++;
				}
				break;
			case dscDevice:	// device list
				PRINT("error: skipping illegal key:%s\n", key);
				break;
			case -1:
				continue;
				break;
			default:
				VERBOSE PRINT(" skipping key:%s\n", key);
				break;
			}
		}
		idx = (int)((uint32_t *) dsc - (uint32_t *) (dev) - sizeof(nxpTfaDeviceList_t) / 4);
		dev->length = (uint8_t)idx;	//store the total list length (inc name dsc)

		// If there are no previous errors get the profiles and livedata names
		if(error == 0) {
			error = ini_browse(findLivedata, &livedata, preformatIni);	//also counts entries 
			error = ini_browse(findProfiles, &profiles, preformatIni);	//also counts entries      
		}
	}

	for(i=0; i<keys.n; i++) {
		if(keyCount[i] == 0) {
			PRINT("Error: There are not enough device sections\n");
			error = 1;
		}
	}

	if((keys.n < 2) && (ini_getkey("left", 0, tmp, 10, inifile) > 0) && (ini_getkey("right", 0, tmp, 10, inifile) > 0)) {
		PRINT("Error: There are not enough device keys in the system section\n");
		error = 1;
	}

	dsc = (nxpTfaDescPtr_t *) headAndDescs->index;
	tfaContSetOffset(dsc, keys.n + 1);	//+ marker first is after index

	for (i = 0; i < keys.n - 1; i++) {	// loop one less from total, 1st is done
		dev = tfaContGetDevList(headAndDescs, i);
		idx = (i+1)*dev->length;	// after this one
		idx++;		// skip marker
		dsc++;		// the next dev list
		tfaContSetOffset(dsc, idx + keys.n + (i+1)*sizeof(nxpTfaDeviceList_t) / 4); // the device list offset is updated
	}

	/*
	 * create profile initlists
	 */
	headAndDescs->nprof = (uint16_t)profiles.n; // record nr of profiles
	dsc = (nxpTfaDescPtr_t *) tfaContGet1stProfList(headAndDescs);	// after the last dev list
	for (i = 0; i < profiles.n; i++) {
		profhead = (nxpTfaProfileList_t *) dsc;
		profhead->ID = TFA_PROFID;
		dsc++;
		currentSection = profiles.names[i];
		VERBOSE PRINT("prof: %s\n", currentSection);
		
		/* Check length of field (application + profile), if it is too large print
		   a warning as it will not fit in the alsa-mixer panel. The "." in the 
		   profile name is discarded for the length, it is not used in the mixer */
		{
			int app_len = 0, pro_len = 0, tot_len = 0, idx = 0;
			char *pch = NULL;
            
			pch = strchr(currentSection, '.');
			idx = (int) (pch - currentSection);
			pro_len = (pch != NULL) ? idx : (int) strlen(currentSection);
			app_len = (int) strlen(headAndDescs->application);
			tot_len = app_len + pro_len;
            
			if (tot_len > 26) {
				PRINT("Warning: length of field (application + profile = %d) is too long for alsa-mixer! (26 max.)\n", tot_len);
			}
		}
		
		// start with name
		profhead->name.type = dscString;
		profhead->name.offset = addString(currentSection);
		dsc = profhead->list;

		/* Save the profile order (used for group section in tfaContAddItem) */
		profile_order[i] = currentSection;
		profhead->group = 0; /* default is no group (0)*/
		for (j = 1; j < group_number; j++) { 
			/* If the group string contains the profilename save the group number */
			if(strstr(group_names[j],currentSection) != NULL) {
				if(j>1 && profhead->group !=0) {
					PRINT("Error: It is not allowed to have the same profile (%s) in multiple groups! \n", currentSection);
					error = 1;
				}
				profhead->group = j;
			}
		}

		/* Find the device section for the current profile */
		j = tfa_cnt_get_devidx_from_ini(iniFile, currentSection);

		// get the profile keys and process them
		for (k = 0; ini_getkey(profiles.names[i], k, key, sizearray(key), preformatIni) > 0; k++) {
			VERBOSE PRINT("\tkey: %d %s \n", k, key);
			dsc->type = parseKeyType(key);
			switch (dsc->type) {
			case dscString:	// ascii: zero terminated string
				PRINT("Warning: In section: [%s] the key: %s is used as a string! \n", currentSection, key);
				break;
			case dscFile:	// filename + file contents
			case dscPatch: // allow patch in profile
				if (ini_gets(profiles.names[i], key, "", value, sizeof(value), preformatIni)) {
					dsc->offset = addString(value);
					dsc++;
				}
				break;
			case dscDefault:
				dsc->offset = addString("23");
				dsc++;
				break;
			case dscBitfield:
				/* Search bitfields in the list for the rev-id from the patch! */
				if (tfaContBfEnum(&key[4], (unsigned short)tfa_dev_type[j]) == 0xffff) {
					PRINT("unknown bitfield: %s \n", key);
					error = 1;
				}
				/* no break (deliberate) */	
			case dscSetProgramConfig:
				if((dsc->type == dscSetProgramConfig) && (profhead->group != 0)) {
					PRINT("Error: It is not allowed to have a profile(%s) containing SetProgramConfig inside a Group! \n", currentSection);
					error = 1;
				}
				/* no break (deliberate) */
			case dscSetInputSelect:
			case dscSetOutputSelect:
			case dscSetLagW:
			case dscSetGains:
			case dscSetvBatFactors:
			case dscSetSensesCal:
			case dscSetSensesDelay:
			case dscSetMBDrc:
			case dscCmd:
			case dscMode:
			case dscCfMem:
			case dscFilter:
			case dscNoInit:
			case dscFeatures:
			case dscRegister:	// register patch e.g. $53=0x070,0x050
				strcpy(value, key);	//store value with key
				strcat(value, "=");
				if (ini_gets(profiles.names[i], key, "", &value[strlen(key) + 1], sizeof(value) - (int)(strlen(key)) - 1, preformatIni)) {
					dsc->offset = addString(value);
					dsc++;
				}
				break;
			case dscProfile:
			case dscDevice:
			case dscLiveData:
				PRINT("error: skipping illegal key:%s\n", key);
				break;
			default:
				VERBOSE PRINT(" skipping key:%s\n", key);
				break;
			}
		}

		idx = (int)(dsc - &profhead->list[0]);
		profhead->length = idx + 1;	//store the total list length (exc header, inc name dsc)

	}	/* device lists for loop end */

	dsc->type = dscMarker;	// mark end of proflists
	dsc->offset = 0x555555;	// easy to read
	dsc++;

	if(error) {
		if (!tfa98xx_cnt_write_verbose)
			remove(preformatIni);
		free(headAndDescs);
		for (j = 0; j < listLength; j++)
			free(stringList[j]);
		free(stringList);
		listLength = 0;
		return 0;
	}

    /*
	* create livedata initlists
	*/
	headAndDescs->nliveData = (uint16_t)livedata.n; // record nr of livedata
	if(livedata.n > 0) {
		dsc = (nxpTfaDescPtr_t *) tfaContGet1stLiveDataList(headAndDescs);	// after the last prof list
		for (i = 0; i < livedata.n; i++) {
			lData = (nxpTfaLiveDataList_t *) dsc;	//
			lData->ID = TFA_LIVEDATAID;
			dsc++;
			currentSection = livedata.names[i];
			VERBOSE PRINT("livedata: %s\n", currentSection);
			// start with name
			lData->name.type = dscString;
			lData->name.offset = addString(currentSection);
			dsc++;
			// get the livedata keys and process them
			for (k = 0; ini_getkey(livedata.names[i], k, key, sizearray(key), preformatIni) > 0; k++) {
				VERBOSE PRINT("\tkey: %d %s \n", k, key);
				dsc->type = dscLiveDataString; //This can be anything, so we need to make sure it can be distinguished from dscString
				strcpy(value, key);	//store value with key
				strcat(value, "=");
				if (ini_gets(livedata.names[i], key, "", &value[strlen(key) + 1], sizeof(value) - (int)(strlen(key)) - 1, preformatIni)) {
					dsc->offset = addString(value);
					dsc++;
				}
			}

			if(k > MEMTRACK_MAX_WORDS) {
				PRINT("Error: Too many livedata items! Found:%d Allowed:%d \n", k, MEMTRACK_MAX_WORDS);
				error = 1;
			}

			idx = (int)((uint32_t *) dsc - (uint32_t *) (lData));
			lData->length = idx - 1;	//store the total list length (exc header, inc name dsc)
		}	/* device lists for loop end */
        
		dsc->type = dscMarker;	// mark end of proflists
		    dsc->offset = 0x777777;
		    dsc++;
	}

	if(error) {
		if (!tfa98xx_cnt_write_verbose)
			remove(preformatIni);
		free(headAndDescs);
		for (j = 0; j < listLength; j++)
			free(stringList[j]);
		free(stringList);
		listLength = 0;
		return 0;
	}

	/*
	* all lists are parsed now so the offsets to profile lists can be filled in
	*  the device list has a dsc with the profile name, this need to become the byte
	*  offset of the profile list
	*/
	error = tfaContFixProfOffsets(headAndDescs);
	error = tfaContFixLiveDataOffsets(headAndDescs);

	if(error == 0) {
		headAndDescs->size = (uint32_t)(((uint32_t *)dsc - (uint32_t *)(headAndDescs->index))+1);	//total
		i = tfaContCreateContainer(headAndDescs, outFileName, loc);
	} else {
		/* Creating cnt should fail */
		i = 0; 
	}

	/* if no verbose is given the no ini file should be created */
	if (!tfa98xx_cnt_write_verbose)
		remove(preformatIni);

	free(headAndDescs);
	for (j = 0; j < listLength; j++)
		free(stringList[j]);
	free(stringList);
	listLength = 0;
	return i; // return size
}

/*
 * file type names for enum type nxpTfa98xxParamsType for user-friendly messages
 */
static const char *filetypeName[] = {
		"patch file" ,
		"speaker file" ,
		"preset file" ,
		"config file" ,
		"equalizer file" ,
		"drc file" ,
		"vstep file" ,
		"cnt file" ,
		"msg file" ,
		"unknown file",
		"info file"
};

/*
 *
 */
nxpTfa98xxParamsType_t cliParseFiletype(char *filename)
{
	char *ext;
	nxpTfa98xxParamsType_t ftype;

	// get filename extension
	ext = strrchr(filename, '.'); // point to filename extension

	if ( ext == NULL ) {
		ftype = tfa_no_params;	// no '.'
	}// now look for supported type
	else if ( strcmp(ext, ".patch")==0 )
		ftype = tfa_patch_params;
	else if ( strcmp(ext, ".speaker")==0 )
		ftype = tfa_speaker_params;
	else if ( strcmp(ext, ".config")==0 )
		ftype = tfa_config_params;
	else if ( strcmp(ext, ".eq")==0 )
		ftype = tfa_equalizer_params;
	else if ( strcmp(ext, ".drc")==0 )
		ftype = tfa_drc_params;
	else if ( strcmp(ext, ".vstep")==0 )
		ftype = tfa_vstep_params;
	else if ( strcmp(ext, ".cnt")==0 )
		ftype = tfa_cnt_params;
	else if ( strcmp(ext, ".preset")==0 )
		ftype = tfa_preset_params;
	else if ( strcmp(ext, ".msg")==0 )
		ftype = tfa_msg_params;
	else 
		ftype = tfa_info_params; /* Default we assume info file */

	if (ftype != tfa_no_params && tfa98xx_cnt_write_verbose )
    	        PRINT("file %s is a %s.\n" , filename, filetypeName[ftype]);

    return ftype;
}

/*
 * Funtion to split the container file into individual files and ini file
 */
int tfa98xx_cnt_split(char *fileName, char *directoryName) {
	nxpTfaLocationInifile_t *loc = malloc(sizeof(nxpTfaLocationInifile_t));
	nxpTfaDeviceList_t *dev;
	nxpTfaProfileList_t *prof;
	nxpTfaFileDsc_t *file;
	nxpTfaHeader_t *buf = NULL;
	nxpTfaHeader_t *new_buf = NULL;
	nxpTfaContainer_t* cont;
	int error = Tfa98xx_Error_Ok;
	int i=0, j=0, k=0, length, size = 0;
	unsigned int m=0;
	char *filename;
    char *buffer = (char *) calloc(NXPTFA_MAXBUFFER, sizeof(char)); /* 50x1024 bytes is the maximum allowed buffer size */
	char completeName[FILENAME_MAX]={0}, cntname[FILENAME_MAX]={0};
	char timeString[20]={0};
	struct tm * time_info;
	time_t current_time = time(NULL);

    if(buffer == NULL) {
        PRINT("Buffer memory allocation failed!\n");
        return Tfa98xx_Error_Fail;
    }
    
    if(loc == 0) {
        PRINT("Memory allocation failed!\n");
        return Tfa98xx_Error_Fail;
    }

    size = tfaReadFile(fileName, (void**) &buf); //mallocs file buffer
    if (size == 0) {
		free(loc);
		free(buf);
		return Tfa98xx_Error_Bad_Parameter; // tfaReadFile reported error already
	}

    cont = (nxpTfaContainer_t*) buf;
	memset(loc->locationIni, '\0', sizeof(loc->locationIni));

	tfaGetAbsolutePath(fileName, loc);	
	if ( tfa98xx_cnt_write_verbose )
		PRINT("Path found: %s \n", loc->locationIni);

	/* Only save the basename */
	fileName = basename(fileName);

	/* Get the name of basename (Remove extention cnt (3)) */
	length = (int)strlen(fileName)-3;
	strncpy(cntname, fileName, length);

	/* Check if there is a directory path given */
	if(directoryName != NULL) {
		/* Add folder to place the files */
		strcat(loc->locationIni, basename(directoryName));

		/* Get the time */
		time(&current_time);
		time_info = localtime(&current_time);

		/* Convert to string usable for making directory */
		strftime(timeString, sizeof(timeString), "_%m-%d-%Y", time_info);

		if (strlen(timeString)==0) {		
			strcat(loc->locationIni, timeString);	
		} else {
			PRINT("Failure to get the current time \n");
		}	

		strcat(loc->locationIni, "/");

		/* Create new folder where the files are placed (to avoid overwriting) */
#if defined (WIN32) || defined(_X64)
		if(mkdir(loc->locationIni) != 0) 
			PRINT("Failed to create directory: %s \n", loc->locationIni);
#else
		if(mkdir(loc->locationIni, 0777) != 0)
			PRINT("Failed to create directory: %s \n", loc->locationIni);
#endif
	}

    error = tfaContShowContainer(buffer, NXPTFA_MAXBUFFER);
	length = (int)(strlen(buffer));
	*(buffer+length) = '\0';

	if ( tfa98xx_cnt_write_verbose ) {
		puts(buffer);
		fflush(stdout);
	}

	length = (int)strlen(loc->locationIni); 
	memcpy(completeName, loc->locationIni, length);
	strcat(completeName, cntname);
	strcat(completeName, "ini"); 
	error = tfaosal_filewrite(completeName, (unsigned char *)buffer, (int)(strlen(buffer)));
	memset(&completeName[0], 0, sizeof(completeName));

        if ( tfa98xx_cnt_write_verbose )
    	        PRINT("Warning: File with same name will be overwritten.\n");

	for(i=0 ; i < cont->ndev ; i++) {
		dev = tfaContGetDevList(cont, i);
		if( dev==0 ) {
			error = Tfa98xx_Error_Fail;
			goto error_exit;
		}

		/* process the device list until a file type is encountered */
		for(j=0;j<dev->length;j++) {
			if ( dev->list[j].type == dscFile || dev->list[j].type == dscPatch) {
				file = (nxpTfaFileDsc_t *)(dev->list[j].offset+(uint8_t *)cont);
				new_buf = (nxpTfaHeader_t *)file->data;
				/* write this file out */
			    if ( tfa98xx_cnt_write_verbose ) {
			    	PRINT("%s: device %s file %s found of type %s\n", __FUNCTION__,
						tfaContDeviceName(i), tfaContGetString(&file->name),
						tfaContFileTypeName(file));
				}

				strcpy(completeName, loc->locationIni);
			    filename = tfaContGetString(&file->name);	// to remove any relative path
				filename = basename(filename);
				strcat(completeName, filename); // Add filename to the absolute path

				/* Remove the header from the info file */
				if(strcmp(tfaContFileTypeName(file), "info") == 0) {
					error = tfaosal_filewrite(completeName, 
						(unsigned char *)new_buf+sizeof(nxpTfaHeader_t), file->size-sizeof(nxpTfaHeader_t));
				} else {
					error = tfaosal_filewrite(completeName, (unsigned char *)new_buf, file->size );
				}

				memset(&completeName[0], 0, sizeof(completeName));
			}
		}
		/* Next, look in the profile list until the file type is encountered
		 */
		for (k=0; k < tfaContMaxProfile(i); k++ ) {
			prof=tfaContGetDevProfList(cont, i, k);
			for(m=0;m<prof->length;m++) {
				if (prof->list[m].type == dscFile) {
					file = (nxpTfaFileDsc_t *)(prof->list[m].offset+(uint8_t *)cont);
					new_buf = (nxpTfaHeader_t *)file->data;
					/* write file type */
				    if ( tfa98xx_cnt_write_verbose ) {
				    	PRINT("%s: profile %s file %s found of type %s\n", __FUNCTION__,
							tfaContProfileName(i,k), tfaContGetString(&file->name),
							tfaContFileTypeName(file));
					}

					strcpy(completeName, loc->locationIni);
				    filename = tfaContGetString(&file->name);	// to remove any relative path
					filename = basename(filename);
					strcat(completeName, filename); // Add filename to the absolute path
					error = tfaosal_filewrite(completeName, (unsigned char *)new_buf, file->size );
					memset(&completeName[0], 0, sizeof(completeName));
				}
			}
		}
	}
	if (error == Tfa98xx_Error_Ok) {
		PRINT("The file is split at location: %s \n", loc->locationIni);
		PRINT("The container file is split into separated device files. \n");
	}

error_exit:

    if (buffer)
		free (buffer);
	if (loc)
		free (loc);
	if (buf) {
		free (buf); // dev, prof and cont point to memory alocated for buf
	}

	return error;
}

/*
 *   load the file depending on its type
 *   normally the container will be loaded
 *   any other valid type will by send to the target device(s)
 */
enum Tfa98xx_Error tfa98xx_cnt_loadfile(char *fname, int cnt_verbose) {
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	int length, size, i=1;
	nxpTfaHeader_t *buf = 0;
	nxpTfaHeaderType_t type;
	char *temp_buffer=NULL, *buffer=NULL;

	size = tfaReadFile(fname, (void**) &buf); /* mallocs file buffer */

	if (size == 0) {
		free(buf);
		return Tfa98xx_Error_Bad_Parameter; /* tfaReadFile reported error already */
	}

	type = (nxpTfaHeaderType_t) buf->id;
	if (type == paramsHdr) {  /* Load container file */

		error = tfa_load_cnt( (void*) buf, size);
		gCont = (nxpTfaContainer_t*) buf;

		if (error == Tfa98xx_Error_Ok && cnt_verbose) {
			do {
				if(i > 50) /* 50x1024 chars/bytes is the maximum allowed buffer size */
					break;
				/* allocate per blocks of 1024 bytes/chars */
				temp_buffer = (char*) realloc(buffer, i*1024); 
				if(temp_buffer == NULL) {
                    if(buffer != NULL)
                        free(buffer);
                } else {
                    buffer = temp_buffer;
                }
				memset(buffer, 0, i*1024); /* clear the entire buffer */
				error = tfaContShowContainer(buffer, i*1024);
				i++;
			} while(error == Tfa98xx_Error_Buffer_too_small);
			length = (int)(strlen(buffer));
			*(buffer+length) = '\0';        /* terminate */
			puts(buffer);
			fflush(stdout);
			free(buffer);   
		} else {
			tfa_soft_probe_all(gCont);
		}

		return error; /* return here  gCont should not be freed */
	} else { /* Check Header and display other files then container */
		error = tfa98xx_header_check(fname, buf);
		if(error == Tfa98xx_Error_Ok)
			error = tfa98xx_show_headers(buf, type, buffer);
	}

	if(error == Tfa98xx_Error_Buffer_too_small)
		PRINT("Error: cnt file size exceeds maximum size of %d characters.\n", 50*1024);

	free(buf);
	return error;
}

/*
 * check CRC for file
 *   CRC is calculated over the bytes following the CRC field
 *
 *   return 0 on error
 *
 *   NOTE the container has another check function because the header differs
 */
int tfaContCrcCheck(nxpTfaHeader_t *hdr)
{
	uint8_t *base;
	int size;
	uint32_t crc;

	base = (uint8_t *)&hdr->CRC + 4; // ptr to bytes following the CRC field
	size = (int)(hdr->size - (base - (uint8_t *)hdr)); // nr of bytes following the CRC field
	if(size > 0) {
		crc = ~crc32_le(~0u, base, (size_t)size);
	}
	else {
		PRINT("Error: invalid size: %d \n", (int)size);
		return 0;
	}

	return crc != hdr->CRC;
}

enum Tfa98xx_Error tfa98xx_header_check(char *fname, nxpTfaHeader_t *buf)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	nxpTfaHeader_t existingHdr;
	int noHeader = 0;
	int size = 0;
	nxpTfa98xxParamsType_t paramsType;
	FILE *f;

	/*  Read the first x bytes to see if the file has a header */
	f=fopen(fname, "rb");
	if ( !f ) {
		ERRORMSG("Can't open %s (%s).\n", fname, strerror(errno));
		return Tfa98xx_Error_Bad_Parameter;
	}

	// read the potential header
	fread(&existingHdr , sizeof(existingHdr), 1, f);

	fclose(f);
	paramsType = cliParseFiletype(fname);

	switch ( paramsType ) {
	case tfa_speaker_params:
		if(!HeaderMatches(&existingHdr, speakerHdr))
			noHeader = 1;
		break;
	case tfa_patch_params:
		if(!HeaderMatches(&existingHdr, patchHdr))
			noHeader = 1;
		break;
	case tfa_drc_params:
		if(!HeaderMatches(&existingHdr, drcHdr))
			noHeader = 1;
		break;
	case tfa_preset_params:
		if(!HeaderMatches(&existingHdr, presetHdr))
			noHeader = 1;
		break;
	case tfa_equalizer_params:
		if(!HeaderMatches(&existingHdr, equalizerHdr))
			noHeader = 1;
		break;
	case tfa_vstep_params:
		if(!HeaderMatches(&existingHdr, volstepHdr))
			noHeader = 1;
		break;
        case tfa_msg_params:
		if(!HeaderMatches(&existingHdr, msgHdr))
			noHeader = 1;
		break;
	default:
		ERRORMSG("Unknown file type:%s \n", fname);
                PRINT("Please check the file type \n");
                error = Tfa98xx_Error_Bad_Parameter;
		break;
	}

        if(error != Tfa98xx_Error_Ok)
                return error;

	if(noHeader) {
		PRINT("File size: %d, does the file contain a header? \n", size);
		return Tfa98xx_Error_Bad_Parameter;
	}

	if(buf->size <= 0) {
		PRINT("\nWarning: Wrong header size: %d. Temporarily changed size to %d! \n\n", buf->size, size);
		buf->size = (uint16_t)size;
	}

	if (tfaContCrcCheck(buf)) {
		ERRORMSG("CRC error in %s\n", fname);
		tfaContShowFile(buf);
		error = Tfa98xx_Error_Bad_Parameter;
	}

        return error;
}

/*
 * volume step2 file (VP2)
 */
enum Tfa98xx_Error tfaContLoadVstepMax2( nxpTfaVolumeStepMax2File_t *vp, char *strings, int maxlength ) 
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
        int msgLength=0, i, j, k=0, size = 0, nrMessages;
        char str[NXPTFA_MAXLINE];
	uint16_t VstepDatasheetName[TFA_MAX_VSTEPS];
	uint16_t VstepDatasheetValue[TFA_MAX_VSTEPS];
	struct nxpTfaVolumeStepRegisterInfo *regInfo;
	struct nxpTfaVolumeStepMessageInfo *msgInfo;

	sprintf(str, "Vstep parameters\n");
        error = tfa_append_substring(str, strings, maxlength);
	sprintf(str, "\tversion:%d.%d.%d \n", vp->version[0], vp->version[1], vp->version[2]);
        error = tfa_append_substring(str, strings, maxlength);
	sprintf(str, "\tnumber of Vsteps:%d\n", vp->NrOfVsteps);
        error = tfa_append_substring(str, strings, maxlength);

        for(i=0; i<vp->NrOfVsteps; i++) {
		/* Copy buffer data to structs 
		 * @size is to step to the next volumestep
		 * @sizeof is needed to skip the registers (32bit because a register is name(16bit) + value(16bit))
		 */
	        regInfo = (struct nxpTfaVolumeStepRegisterInfo*)(vp->vstepsBin + size);
		msgInfo = (struct nxpTfaVolumeStepMessageInfo*)(vp->vstepsBin+
				(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)+size));

                sprintf(str, "-------------------------------------------------------\n");
                error = tfa_append_substring(str, strings, maxlength);
                sprintf(str, "Volumestep %d: \n", i);
		error = tfa_append_substring(str, strings, maxlength);
	        sprintf(str, "\t Number of registers/bitfields:%d\n", regInfo->NrOfRegisters);
		error = tfa_append_substring(str, strings, maxlength);

		/* The vstep contains a 16bit register and a 16bit value */
                for(j=0; j<regInfo->NrOfRegisters*2; j++) {
                        /* Byte swap the datasheetname */
                        VstepDatasheetName[k] = (regInfo->registerInfo[j]>>8) | (regInfo->registerInfo[j]<<8);
			j++;
                        VstepDatasheetValue[k] = regInfo->registerInfo[j]>>8;
                        /* this is only for tfa2 family ,so hardcode */
                        sprintf(str, "\t %s=%d (0x%x)\n", tfaContBfName((uint16_t)VstepDatasheetName[k], 0xff88),
                                                        VstepDatasheetValue[k], VstepDatasheetValue[k]);
			error = tfa_append_substring(str, strings, maxlength);
			k++;
                }

		nrMessages = msgInfo->NrOfMessages;

	        sprintf(str, "\t Number of messages:%d\n", nrMessages);
                error = tfa_append_substring(str, strings, maxlength);

	        for(j=0; j<nrMessages; j++) {
			msgInfo = (struct nxpTfaVolumeStepMessageInfo*)(vp->vstepsBin+
				(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)+size));

	                /* Get the message length */
	                msgLength = ((msgInfo->MessageLength.b[0] << 16) + (msgInfo->MessageLength.b[1] << 8) + msgInfo->MessageLength.b[2]);

                        sprintf(str, "\t Message %d: \n", j);
			error = tfa_append_substring(str, strings, maxlength);
	                sprintf(str, "\t   message type: %d \n", msgInfo->MessageType);
			error = tfa_append_substring(str, strings, maxlength);
                        
			if(msgInfo->MessageType == 3) {
				sprintf(str, "\t   message length: %d bytes \n", msgLength);
				error = tfa_append_substring(str, strings, maxlength);
				sprintf(str, "\t   Nr of filters: %d\n", msgInfo->CmdId[0]);
				error = tfa_append_substring(str, strings, maxlength);

				/* MessageLength is in bytes */
				size += sizeof(msgInfo->MessageType) + sizeof(msgInfo->MessageLength) + msgLength;
			} else {
				sprintf(str, "\t   message length: %d bytes \n", msgLength * 3);
				error = tfa_append_substring(str, strings, maxlength);
				sprintf(str, "\t   command id: [%02x %02x %02x]\n", msgInfo->CmdId[0], msgInfo->CmdId[1], msgInfo->CmdId[2]);
				error = tfa_append_substring(str, strings, maxlength);

				/* MessageLength is in words (3 bytes) */
				size += sizeof(msgInfo->MessageType) + sizeof(msgInfo->MessageLength) + sizeof(msgInfo->CmdId) + ((msgLength-1) * 3);
			}
	        }
		/* Add size to skip the registers and message info */
	        size += sizeof(regInfo->NrOfRegisters) + (regInfo->NrOfRegisters * sizeof(uint32_t)) + sizeof(msgInfo->NrOfMessages);
	}
	 
        return error;
}

int parse_vstep(nxpTfaVolumeStepMax2_1File_t *vp, int header) 
{
    int msgLength=0, j, size = 0, nrMessages;
	struct nxpTfaVolumeStepRegisterInfo *regInfo;
	struct nxpTfaVolumeStepMessageInfo *msgInfo;

	/* If there is a header we skip the first 36 bytes */
	if(!header)
		size=36;

	regInfo = (struct nxpTfaVolumeStepRegisterInfo*)(vp->vstepsBin+size);
	msgInfo = (struct nxpTfaVolumeStepMessageInfo*)((vp->vstepsBin+size)+
			(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)));

	nrMessages = msgInfo->NrOfMessages;

	for(j=0; j<nrMessages; j++) {
		msgInfo = (struct nxpTfaVolumeStepMessageInfo*)(vp->vstepsBin+
			(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)+size));

	        /* Get the message length */
	        msgLength = ((msgInfo->MessageLength.b[0] << 16) + (msgInfo->MessageLength.b[1] << 8) + msgInfo->MessageLength.b[2]);
                    
		/* We only need to find messagetype 3 */
		if(msgInfo->MessageType == 3) {
			return 1;
		} else {
			/* MessageLength is in words (3 bytes) */
			size += sizeof(msgInfo->MessageType) + sizeof(msgInfo->MessageLength) + sizeof(msgInfo->CmdId) + ((msgLength-1) * 3);
		}
	}
	 
	return 0;
}

enum Tfa98xx_Error tfa98xx_show_headers(nxpTfaHeader_t *buf, nxpTfaHeaderType_t type, char *buffer)
{
	enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
	int length = 0, i=1;
	nxpTfaVolumeStepMax2File_t *vp = NULL;
	char *temp_buffer=NULL;

	switch (type) {
	case volstepHdr:
		if ( tfa98xx_cnt_write_verbose ) {
			if(strncmp(buf->version, "3", 1) == 0) { /* Max2 */

				vp = (nxpTfaVolumeStepMax2File_t *) buf;
				tfaContShowHeader( &vp->hdr);
				do {
					if(i > 50) { /* 50x1024 chars/bytes is the maximum allowed buffer size */
						error = Tfa98xx_Error_Fail;
						break;
					}
					/* allocate per blocks of 1024 bytes/chars */
					temp_buffer = (char*) realloc(buffer, i*1024);
					if(temp_buffer == NULL) {
						if(buffer != NULL)
							free(buffer);
					} else {
						buffer = temp_buffer;
						memset(buffer, 0, i*1024); /* clear the entire buffer */
						error = tfaContLoadVstepMax2((nxpTfaVolumeStepMax2File_t *) buf, buffer, i*1024);
					}
					i++;
				} while(error == Tfa98xx_Error_Buffer_too_small);

				if(error == Tfa98xx_Error_Ok) {
					length = (int)(strlen(buffer));
					*(buffer+length) = '\0'; /* terminate */
					puts(buffer);
					fflush(stdout);
				}
				free(buffer);
			} else {
				tfaContShowVstep2((nxpTfaVolumeStep2File_t *) buf);
			}
		}
		break;
	case speakerHdr:
		if ( tfa98xx_cnt_write_verbose )
			tfaContShowSpeaker((nxpTfaSpeakerFile_t *) buf);
		break;
	case msgHdr:
		if ( tfa98xx_cnt_write_verbose )
			tfaContShowMsg((nxpTfaMsgFile_t *) buf);
		break;
	case equalizerHdr:
		if ( tfa98xx_cnt_write_verbose )
			tfaContShowEq((nxpTfaEqualizerFile_t *) buf);
		break;
	case drcHdr:
		if ( tfa98xx_cnt_write_verbose )
			tfaContShowDrc((nxpTfaDrc_t *) buf);
		break;
	default:
			if ( tfa98xx_cnt_write_verbose )
					tfaContShowFile(buf);
			break;
	}
	return error;
}

/*
 * display the contents of a DSP message
 *  - input 3 bytes buffer
 */
int tfaContShowDspMsg(uint8_t *msg) {
	struct msg {
		uint8_t mod;
		uint8_t m_id;
		uint8_t p_id;
	} *pmsg;

	pmsg = (struct msg*)msg;

	PRINT("Module: 0x%02x ,M_ID: 0x%02x, P_ID:: 0x%02x\n ", pmsg->mod, pmsg->m_id, pmsg->p_id);

	return 0;
}

void tfaContShowMsg(nxpTfaMsgFile_t *file) 
{
	int i;
	tfaContShowHeader( &file->hdr);

	PRINT("msg code: (lsb first)");
	for(i=0; i<3; i++) {
		PRINT("0x%02x ", file->data[i]);
	}
	PRINT("\n");
	tfaContShowDspMsg(file->data);
}

void tfaContShowEq(nxpTfaEqualizerFile_t *eqf) {
	int ind;
	unsigned int i;
	tfaContShowHeader( &eqf->hdr);

	for(ind=0;ind<TFA98XX_MAX_EQ;ind++) {
		PRINT("%d: ", ind);
		if (eqf->filter[ind].enabled ) {
			for(i=0;i<sizeof(nxpTfaBiquadOld_t);i++) {
				PRINT("0x%02x ", eqf->filter[ind].biquad.bytes[i]);
			}
		} else { 
			PRINT("disabled");
		}
		PRINT("\n");
	}
}

void tfaContShowDrc(nxpTfaDrc_t *file) 
{
	int i;
	tfaContShowHeader( &file->hdr);

	PRINT("msg code: (lsb first)");
	for(i=0; i<3; i++) {
		PRINT("0x%02x ", file->data[i]);
	}
	PRINT("\n");

	tfaContShowDspMsg(file->data);
}

/*
 * volume step2 file (VP2)
 */
void tfaContShowVstep2( nxpTfaVolumeStep2File_t *vp) {
	int step, i;
	tfaContShowHeader( &vp->hdr);

	PRINT("samplerate: %d\n", vp->samplerate);

	//  hexdump("eqf ",(const unsigned char * )&eqf->filter[0],sizeof(eqf->filter));
	for(step=0;step<vp->vsteps;step++) {   //TODO add rest of filter params
		PRINT("vstep[%d]: ", step);
		PRINT(" att:%.2f ", vp->vstep[step].attenuation);
		for(i=0;i<TFA98XX_MAX_EQ;i++)
			if (vp->vstep[step].filter[i].enabled) {
				break;
			}
		PRINT(", %s filters enabled", i==TFA98XX_MAX_EQ ? "no":"has");
		PRINT("\n");
	}
}

/*
 *
 */
static char *keyName[] = {
		"params", 	/* paramsHdr */
		"vstep", 	/* volstepHdr     */
		"patch", 	/* patchHdr     */
		"speaker", 	/* speakerHdr     */
		"preset", 	/* presetHdr   */
		"config", 	/* configHdr   */
		"eq",   	/* equalizerHdr */
		"msg",
		"drc", /* drcHdr  */
		"info"		/* Information file */
};
char *tfaContFileTypeName(nxpTfaFileDsc_t *file) {
	uint16_t type;
	nxpTfaHeader_t *hdr= (nxpTfaHeader_t *)file->data;

	type = hdr->id;

	switch (type) {
	case paramsHdr:
		return keyName[0];
		break;
	case volstepHdr:
		return keyName[1];
		break;
	case patchHdr:
		return keyName[2];
		break;
	case speakerHdr:
		return keyName[3];
		break;
	case presetHdr:
		return keyName[4];
		break;
	case configHdr:
		return keyName[5];
		break;
	case equalizerHdr:
		return keyName[6];
		break;
	case msgHdr:
		return keyName[7];
		break;
	case drcHdr:
		return keyName[8];
		break;
	case infoHdr:
		return keyName[9];
		break;
	default:
		ERRORMSG("File header has of unknown type: %x\n",type);
		return nostring;
	}
}

/* Compare string with profile 
 * Return 1 on success
 * Return 0 on fail
 */
int tfa_search_profile(nxpTfaDescPtr_t *dsc, char *profile_name)
{
	nxpTfaProfileList_t *prof;

	if(dsc->type == dscProfile) {
		prof = (nxpTfaProfileList_t *)(dsc->offset+(uint8_t *)gCont);

		/* Compare both strings */
		if(strcmp(tfaContGetString(&prof->name), profile_name) == 0)
			return 1;		
	}

	return 0;
}

enum Tfa98xx_Error tfaContShowItem(nxpTfaDescPtr_t *dsc, char *strings, int maxlength, int devidx)
{
	enum Tfa98xx_Error err;
	nxpTfaFileDsc_t *file;
	nxpTfaProfileList_t *prof;
	nxpTfaLiveDataList_t *lData;
	nxpTfaLiveData_t *liveD;
	nxpTfaRegpatch_t *reg;
	nxpTfaMode_t *cas;
	nxpTfaNoInit_t *noinit;
	nxpTfaFeatures_t *features;
	nxpTfaBitfield_t *bitF;
	nxpTfaMsg_t *msg;
	nxpTfaGroup_t *grp;
	nxpTfaContBiquad_t *bq;
	nxpTfaDspMem_t *mem;
	char str[NXPTFA_MAXLINE];
	char *str2,c='x';
	int i, idx, len, n;
	uint8_t *data;
	uint16_t size;
	uint32_t modifiers=0;

	/* family is needed in case of datasheet name table lookups */
	int devid = tfa_cnt_get_devid( tfa98xx_get_cnt(),devidx);

	switch (dsc->type) {
	case dscDefault:
		sprintf(str, "default:\n");
		break;
	case dscDevice: // device list
		sprintf(str, "device\n");
		break;
	case dscProfile:    // profile list
		prof = (nxpTfaProfileList_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "profile=%s\n",  tfaContGetString(&prof->name));
		break;
	case dscGroup:    // group list
		grp = (nxpTfaGroup_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "group=%s", tfaContProfileName(0,grp->profileId[0]));
		err = tfa_append_substring(str, strings, maxlength);
		for(i=1; i<grp->msg_size; i++) {
			sprintf(str, ", %s", tfaContProfileName(0,grp->profileId[i]));
			err = tfa_append_substring(str, strings, maxlength);
		}
		sprintf(str, "\n");
		break;
	case dscLiveData: //livedata list
		lData = (nxpTfaLiveDataList_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "livedata=%s\n",  tfaContGetString(&lData->name));
		break;
	case dscMode:
		cas = (nxpTfaMode_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "mode=%d\n", cas->value);
		break;
	case dscRegister:   // register patch
		reg = (nxpTfaRegpatch_t *)(dsc->offset+(uint8_t *)gCont);
		if(reg->mask == 0xffff)
			sprintf(str, "$0x%x=0x%02x\n", reg->address, reg->value);
		else
			sprintf(str, "$0x%x=0x%02x,0x%02x\n", reg->address, reg->value, reg->mask);
		break;
	case dscString: // ascii: zero terminated string
		sprintf(str, ";string: %s\n", tfaContGetString(dsc));
		break;
	case dscFile:       // filename + file contents
	case dscPatch:
		file = (nxpTfaFileDsc_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "%s=%s\n", tfaContFileTypeName(file), tfaContGetString(&file->name));
		break;
	case dscCfMem:
		mem = (nxpTfaDspMem_t *)(dsc->offset+(uint8_t *)gCont);
		if((mem->type == Tfa98xx_DMEM_XMEM) || (mem->type == Tfa98xx_DMEM_YMEM) || (mem->type == Tfa98xx_DMEM_IOMEM)) {
			str2 = str;
			switch (mem->type) {
			case Tfa98xx_DMEM_XMEM:
				c = 'x';
				break;
			case Tfa98xx_DMEM_YMEM:
				c= 'y';
				break;
			case Tfa98xx_DMEM_IOMEM:
				c = 'i';
				break;
			}
			n = sprintf(str2, "@%c[0x%x]=",c , mem->address);
			str2 += n;
			for(i=0;i<mem->size;i++) {
				n = sprintf(str2, "0x%x,", mem->words[i] & 0xffffff);
				str2 += n;
			}
			n = (int)strlen(str);
			str[n-1]= '\n'; /* removes last comma */
		} else {
			sprintf(str,"unsupported mem type:%d\n", mem->type);
		}
		break;
	case dscBitfield:   // Bitfield patch
		bitF = (nxpTfaBitfield_t *)(dsc->offset+(uint8_t *)gCont);

		/* Print the datasheet name and the bitname */
		if(strcmp(tfaContBfName(bitF->field, (unsigned short)devid), "Unknown bitfield enum") != 0) {
			sprintf(str, "%s=%d    \t ; %s \n", tfaContBfName(bitF->field, (unsigned short)devid), 
					bitF->value, tfaContBitName(bitF->field, (unsigned short)devid));
		} else {
			/* No datasheet name was found, so only print the bitname */
			sprintf(str, "%s=%d \n", tfaContBitName(bitF->field, (unsigned short)devid), bitF->value);
		}
		break;
	case dscFilter:
		bq = (nxpTfaContBiquad_t *)(dsc->offset+(uint8_t *)gCont);

		if (bq->aa.index > 100) {
			sprintf(str, "filter[%d,S]=", bq->aa.index - 100);
		} else if (bq->aa.index > 50) {
			sprintf(str, "filter[%d,P]=", bq->aa.index - 50);
		} else {
			sprintf(str, "filter[%d]=", bq->aa.index);
		}
		err = tfa_append_substring(str, strings, maxlength);

		if (bq->aa.index==13 || bq->aa.index==63 || bq->aa.index==113) {
			sprintf(str, "%s,%.0f,%.2f\n", tfa_cont_eqtype_name(bq->in.type),
						bq->in.cutOffFreq, bq->in.leakage);
		} else if ( (bq->aa.index==10) || (bq->aa.index==60) || (bq->aa.index==110) ||
			    (bq->aa.index==11) || (bq->aa.index==61) || (bq->aa.index==111) ||
			    (bq->aa.index==12) || (bq->aa.index==62) || (bq->aa.index==112) ) {
			sprintf(str, "%s,%.0f,%.1f,%.1f\n", tfa_cont_eqtype_name(bq->aa.type),
					bq->aa.cutOffFreq, bq->aa.rippleDb, bq->aa.rolloff);
		} else {
			sprintf(str,"unsupported filter index \n");
		}
		break;
	case dscNoInit:
		noinit = (nxpTfaNoInit_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str,"noinit=%d\n", noinit->value);
		break;
	case dscFeatures:
		features = (nxpTfaFeatures_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str,"features=0x%02x,0x%02x,0x%02x\n", features->value[0], features->value[1], features->value[2]);
		break;
	case dscLiveDataString:
		liveD = (nxpTfaLiveData_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "%s=%s, 0x%02x , 0x%x \n", liveD->name, liveD->addrs, liveD->tracker, liveD->scalefactor);
		break;
	case dscSetInputSelect:
	case dscSetOutputSelect:
	case dscSetProgramConfig:
	case dscSetLagW:
	case dscSetGains:
	case dscSetvBatFactors:
	case dscSetSensesCal:
	case dscSetSensesDelay:
	case dscSetMBDrc:
		msg = (nxpTfaMsg_t *)(dsc->offset+(uint8_t *)gCont);
		sprintf(str, "%s=0x%02x%02x%02x", tfaContGetCommandString(dsc->type), msg->cmdId[2], msg->cmdId[1], msg->cmdId[0]);
		err = tfa_append_substring(str, strings, maxlength);

		for(i=0; i<msg->msg_size; i++) {
			sprintf(str, ", 0x%06x", msg->data[i]);
			err = tfa_append_substring(str, strings, maxlength);
		}
		sprintf(str, "\n");
		break;
	case dscCmd:
	    data = dsc->offset+(uint8_t *)gCont;
	    size = *((uint16_t *)data);
	    idx = 2;

	    i = 0;
	    while (cmds[i].name != NULL) {
	        uint32_t value = (data[idx+0] << 16) + (data[idx+1] << 8) + data[idx+2];
	        if (cmds[i].id == (value & ~CMD_ID_CONFIG_MASK)) {
	        	modifiers = value & CMD_ID_CONFIG_MASK;
	        	break;
	        }
	        i++;
	    }
	    if (cmds[i].name == NULL) {
	        int value = (data[idx+0] << 16) + (data[idx+1] << 8) + data[idx+2];
	        PRINT("cmd: Unsupported command: 0x%x\n", value);
	        return -1;
	    }

	    len = sprintf(str, "cmd=%s", cmds[i].name);
	    if (modifiers) {
	        if (modifiers & CMD_ID_DISABLE_COMMON)
	        	len += sprintf(str+len, "_DC");
	        if (modifiers & CMD_ID_DISABLE_PRIMARY)
	        	len += sprintf(str+len, "_DP");
	        if (modifiers & CMD_ID_DISABLE_SECONDARY)
	        	len += sprintf(str+len, "_DS");
	        if (modifiers & CMD_ID_SAME_CONFIG)
	        	len += sprintf(str+len, "_SC");
	    }

	    idx += 3;

	    for (i=0; i<((size-3)/3); i++) {
	        int value = (data[idx+0] << 16) + (data[idx+1] << 8) + data[idx+2];
	        idx += 3;
		    len += sprintf(str+len, ",0x%.6x", value);
	    }
		len += sprintf(str+len, "\n");

		break;
	}

	err = tfa_append_substring(str, strings, maxlength);
	return err;
}

int tfaRunBitfieldDump(Tfa98xx_handle_t handle, unsigned char reg) 
{
	nxpTfaBitfield_t bf;
	char *bfName;
	int idx;
	unsigned int tmpConversion;
	unsigned int matches = 0;
	unsigned short rev = tfa98xx_get_device_revision(handle);

	union {
		uint16_t field;
		nxpTfaBfEnum_t bfSpecifier;
	} bfUnion;

	bfUnion.bfSpecifier.address = reg;

	/* Scan for matches */
	for(idx=0; idx<=15; idx++) {
		bfUnion.bfSpecifier.pos = idx;
		bfName = tfaContDsName(bfUnion.field, rev);

		if( strcmp(bfName, "Unknown bitfield enum") != 0 ) {
			matches |= (1 << idx); /* Flag which indexes have a match */

			bf.field = tfaContBfEnum(bfName, rev);
			tfaRunReadBitfield(handle, &bf);

			bfUnion.field = bf.field;
			tmpConversion = bfUnion.bfSpecifier.len;
			idx += (int)tmpConversion;
		}
	}

	/* If no datasheet names are found return error */
	if(matches == 0) {
		PRINT("\n");
		return 1;
	}

	/* Print from MSB to LSB */
	for(idx=15; idx>=0; idx--) {
		/* Check if match was found on idx */
		if((matches >> idx) & 1) { 
			bfUnion.bfSpecifier.pos = idx;
			
			bfName = tfaContDsName(bfUnion.field, rev);
			bf.field = tfaContBfEnum(bfName, rev);
			tfaRunReadBitfield(handle, &bf);

			PRINT("%s:%d ", bfName, bf.value);
		}
	}
	PRINT("\n");
	return 0;
}

int access_bf_without_cnt(int slave_address, unsigned short rev_id) {
        return ((slave_address >=0) && (rev_id != 0xffff)) ? 1 : 0;
}

enum Tfa98xx_Error get_devcount(int slave_address, unsigned short rev_id, int *devcount) {
        *devcount = tfa98xx_cnt_max_device();
        if((*devcount <= 0) && (!access_bf_without_cnt(slave_address, rev_id))) {
		PRINT_ERROR("No or wrong container file loaded\n");
		return  Tfa98xx_Error_Bad_Parameter;
        }
        return Tfa98xx_Error_Ok;
}

#define FIELD_LENGTH 50
void print_rw_bitfield(nxpTfaBitfield_t bf, int slave_address, unsigned short rev_id, int dev_idx, int write) {
        char devicePrintString[FIELD_LENGTH], fieldPrintString[FIELD_LENGTH];

        if(strcmp(tfaContBfName(bf.field, rev_id), "Unknown bitfield enum") != 0)
                snprintf(fieldPrintString, FIELD_LENGTH, "%s", tfaContBfName(bf.field, rev_id));
        else
                snprintf(fieldPrintString, FIELD_LENGTH, "%s", tfaContBitName(bf.field, rev_id));

        if( access_bf_without_cnt(slave_address, rev_id) )
                snprintf(devicePrintString, FIELD_LENGTH, "0x%02x", slave_address);
        else
                snprintf(devicePrintString, FIELD_LENGTH, "%s", tfaContDeviceName(dev_idx));          

        if(write)       /* write: */
                PRINT("[%s] %s < 0x%x \n", devicePrintString, fieldPrintString, bf.value);
        else            /* read: */
                PRINT("[%s] %s:0x%x \n", devicePrintString, fieldPrintString, bf.value);                
}

enum Tfa98xx_Error tfa_rw_bitfield_per_device(const char *bf_name, uint16_t bf_value, int dev_idx, 
                                                int slave_address, unsigned short rev_id, int write) {
        nxpTfaBitfield_t bf;
        enum Tfa98xx_Error err = tfaContOpen(dev_idx);
        if(err != Tfa98xx_Error_Ok)
		return err;
		
        if(rev_id == 0xffff)
	        rev_id = tfa98xx_dev_revision(dev_idx);

        if( access_bf_without_cnt(slave_address, rev_id) )
	        tfa_mock_probe(dev_idx, rev_id, slave_address);
		
        bf.value = bf_value;
        bf.field = tfaContBfEnum(bf_name, rev_id);
        if(bf.field != 0xffff) {
	        if(write)       /* write: */
		        err = tfaRunWriteBitfield(dev_idx, bf);
	        else            /* read: */
		        err = tfaRunReadBitfield(dev_idx, &bf);

			print_rw_bitfield(bf, slave_address, rev_id, dev_idx, write);
        } else {
			PRINT("Error: Unknown name: %s \n", bf_name);
        }

        tfaContClose(dev_idx);
        return err;
}

enum Tfa98xx_Error tfa_rw_bitfield(const char *bf_name, uint16_t bf_value, int slave_address, int tfa_arg, int write) {
        enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
        int dev_idx, devcount, selected_idx = 0;
        unsigned short rev_id = (unsigned short)tfa_arg;

        err = get_devcount(slave_address, rev_id, &devcount);
        if(err != Tfa98xx_Error_Ok)
                return err;
				
        if(slave_address >= 0) {
			selected_idx = ( slave_address == 0x36 ) ? 0 : 1; /* Determine which index to use. */
			selected_idx = ( slave_address == 0x34 ) ? 0 : selected_idx; /* Specific for max2. */
			devcount = selected_idx+1; /* Make sure loop runs once. */
		}

        for(dev_idx = selected_idx; dev_idx < devcount; dev_idx++) {
        	err = tfa_rw_bitfield_per_device(bf_name, bf_value, dev_idx, slave_address, rev_id, write);
        }

        return err;
}

enum Tfa98xx_Error tfa_check_stringlength(int substring_len, int str_buff_len, int maxlength)
{
	if( (substring_len+str_buff_len) >= maxlength ) {
		return Tfa98xx_Error_Buffer_too_small; // max length too small
	}
	return Tfa98xx_Error_Ok;
}

enum Tfa98xx_Error tfa_append_substring(char *substring, char *str_buff, int maxlength) 
{
	int substring_len = (int)(strlen(substring));
        int str_buff_len = (int)(strlen(str_buff));

	if( tfa_check_stringlength(substring_len, str_buff_len, maxlength) != Tfa98xx_Error_Ok )
		return Tfa98xx_Error_Buffer_too_small;

	strcat(str_buff, substring);

	return Tfa98xx_Error_Ok;
}

/*
 * write a parameter file to the device
 *   a filedescriptor needs to be created first to call tfaContWriteFile()
 */
enum Tfa98xx_Error tfaContWriteFileByname(int device,  char *fname, int VstepIndex, int VstepMsgIndex) {
	nxpTfaFileDsc_t *file;
	void *fbuf;
	int size;
	enum Tfa98xx_Error err;

	size = tfaReadFile(fname, &fbuf);
	if (!size) {
		free(fbuf);
		return Tfa98xx_Error_Bad_Parameter;
	}

	file = malloc(size+(sizeof(nxpTfaFileDsc_t)));

	if ( !file ) {
		ERRORMSG("Can't allocate %d bytes.\n", size);
		free(fbuf);
		return Tfa98xx_Error_Other;
	}
	memcpy(file->data, fbuf, size);
	file->size = size;
	err = tfaContWriteFile(device, file, VstepIndex, VstepMsgIndex);
	free(fbuf);
	free(file);

	return err;
}

enum Tfa98xx_Error getProfileInfo(char* profile_arg, int *profile)
{
	int i;

	/* Is the input a integer? */
	if(sscanf(profile_arg, "%d", profile) == 0) {
		/* Compare the input with available profiles */
		for(i=0;i<tfaContMaxProfile(0);i++) {
			if(strcmp(tfaContProfileName(0,i), profile_arg)==0) {
				*profile = i;
				break;
			} else {
				*profile = -2;
			}
		}
	}

	/* if profile is -1 the info is already printed! */
	if (*profile == -1) {
		return Tfa98xx_Error_Ok;
	}
		
	for (i=0; i < tfa98xx_cnt_max_device(); i++ ) {
		if(*profile == -2) {
			PRINT("Incorrect profile name given. (%s)\n", profile_arg);
			return Tfa98xx_Error_Bad_Parameter;
		}else if ( *profile < 0 || *profile >= tfaContMaxProfile(i) ) {
			PRINT("Incorrect profile given. The value must be [0 - %d].\n", (tfaContMaxProfile(i)-1));
			return Tfa98xx_Error_Bad_Parameter;
		}
	}

	return Tfa98xx_Error_Ok;
}

#define WORD16(MSB, LSB) ((uint16_t)( ((uint8_t)MSB)<<8 | (uint8_t)LSB))
int multi_message_extract(char *fname)
{
	uint16_t msg_length, done_length=0, total_length;
	uint8_t *inbuf, *msg;
	int size, msg_count=0;
	FILE *newFile;
	char filename[100];

	size = tfaReadFile(fname, (void**) &inbuf); /* mallocs file buffer */
	if (size == 0) {
		free(inbuf);
		return Tfa98xx_Error_Bad_Parameter; /* tfaReadFile reported error already */
	}

	if(inbuf[0] != 'm' || inbuf[1] != 'm') {
		printf("%s: This function is only for multi messages \n", __FUNCTION__);
		free(inbuf);
		return Tfa98xx_Error_Bad_Parameter;
	}

	printf("Extracting messages from %s \n\n", fname);

	/* Bit 2 and 3 are the total length of the multi-msg file */
	total_length = WORD16(inbuf[2], inbuf[3]);
	if(!total_length) {
		printf("%s: multi-message length is incorrect:%d\n", __FUNCTION__, total_length);
		free(inbuf);
		return Tfa98xx_Error_Bad_Parameter;
	}

	if (tfa98xx_cnt_write_verbose)
		printf("Total file length = %d \n", total_length);

	/* Skip the first 4 bytes (2 bytes mm, 2 bytes total length) */
	msg = inbuf+4;
	done_length = 0;

	do {
		/* Every 'new' message starts with the msg length */
		msg_length = WORD16(msg[0], msg[1]);
		msg+=2;

		if(msg_length > 0) {
			msg_count++;
			sprintf(filename, "multi-msg_extract_file_%d.bin", msg_count);
			newFile = fopen(filename, "w+b");
			fwrite(msg, msg_length, 1, newFile);
			fclose(newFile);
			PRINT("Created %s of %d bytes \n", filename, msg_length);
		} else {
			PRINT("Error: Impossible msg length of %d bytes found \n", msg_length);
			free(inbuf);
			return Tfa98xx_Error_Bad_Parameter;
		}

		msg += msg_length;
		done_length += msg_length+2;
	} while(done_length < total_length);

	free(inbuf);
	return 0;
}
nxp_vstep_msg_t *tfa_vstep_info(int dev_idx){
	nxpTfaDeviceList_t *dev = tfaContDevice(dev_idx);
	int i,j,size=0,nrMessages,msgLength=0;
	nxpTfaFileDsc_t *file;
	nxp_vstep_msg_t *vstep_msg_info=NULL;
	nxpTfaHeader_t *hdr;
	nxpTfaVolumeStepMax2File_t *vp;
	nxpTfaHeaderType_t type;
	struct nxpTfaVolumeStepRegisterInfo *regInfo = {0};
	struct nxpTfaVolumeStepMessageInfo *msgInfo = {0};
	static nxpTfaContainer_t *g_cont = NULL;
	g_cont = tfa98xx_get_cnt(); /* container file */
	//struct nxpTfaVolumeStepMessageInfo *vstep_msgs = {0};
	
	for(i=0;i<dev->length;i++) {
		if ( dev->list[i].type == dscFile ) {
			file = (nxpTfaFileDsc_t *)(dev->list[i].offset+(uint8_t *)g_cont);
			hdr = (nxpTfaHeader_t *)file->data;
			vp = (nxpTfaVolumeStepMax2File_t *)hdr;
			type = (nxpTfaHeaderType_t) hdr->id;
			switch (type){
				case volstepHdr:
					vstep_msg_info = (nxp_vstep_msg_t *)malloc(sizeof(nxp_vstep_msg_t));
					vstep_msg_info->fw_version = (((int )vp->version[0]&0xff)<<16) + (((int )vp->version[1]&0xff)<<8) + (((int)vp->version[2]&0x03)<<6);
					vstep_msg_info->no_of_vsteps = vp->NrOfVsteps;
					regInfo = (struct nxpTfaVolumeStepRegisterInfo*)(vp->vstepsBin + size);
					vstep_msg_info->reg_no = regInfo->NrOfRegisters; 
					vstep_msg_info->msg_reg = (unsigned char *)malloc(4*regInfo->NrOfRegisters*sizeof(unsigned char));
					memcpy(vstep_msg_info->msg_reg, regInfo->registerInfo, 4*regInfo->NrOfRegisters*sizeof(unsigned char));
					msgInfo = (struct nxpTfaVolumeStepMessageInfo*)(vp->vstepsBin+(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)+size));
					nrMessages = msgInfo->NrOfMessages;
					//vstep_msgs = (struct nxpTfaVolumeStepMessageInfo*) malloc (nrMessages*sizeof(msgInfo));
					for(j=0; j<nrMessages; j++) {
						/* location of message j, from vstep i */
						msgInfo = (struct nxpTfaVolumeStepMessageInfo*)(vp->vstepsBin+
									(regInfo->NrOfRegisters * sizeof(uint32_t)+sizeof(regInfo->NrOfRegisters)+size));
						/* message length */
						msgLength = ( (msgInfo->MessageLength.b[0] << 16) + (msgInfo->MessageLength.b[1] << 8) + msgInfo->MessageLength.b[2]);
						if((msgInfo->CmdId[0]==0x04) && (msgInfo->CmdId[1]==0x81) && (msgInfo->CmdId[2]==0x00)){
							/*removing command id which is is 1 word long*/
							vstep_msg_info->algo_param_length = (msgLength-1)*3;
							vstep_msg_info->msg_algo_param = (unsigned char *)malloc((msgLength-1)*3*sizeof(unsigned char));
							memcpy((unsigned char *)vstep_msg_info->msg_algo_param, (unsigned char *)msgInfo->ParameterData+1, (msgLength-1)*3*sizeof(unsigned char));
						}
						if((msgInfo->CmdId[0]==0x00) && (msgInfo->CmdId[1]==0x82) && (msgInfo->CmdId[2]==0x00)){
							/*removing command id which is is 1 word long*/
							vstep_msg_info->filter_coef_length = (msgLength-2)*3;
							vstep_msg_info->msg_filter_coef = (unsigned char *)malloc((msgLength-1)*3*sizeof(unsigned char));
							memcpy((unsigned char *)vstep_msg_info->msg_filter_coef, (unsigned char *)msgInfo->ParameterData+1, (msgLength-1)*3*sizeof(unsigned char));
						}
						if((msgInfo->CmdId[0]==0x00) && (msgInfo->CmdId[1]==0x81) && (msgInfo->CmdId[2]==0x07)){
							/*removing command id which is is 1 word long*/
							vstep_msg_info->mbdrc_length = (msgLength-1)*3;
							vstep_msg_info->msg_mbdrc = (unsigned char *)malloc((msgLength-1)*3*sizeof(unsigned char));
							memcpy((unsigned char *)vstep_msg_info->msg_mbdrc, (unsigned char *)msgInfo->ParameterData+1, (msgLength-1)*3*sizeof(unsigned char));

						}
						/*memcpy((struct nxpTfaVolumeStepMessageInfo*)(vstep_msgs+msgLength), msgInfo, sizeof(msgInfo));*/
						if(msgInfo->MessageType == 3) {
						/* MessageLength is in bytes */
							size += sizeof(msgInfo->MessageType) + sizeof(msgInfo->MessageLength) + msgLength;
						} else {
						/* MessageLength is in words (3 bytes) */
							size += sizeof(msgInfo->MessageType) + sizeof(msgInfo->MessageLength) + sizeof(msgInfo->CmdId) + ((msgLength-1) * 3);
						}
					}
					break;
				default:
					break;
			}
		}
	}
	return vstep_msg_info;
}
int tfa_speaker_info(int dev_idx){
	nxpTfaDeviceList_t *dev = tfaContDevice(dev_idx);
	int i,size=0;
	nxpTfaFileDsc_t *file;
	nxpTfaHeader_t *hdr;
	nxpTfaHeaderType_t type;
	static nxpTfaContainer_t *g_cont = NULL;
	g_cont = tfa98xx_get_cnt(); /* container file */
	//struct nxpTfaVolumeStepMessageInfo *vstep_msgs = {0};
	
	for(i=0;i<dev->length;i++) {
		if ( dev->list[i].type == dscFile ) {
			file = (nxpTfaFileDsc_t *)(dev->list[i].offset+(uint8_t *)g_cont);
			hdr = (nxpTfaHeader_t *)file->data;
			type = (nxpTfaHeaderType_t) hdr->id;
			switch (type){
				case speakerHdr:
					size = hdr->size - sizeof(struct nxpTfaSpkHeader) - sizeof(struct nxpTfaFWVer);
					/*size includes command id too. removing it*/
					size--;
					break;
				default:
					break;
			}
		}
	}
	return size;
}
