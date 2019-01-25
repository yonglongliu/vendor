#include <assert.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include "Tfa98API.h"

#include "tfa.h"
#include "tfa_dsp_fw.h"
#include "tfa_service.h"

/**
 * Return the maximum nr of devices (SC39786)
 */
int Tfa98xx_MaxDevices(void)
{
	return tfa98xx_max_devices();
}

Tfa98xx_Error_t
Tfa98xx_Open(unsigned char slave_address, Tfa98xx_handle_t *pHandle)
{
	Tfa98xx_Error_t err = tfa_probe(slave_address, pHandle);
	if (err != Tfa98xx_Error_Ok)
		return err;
	return tfa98xx_open(*pHandle);
}

Tfa98xx_Error_t Tfa98xx_Close(Tfa98xx_handle_t handle)
{
	return tfa98xx_close(handle);
}


/* Tfa98xx_DspReset
 *  set or clear DSP reset signal
 */
 Tfa98xx_Error_t Tfa98xx_DspReset(Tfa98xx_handle_t handle, int state)
{
	return tfa98xx_dsp_reset(handle, state);
}

/* Tfa98xx_DspSystemStable
 *  return: *ready = 1 when clocks are stable to allow DSP subsystem access
 */
Tfa98xx_Error_t Tfa98xx_DspSystemStable(Tfa98xx_handle_t handle, int *ready)
{
	int dspstatus;
	enum Tfa98xx_Error error;

	error = tfa98xx_dsp_system_stable(handle, &dspstatus);

	*ready = dspstatus;
	return error;
}

Tfa98xx_Error_t Tfa98xx_Init(Tfa98xx_handle_t handle)
{
	return tfa98xx_init(handle);
}

/*
 * write a 16 bit subaddress
 */
Tfa98xx_Error_t
Tfa98xx_WriteRegister16(Tfa98xx_handle_t handle,
			unsigned char subaddress, unsigned short value)
{
	return reg_write(handle, subaddress, value);
}

Tfa98xx_Error_t
Tfa98xx_ReadRegister16(Tfa98xx_handle_t handle,
		       unsigned char subaddress, unsigned short *pValue)
{
	return reg_read(handle, subaddress, pValue);
}

Tfa98xx_Error_t
Tfa98xx_ReadData(Tfa98xx_handle_t handle, 
	unsigned char subaddress, int num_bytes, unsigned char data[])
{
	return tfa98xx_read_data(handle, subaddress, num_bytes, data);
}

Tfa98xx_Error_t
Tfa98xx_DspWriteSpeakerParameters(Tfa98xx_handle_t handle,
				  int length,
				  const unsigned char *pSpeakerBytes)
{

	return tfa98xx_dsp_write_speaker_parameters(handle,
				  length, pSpeakerBytes);
}

Tfa98xx_Error_t
Tfa98xx_DspWritePreset(Tfa98xx_handle_t handle, int length,
		       const unsigned char *pPresetBytes)
{
	return tfa98xx_dsp_write_preset(handle, length, pPresetBytes);
}

Tfa98xx_Error_t Tfa98xx_SetVolumeLevel(Tfa98xx_handle_t handle, unsigned short vollevel)
{
	return tfa98xx_set_volume_level(handle, vollevel);
}

Tfa98xx_Error_t Tfa98xx_SetMute(Tfa98xx_handle_t handle, Tfa98xx_Mute_t mute)
{
	return tfa98xx_set_mute(handle, mute);
}

void Tfa98xx_ConvertData2Bytes(int num_data, const int data[],
			       unsigned char bytes[])
{
	tfa98xx_convert_data2bytes(num_data, data,
			       bytes);
				   
}

Tfa98xx_Error_t Tfa98xx_DspSupportDrc(Tfa98xx_handle_t handle,
				      int *pbSupportDrc)
{
	return tfa98xx_dsp_support_drc(handle, pbSupportDrc);
}

#define PATCH_HEADER_LENGTH 6
Tfa98xx_Error_t
Tfa98xx_DspPatch(Tfa98xx_handle_t handle, int patchLength,
		 const unsigned char *patchBytes)
{
	return tfa_dsp_patch(handle, patchLength, patchBytes);
}

