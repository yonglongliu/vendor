/*
 * tfaContainerWrite.h
 *
 *  Created on: Feb 20, 2015
 *      Author: wim
 */

#ifndef SRV_INC_TFACONTAINERWRITE_H_
#define SRV_INC_TFACONTAINERWRITE_H_

#include "tfa_container.h"
#include "tfaContUtil.h"

typedef struct nxpTfaLocationInifile{
	char locationIni[FILENAME_MAX];
} nxpTfaLocationInifile_t;

void free_gCont();

nxpTfa98xxParamsType_t cliParseFiletype(char *filename);
/*
 * loads the container file
 */
enum Tfa98xx_Error tfa98xx_cnt_loadfile(char *fname, int cnt_verbose);
/*
 * Get customer header information
 */
void tfaContGetHdr(char *inputname, struct nxpTfaHeader *hdrbuffer);
/*
 * Specify the device type for service layer support
 */
void tfa_cont_dev_type(int type);

/*
 * Get customer header speaker file information
 */
void tfa_cnt_get_spk_hdr(char *inputname, struct nxpTfaSpkHeader *hdrbuffer);
void tfa_cont_write_verbose(int verbose);
/* display */
void tfaContShowSpeaker(nxpTfaSpeakerFile_t *spk);
void tfaContShowEq(nxpTfaEqualizerFile_t *eqf);
void tfaContShowDrc(nxpTfaDrc_t *file);
void tfaContShowMsg(nxpTfaMsgFile_t *file);
void tfaContShowVstep2( nxpTfaVolumeStep2File_t *vp);
void tfaContShowFile(nxpTfaHeader_t *hdr);
char *tfaContFileTypeName(nxpTfaFileDsc_t *file);

/**
 * read or write current bitfield value
 * @param bf_name the string of the bitfield name
 * @param bf_value the value that the bitfield should be set to (if write > 0)
 * @param slave_arg the slave address or device index to write bf, 
 *	slave_arg = -1 indicates write bf on all available slaves.
 * @param tfa_arg the device type, -1 if not specified
 * @param write indicates wether to write or read (write > 0 means write)
 * @return Tfa98xx_Error
 */
enum Tfa98xx_Error tfa_rw_bitfield(const char *bf_name, uint16_t bf_value, int slave_arg, int tfa_arg, int write);

/*
 * show the contents of the local container
 */
enum Tfa98xx_Error  tfaContShowContainer(char *strings, int maxlength);
int tfa_search_profile(nxpTfaDescPtr_t *dsc, char *profile_name);
enum Tfa98xx_Error  tfaContShowItem(nxpTfaDescPtr_t *dsc, char *strings, int maxlength, int devidx);
enum Tfa98xx_Error  tfaContShowDevice(int idx, char *strings, int maxlength);
enum Tfa98xx_Error  tfa98xx_header_check(char *fname, nxpTfaHeader_t *buf);
enum Tfa98xx_Error  tfa98xx_show_headers(nxpTfaHeader_t *buf, nxpTfaHeaderType_t type, char *buffer);
int parse_vstep(nxpTfaVolumeStepMax2_1File_t *vp, int header);

/**
 * Append a substring to a char buffer.
 * Make sure it doesn't write outside the buffer with maxlength
 * Keep track of an offset that indicates from what index to append
 * @param substring string to append to buffer
 * @param str_buff string buffer that gets appended
 * @param maxlength max length of the string buffer
 * @return Tfa98xx_Error
 */
enum Tfa98xx_Error tfa_append_substring(char *substring, char *str_buff, int maxlength);

int tfaContCrcCheck(nxpTfaHeader_t *hdr) ;

/**
 * Split the (binary) container file into human readable ini and individual files
 * @return 0 if all files are generated
 */
int tfa98xx_cnt_split(char *fileName, char *directoryName);

/* write functions */
int tfaContSave(nxpTfaHeader_t *hdr, char *filename);
int tfaContBin2Hdr(char *iniFile, int argc, char *argv[]);
int tfaContIni2Container(char *iniFile);

/*
 * create the containerfile as descibred by the input 'ini' file
 * return the size of the new file
 */
int tfaContParseIni(char *iniFile, char *outFileName, nxpTfaLocationInifile_t *loc);

int tfaContCreateContainer(nxpTfaContainer_t *contIn, char *outFileName, nxpTfaLocationInifile_t *loc);
nxpTfaProfileList_t *tfaContFindProfile(nxpTfaContainer_t *cont,const char *name);
nxpTfaLiveDataList_t *tfaContFindLiveData(nxpTfaContainer_t *cont,const char *name);
nxpTfaProfileList_t *tfaContGet1stProfList(nxpTfaContainer_t *cont);
nxpTfaLiveDataList_t *tfaContGet1stLiveDataList(nxpTfaContainer_t *cont);
nxpTfaProfileList_t *tfaContGetDevProfList(nxpTfaContainer_t *cont,int devIdx,int profIdx);
nxpTfaDescPtr_t *tfaContSetOffset(nxpTfaDescPtr_t *dsc,int idx);
char *tfaContGetString(nxpTfaDescPtr_t *dsc);
nxpTfaDeviceList_t *tfaContGetDevList(nxpTfaContainer_t *cont,int idx);
nxpTfaDescriptorType_t parseKeyType(char *key);

/*
 * write a parameter file to the device
 */
enum Tfa98xx_Error tfaContWriteFileByname(int device,  char *fname, int VstepIndex, int VstepMsgIndex);

/**
 * print all the bitfields of the register
 * @param handle slave handle
 * @param reg address
 * @return 0 if at least 1 name was found
 */
int tfaRunBitfieldDump(Tfa98xx_handle_t handle, unsigned char reg);

/**
 * set the dev revid in the module global tfa_dev_type[]
 */
int tfa_cont_set_patch_dev(char *configpath, char *patchfile, int devidx);

/*
 * Get the information of the profile
 */
enum Tfa98xx_Error getProfileInfo(char *profile_arg, int *profile);

void filter_coeff_anti_alias(nxpTfaAntiAliasFilter_t *self, int tfa98xx_family_type);
void filter_coeff_equalizer(nxpTfaEqFilter_t *self, int tfa98xx_family_type);
void filter_coeff_integrator(nxpTfaIntegratorFilter_t *self, int tfa98xx_family_type);

const char *tfa_cont_eqtype_name(enum nxpTfaFilterType t);

/**
 * Get number of matches in the header data
 *
 * @param hdr pointer to file header data
 * @param t typenumber for tfaheader
 * @return number of matches
 */
int HeaderMatches (nxpTfaHeader_t *hdr, nxpTfaHeaderType_t t);

int multi_message_extract(char *fname);
/**
 * get information regarding vtsep file. 
 *
 * @param device id
 * @return nxp_vstep_msg_t
 */
nxp_vstep_msg_t *tfa_vstep_info(int dev_idx);
/**
 * get information regarding speaker file. 
 *
 * @param device id
 * @return length of speaker parameter
 */
int tfa_speaker_info(int dev_idx);

#endif /* SRV_INC_TFACONTAINERWRITE_H_ */