Tfa98xx_Error_t
Tfa98xx_DspReadMem(Tfa98xx_handle_t handle,
		   unsigned short start_offset, int num_words, int *pValues)
{
	return mem_read(handle, start_offset, num_words, pValues);
}

Tfa98xx_Error_t
Tfa98xx_DspWriteMem(Tfa98xx_handle_t handle, unsigned short address, int value, int memtype)
{
	return mem_write(handle, address, value, memtype);
}

const char* Tfa98xx_GetErrorString(Tfa98xx_Error_t error)
{
        const char* pErrStr;

        switch (error)
        {
                case Tfa98xx_Error_Ok:
                        pErrStr = "Ok";
                        break;
                case Tfa98xx_Error_Fail:
			pErrStr = "generic_failure";
			break;
		case Tfa98xx_Error_NoClock:
			pErrStr = "No_I2S_Clock";
			break;
		case Tfa98xx_Error_AmpOn:
			pErrStr = "amp_still_running";
			break;
                case Tfa98xx_Error_DSP_not_running:
                        pErrStr = "DSP_not_running";
                        break;
                case Tfa98xx_Error_Bad_Parameter:
                        pErrStr = "Bad_Parameter";
                        break;
                case Tfa98xx_Error_NotOpen:
                        pErrStr = "NotOpen";
                        break;
                case Tfa98xx_Error_InUse:
                        pErrStr = "InUse";
                        break;
                case Tfa98xx_Error_RpcBusy:
                        pErrStr = "RpcBusy";
                        break;
                case Tfa98xx_Error_RpcModId:
                        pErrStr = "RpcModId";
                        break;
                case Tfa98xx_Error_RpcParamId:
                        pErrStr = "RpcParamId";
                        break;
                case Tfa98xx_Error_RpcInvalidCC:
                        pErrStr = "RpcInvalidCC";
                        break;
                case Tfa98xx_Error_RpcInvalidSeq:
                        pErrStr = "RpcInvalidSec";
                        break;
		case Tfa98xx_Error_RpcInvalidParam:
			pErrStr = "RpcInvalidParam";
			break;
		case Tfa98xx_Error_RpcBufferOverflow:
			pErrStr = "RpcBufferOverflow";
			break;
		case Tfa98xx_Error_RpcCalibBusy:
			pErrStr = "RpcCalibBusy";
			break;
		case Tfa98xx_Error_RpcCalibFailed:
			pErrStr = "RpcCalibFailed";
			break;
                case Tfa98xx_Error_Not_Supported:
                        pErrStr = "Not_Supported";
                        break;
                case Tfa98xx_Error_I2C_Fatal:
                        pErrStr = "I2C_Fatal";
                        break;
                case Tfa98xx_Error_I2C_NonFatal:
                        pErrStr = "I2C_NonFatal";
                        break;
                case Tfa98xx_Error_StateTimedOut:
                        pErrStr = "WaitForState_TimedOut";
                        break;
                default:
                        pErrStr = "Unspecified error";
                        break;
        }
        return pErrStr;
}
/*
 *
 */
Tfa98xx_Error_t
Tfa98xx_DspReadSpeakerParameters(Tfa98xx_handle_t handle,
				 int length, unsigned char *pSpeakerBytes)
{
	return tfa98xx_dsp_read_spkr_params(handle,
					 SB_PARAM_GET_LSMODEL, length,
					 pSpeakerBytes);
}

Tfa98xx_Error_t
Tfa98xx_DspReadExcursionModel(Tfa98xx_handle_t handle,
			      int length, unsigned char *pSpeakerBytes)
{
	return tfa98xx_dsp_read_spkr_params(handle,
					 SB_PARAM_GET_XMODEL, length,
					 pSpeakerBytes);
}

/* biquad max1 */
/* the number of biquads supported */#define BIQUAD_COEFF_SIZE       6
#define EQ_COEFF_SIZE           7
#define TFA98XX_BIQUAD_NUM              10

/*
 * Floating point calculations must be done in user-space
 */
static Tfa98xx_Error_t
calcBiquadCoeff(float b0, float b1, float b2,
		float a1, float a2, unsigned char bytes[BIQUAD_COEFF_SIZE * 3])
{
	float max_coeff;
	int headroom;
	int coeff_buffer[BIQUAD_COEFF_SIZE];
	/* find max value in coeff to define a scaler */
#ifdef __KERNEL__
	float mask;
	max_coeff = abs(b0);
	if (abs(b1) > max_coeff)
		max_coeff = abs(b1);
	if (abs(b2) > max_coeff)
		max_coeff = abs(b2);
	if (abs(a1) > max_coeff)
		max_coeff = abs(a1);
	if (abs(a2) > max_coeff)
		max_coeff = abs(a2);
	/* round up to power of 2 */
	mask = 0x0040000000000000;
	for (headroom = 23; headroom > 0; headroom--) {
		if (max_coeff & mask)
			break;
		mask >>= 1;
	}
#else
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
#endif
	/* some sanity checks on headroom */
	if (headroom > 8)
		return Tfa98xx_Error_Bad_Parameter;
	if (headroom < 0)
		headroom = 0;
	/* set in correct order and format for the DSP */
	coeff_buffer[0] = headroom;
#ifdef __KERNEL__
	coeff_buffer[1] = (int) TO_INT(-a2 * (1 << (23 - headroom)));
	coeff_buffer[2] = (int) TO_INT(-a1 * (1 << (23 - headroom)));
	coeff_buffer[3] = (int) TO_INT(b2 * (1 << (23 - headroom)));
	coeff_buffer[4] = (int) TO_INT(b1 * (1 << (23 - headroom)));
	coeff_buffer[5] = (int) TO_INT(b0 * (1 << (23 - headroom)));
#else
	coeff_buffer[1] = (int) (-a2 * pow(2.0, 23 - headroom));
	coeff_buffer[2] = (int) (-a1 * pow(2.0, 23 - headroom));
	coeff_buffer[3] = (int) (b2 * pow(2.0, 23 - headroom));
	coeff_buffer[4] = (int) (b1 * pow(2.0, 23 - headroom));
	coeff_buffer[5] = (int) (b0 * pow(2.0, 23 - headroom));
#endif

	/* convert to fixed point and then bytes suitable for
	 * transmission over I2C */
	tfa98xx_convert_data2bytes(BIQUAD_COEFF_SIZE, coeff_buffer, bytes);
	return Tfa98xx_Error_Ok;
}

Tfa98xx_Error_t
Tfa98xx_DspBiquad_SetCoeff(Tfa98xx_handle_t handle,
			   int biquad_index, float b0,
			   float b1, float b2, float a1, float a2)
{
	enum Tfa98xx_Error error;
	unsigned char bytes[BIQUAD_COEFF_SIZE * 3];

	if (biquad_index > TFA98XX_BIQUAD_NUM)
		return Tfa98xx_Error_Bad_Parameter;
	if (biquad_index < 1)
		return Tfa98xx_Error_Bad_Parameter;
	error = calcBiquadCoeff(b0, b1, b2, a1, a2, bytes);
	if (error == Tfa98xx_Error_Ok) {
		error =
		    tfa_dsp_cmd_id_write(handle, MODULE_BIQUADFILTERBANK,
					(unsigned char)biquad_index,
					(BIQUAD_COEFF_SIZE * 3), bytes);
	}
	return error;
}
Tfa98xx_Error_t Tfa98xx_SetSampleRate(Tfa98xx_handle_t handle, int rate)
{
#define TFA98XX_I2SREG             0x04
#define TFA98XX_I2SREG_I2SSR_POS         12
#define TFA98XX_I2SREG_CHS12_POS         3

/* sample rates */
/* I2S_CONTROL bits */
#define TFA98XX_I2SCTRL_RATE_08000 (0<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_11025 (1<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_12000 (2<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_16000 (3<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_22050 (4<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_24000 (5<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_32000 (6<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_44100 (7<<TFA98XX_I2SREG_I2SSR_POS)
#define TFA98XX_I2SCTRL_RATE_48000 (8<<TFA98XX_I2SREG_I2SSR_POS)

    enum Tfa98xx_Error error;
    unsigned short value = 0;

    /* read the SystemControl register, modify the bit and write again */
    error = reg_read(handle, TFA98XX_I2SREG, &value);
    if (error == Tfa98xx_Error_Ok) {
        /* clear the 4 bits first */
        value &= (~(0xF << TFA98XX_I2SREG_I2SSR_POS));
        switch (rate) {
        case 48000:
            value |= TFA98XX_I2SCTRL_RATE_48000;
         break;
        case 44100:
            value |= TFA98XX_I2SCTRL_RATE_44100;
            break;
        case 32000:
            value |= TFA98XX_I2SCTRL_RATE_32000;
            break;
        case 24000:
            value |= TFA98XX_I2SCTRL_RATE_24000;
            break;
        case 22050:
            value |= TFA98XX_I2SCTRL_RATE_22050;
            break;
        case 16000:
            value |= TFA98XX_I2SCTRL_RATE_16000;
            break;
        case 12000:
            value |= TFA98XX_I2SCTRL_RATE_12000;
            break;
        case 11025:
            value |= TFA98XX_I2SCTRL_RATE_11025;
            break;
        case 8000:
            value |= TFA98XX_I2SCTRL_RATE_08000;
            break;
        default:
            error = Tfa98xx_Error_Bad_Parameter;
        }
    }
	if (error == Tfa98xx_Error_Ok)	/*set sample rate */
		error = reg_write(handle, TFA98XX_I2SREG, value);
		
	return error;
}
Tfa98xx_Error_t
Tfa98xx_SelectChannel(Tfa98xx_handle_t handle, Tfa98xx_Channel_t channel)
{
    enum Tfa98xx_Error error;
    unsigned short value;
    /* read the SystemControl register, modify the bit and write again */
    error = reg_read(handle, TFA98XX_I2SREG, &value);
    if (error == Tfa98xx_Error_Ok) {
        /* clear the 2 bits first */
        value &= ~(0x3 << TFA98XX_I2SREG_CHS12_POS);
        switch (channel) {
        case Tfa98xx_Channel_L:
            value |= (0x1 << TFA98XX_I2SREG_CHS12_POS);
            break;
        case Tfa98xx_Channel_R:
            value |= (0x2 << TFA98XX_I2SREG_CHS12_POS);
            break;
        case Tfa98xx_Channel_L_R:
            value |= (0x3 << TFA98XX_I2SREG_CHS12_POS);
            break;
        case Tfa98xx_Channel_Stereo:
            /* real stereo on 1 DSP not yet supported */
            error = Tfa98xx_Error_Not_Supported;
            break;
        default:
            error = Tfa98xx_Error_Bad_Parameter;
        }
    }
    if (error == Tfa98xx_Error_Ok)
        error = reg_write(handle, TFA98XX_I2SREG, value);

    return error;
}

Tfa98xx_Error_t Tfa98xx_SetVolume(Tfa98xx_handle_t handle, float voldB)
{
    /* This function is depricated, as floating point is not allowed in TFA 
     * layer code.
     */
    enum Tfa98xx_Error error = Tfa98xx_Error_Ok;
    if (voldB > 0.0) {
        error = Tfa98xx_Error_Bad_Parameter;
    }
    /* 0x00 -> 0.0 dB
        * 0x01 -> -0.5 dB
        * ...
        * 0xFE -> -127dB
        * 0xFF -> muted
    */
    if (error == Tfa98xx_Error_Ok) {
        int volume_value;
#ifdef __KERNEL__
        volume_value = TO_INT(voldB * -2);
#else
        volume_value = (int)(voldB / (-0.5f));
#endif
        if (volume_value > 255)	/* restricted to 8 bits */
            volume_value = 255;

        error = tfa98xx_set_volume_level(handle, (unsigned short)volume_value);
    }
    return error;

}
/* Execute RPC protocol to read something from the DSP */
Tfa98xx_Error_t
Tfa98xx_DspGetParam(Tfa98xx_handle_t handle,
		    unsigned char module_id,
		    unsigned char param_id, int num_bytes, unsigned char data[])
{
	return tfa_dsp_cmd_id_write_read(handle,
		    module_id,
		    param_id, num_bytes, data);
}
