/*
 * climax.c
 *
 *  Created on: Apr 3, 2012
 *      Author: nlv02095
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(WIN32) || defined(_X64)
#include <windows.h>
#else
#include <unistd.h>
#include "udp_sockets.h"
#endif
#include <ctype.h>
#include <stdint.h>
#include "cmdline.h"
#include "climax.h"
#include "dbgprint.h"
#include "tfaContUtil.h"
#include "tfa.h"
#include "tfa98xxLiveData.h"
#include "tfa98xxCalibration.h"
#include "tfa98xxDiagnostics.h"
#include "tfa_container.h"
#include "tfaContainerWrite.h"
#include "tfaOsal.h"
#include "NXP_I2C.h"  /* for HAL */
#include "lxScribo.h" /* for socket i/f */
#include "tfa_ext.h"

void  lxDummyVerbose(int level);
/*
 * globals
 */
/*
 * accounting globals
 */
//int tfa98xx_runtime_verbose;
int gTfaRun_timingVerbose;

unsigned char gTfa98xxI2cSlave;
/*
 * module globals for output control
 */
int cli_verbose=0;	/* verbose flag */
int cli_trace=0 ;	/* message trace flag from bb_ctrl */

int socket_verbose = 0;
void i2cserver_set_verbose(int val);/* used in i2cserver.c */
void cliUdpSocketServer(char *socketnr);
void cli_server(char *socketnr);
const char *defined_pins[] = {
	"GPIO_LED_HEARTBEAT", // ID = 1
	"GPIO_LED_PLAY",
	"GPIO_LED_REC",
	"GPIO_LED_CMD",
	"GPIO_LED_AUDIO_ON",
	"GPIO_LED_USER0",
	"GPIO_LED_USER1",
	"GPIO_INT_L",
	"GPIO_INT_R",
	"GPIO_RST_L",
	"GPIO_RST_R",
	"GPIO_PWR_1V8",
	"GPIO_LED_RED",
	"GPIO_LED_BLUE",
	"GPIO_LED_UDA_ACTIVE",
	"GPIO_UP",
	"GPIO_DOWN",
	"GPIO_X65",
	"GPIO_LED_POWER_N",
	"GPIO_ISP_SELECT_N",
	"GPIO_BOOTSW4",
	"GPIO_BOOTSW3",
	"GPIO_PROFILE_SELECT",
	"GPIO_BOOTSW1",
	"GPIO_SD_ENABLE_POWER",
	"GPIO_USB_VBUS_ENABLE_N",
	"GPIO_LPC_VBUS_ENABLE_N",
	"GPIO_ENABLE_USB_FAST_CHARGE",
	"GPIO_ENABLE_I2S_N",
	"GPIO_LPC_ENABLE_RID",
	"GPIO_USB_VBUS_ACK_N",
	"GPIO_LPC_VBUS_ACK_N",
	"GPIO_USB_ENABLE_RID_GND",
	"GPIO_USB_ENABLE_RID_PD1",
	"GPIO_USB_ENABLE_RID_PD2",
	"GPIO_USB_ENABLE_RID_PD3",
	"GPIO_USB_ENABLE_RID_PD4",
	"GPIO_DETECT_ANA",
	"GPIO_DETECT_I2S2",
	"GPIO_DETECT_I2S1",
	"GPIO_DETECT_SPDIF",
	"GPIO_ENABLE_ANA",
	"GPIO_ENABLE_I2S1",
	"GPIO_ENABLE_LPC",
	"GPIO_ENABLE_SPDIF",
	"GPIO_PHY_PWDN",
	"GPIO_RSV2",
	"GPIO_RSV3",
	"GPIO_RSV4",
	"VIRT_CLOCK_STOP",
	"VIRT_NO_AUDIO_OUT_ENABLE",
	"VIRT_CLEAR_CALIBRATION",
	"VIRT_I2S_SLAVE_ENABLE",
	"VIRT_I2S_32FS_ENABLE",
	"VIRT_BOARD_RESET",
	0
};

#if defined(WIN32) || defined(_X64)
extern int optind;   // processed option count
#endif

int lxScriboListenSocketInit(char *);
void CmdProcess(void* , int );
void free_args_info_memory();

/* Used for the livedata timer */
void timer_handler(void);
int trigger=0;
void timer_handler(void)
{
	trigger = 1;
}


/*
 * translate string to uppercase
 *  note:  no toupper() in android
 */
static void to_upper(char *str) {
	while (*str!='\0') {
		if (*str  >= 'a' && *str <= 'z') {
			*str = ('A' + *str - 'a');
			str++;
		}
		else
			str++;
	}
}
	
/**
 * command line arguments to control the main state of the Speakerboost
 * state of the system.
 *
 *  usage: climax start [profile] [vstep]
 *             climax stop
 *
 * @param argc argument count
 * @param *argv[] arguments
 * @return 0 if ok
 */

int climax_args(int argc, char *argv[]) {
	Tfa98xx_Error_t err = Tfa98xx_Error_Ok;
	uint16_t bf_value = 0;
	char *p;
	int i, slave_argument=-1, tfa_argument=-1, loop_argument=-1;

	if ( argc == 0)
		return 0;

	slave_argument = (gCmdLine.slave_given ? gCmdLine.slave_arg : -1);
	tfa_argument = (gCmdLine.tfa_given ? gCmdLine.tfa_arg : -1);

	for(i=0; i<argc; i++) {
		loop_argument = (gCmdLine.loop_given ? gCmdLine.loop_arg : -1);
		p = strchr(argv[i],'=');

		if (p) {
			*p++='\0';
			bf_value=(uint16_t)strtol(p, NULL, 0);
		}

		to_upper(argv[i]);
		do {
			if (p) { /* write */
				err = tfa_rw_bitfield((const char *)argv[i], bf_value, slave_argument, tfa_argument, 1);
			} else { /* read */
				err = tfa_rw_bitfield((const char *)argv[i], 0, slave_argument, tfa_argument, 0);
			}

			loop_argument--;
		} while(loop_argument > 0);
	}

	return  !( err == Tfa98xx_Error_Ok);
}

/*
 * functions called from scribo telnet
 */
int climain(int argc, char *argv[]);
int cliload(char *name) {
	return tfa98xx_cnt_loadfile(name, 0);
}
int main(int argc, char *argv[]) {
	return climain(argc, argv);
}
int climain(int argc, char *argv[])
{
	char *devicename, *xarg;
	int profile=0, vsteps[TFACONT_MAXDEVS];
	int status, i=0, j;
	Tfa98xx_handle_t handlesIn[] ={-1, -1}; //default one device
	int maxdev, error=0;
	int cnt_verbose = 0;
	char buffer[8*1024];	//,byte TODO check size or use malloc
	int length=0;

	devicename = cliInit(argc, argv);

	for(j=0; j<TFACONT_MAXDEVS; j++) {
		if (gCmdLine.volume_given) {
			vsteps[j] = gCmdLine.volume_arg;
		} else {
			vsteps[j] = 0;
		}
	}

	if ( gCmdLine.tfa_given )
		tfa_cont_dev_type(gCmdLine.tfa_arg);
		
	if ( gCmdLine.ini2cnt_given ) {
		return tfaContIni2Container(gCmdLine.ini2cnt_arg);
	}

	if ( gCmdLine.bin2hdr_given ) {
		xarg = argv[optind]; // this is the remaining argv
		if(xarg) {
			error = tfaContBin2Hdr(gCmdLine.bin2hdr_arg, argc-optind, &argv[optind]);
			PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
			return error;
		} else {
			PRINT_ERROR("Please specify the header arguments.\n");
			return 1;
		}
	}

	/*
	 * load container
	 */
	if ( gCmdLine.load_given) {
		/* container file contents should ONLY be printed
		 * when loading container with verbose is given. Not all the time
		 * when verbose_given in combination with other commands! 
		 */
		if ( (argc == 4) && gCmdLine.verbose_given )
			cnt_verbose = gCmdLine.verbose_arg & 0x0f;

		if( (argc == 5) && gCmdLine.verbose_given && gCmdLine.device_given)
			cnt_verbose = gCmdLine.verbose_arg & 0x0f;

		error = tfa98xx_cnt_loadfile(gCmdLine.load_arg, cnt_verbose);

		if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
			PRINT_ERROR("Load failed\n");
			PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
			return error;
		}

		if(cli_verbose)
			PRINT("container:%s\n", gCmdLine.load_arg);

		tfaContGetSlave(0, &gTfa98xxI2cSlave); // set 1st slave
	}

	/* Only for printing profile info */
	if ( gCmdLine.profile_given ) {
		if(strcmp(gCmdLine.profile_arg[0], "-1")==0) {
			int i, vsteps;
			char app_name[9];
			// show device 0 profiles
			tfa_cnt_get_app_name(app_name);
			PRINT("Application: %s\n", app_name);
			for(i=0;i<tfaContMaxProfile(0);i++) {
				vsteps = tfacont_get_max_vstep(0, i);
				PRINT("Profile[%d]:%s [%d]\n", i, tfaContProfileName(0,i), vsteps);
			}
		}
	}

	if ( gCmdLine.slave_given ) {
		if(gCmdLine.slave_arg == -1) {
			for (i=0; i < tfa98xx_cnt_max_device(); i++ ) {
				tfaContGetSlave(i, &gTfa98xxI2cSlave); /* get device I2C address */
				PRINT("slave[%d]: %s (0x%02x)\n", i, tfaContDeviceName(i), gTfa98xxI2cSlave);
			}
		}
		gTfa98xxI2cSlave = (unsigned char) gCmdLine.slave_arg;
	}

	if ( cli_verbose ) {
		if ( tfa98xx_cnt_max_device() == -1) {
			PRINT("devicename=%s, i2c=0x%02x\n" ,devicename, gTfa98xxI2cSlave); //TODO use lxScriboGetName() after register
		} else {
			PRINT("devicename=%s\n", devicename);
			for (i=0; i < tfa98xx_cnt_max_device(); i++ ) {
				tfaContGetSlave(i, &gTfa98xxI2cSlave); /* get device I2C address */
				PRINT("\tFound Device[%d]: %s at 0x%02x.\n", i, tfaContDeviceName(i), gTfa98xxI2cSlave);
			}
		}
	}

	tfa98xxI2cSetSlave(gTfa98xxI2cSlave);

	// split the container file into individual files
	if ( gCmdLine.splitparms_given) {
		if ( tfa98xx_cnt_max_device() == -1) {
			PRINT("Please supply a container file with profile argument.\n");
			return error;
		} else {
			error = tfa98xx_cnt_split(gCmdLine.load_arg, gCmdLine.splitparms_arg);
			if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
				PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
				return error;
			}
		}
	}

	cliTargetDevice(devicename);

/*** moved pins to the end ***/

	/* If we have a device we need to get device info */
	if ((error == Tfa98xx_Error_Ok) && (tfa98xx_get_cnt() != NULL)) {		
		error = tfa_probe_all(tfa98xx_get_cnt());
		
		if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
		    PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
		    return error;
		}
	}

	tfaLiveDataVerbose(cli_verbose);
	tfaDiagnosticVerbose(cli_verbose);
    tfaCalibrationVerbose(cli_verbose);
    
	if ( gCmdLine.speaker_given) {
		if( handlesIn[0] == -1) //TODO properly handle open/close dev
			error = Tfa98xx_Open(gTfa98xxI2cSlave*2, &handlesIn[0]);
					
		if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
			PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
			return error;
		}

		tfa98xx_set_spkr_select(handlesIn[0], gCmdLine.speaker_arg);
	}

	if ( gCmdLine.start_given ) {
		j=0;
		if ( tfa98xx_cnt_max_device() == -1) {
			PRINT("Please provide container file for this option.\n");
			return error;
		} else {
			do {
				if ( gCmdLine.profile_given ) {
					error = getProfileInfo(gCmdLine.profile_arg[j], &profile);
					if (error != Tfa98xx_Error_Ok) {
						PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
						return error;
					}
				}
				if(profile != -1) {
					error = tfa_start(profile, vsteps);
					if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
						PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
						return error;
					}
				}
				j++;
			} while(j < (int)gCmdLine.profile_given);
		}
	}

	if (gCmdLine.stop_given) {
		if (cli_verbose)
			PRINT("Stop given\n");

		error = tfa_stop();

		free_args_info_memory();
		free_gCont();

		if (error != Tfa98xx_Error_Ok) {	// error with profile number so exit
			PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
			return error;
		}

		return error;
	}

    status = cliCommands(argv[optind], handlesIn, profile);	// execute the command options first
    if (status) 
		return status;

    /************************** main start/stop ***************************/
    if ( !gCmdLine.pin_given ) {
    	status = climax_args(argc-optind, &argv[optind]);

    	if ( status ) {
    		PRINT("error from args : %d\n", status);
    	}
    }
    /************************** main start/stop done *********************/
    if ( gCmdLine.record_given ) {  
		int loopcount=gCmdLine.count_arg;
		int nr_of_items=0, j=0, count=1;
		float live_data[MEMTRACK_MAX_WORDS] = {0};
		FILE *file = NULL;

		maxdev=tfa98xx_cnt_max_device(); /* slave option not allowed */
		if ( maxdev <= 0 ) {
			PRINT("Please provide container file.\n");
			return 0;
		} else {
			PRINT("recording interval time: %d ms\n", gCmdLine.record_arg);
			for (i=0; i < maxdev; i++ ) {	
				tfaContGetSlave(i, &gTfa98xxI2cSlave); /* get device I2C address */
				PRINT("Found Device[%d]: %s at 0x%02x.\n", i, tfaContDeviceName(i), gTfa98xxI2cSlave);

				if( handlesIn[i] == -1) /* TODO properly handle open/close dev */
					error = Tfa98xx_Open(gTfa98xxI2cSlave*2, &handlesIn[i]);
					
				if (error != Tfa98xx_Error_Ok) {	/* error with profile number so exit */
					PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
					return error;
				}  
			}

			/* Clear the buffer before the first use! */
			buffer[0] = '\0';

			/* We need to check if there are items for every device */
			for (i=0; i < maxdev; i++ ) {
				error = tfa98xx_get_live_data_items(handlesIn, i, buffer+length);
				if(error == Tfa98xx_Error_Fail)
					PRINT("Error: buffer size is too short! \n");

				if (error != Tfa98xx_Error_Ok)
					return error;

				length = (int)(strlen(buffer));
			}

			*(buffer+length) = '\0';

			if ( gCmdLine.output_given) {
				file = fopen(gCmdLine.output_arg, "w");
				fwrite(buffer, 1, length, file);
			} else {
				puts(buffer);
				fflush(stdout);
			}

			for (i=0; i < maxdev; i++ ) {
				/* Send the memtrack items. Only for max2 */
				error = tfa98xx_set_live_data(handlesIn, i);
				if (error != Tfa98xx_Error_Ok) {
					PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
					if(file != NULL) {
						fclose(file);
						file = NULL;
					}
					return error;
				}
			}

			/* Start the timer with the given time argument (20ms is default) */
			if(start_timer(gCmdLine.record_arg, &timer_handler)) {
				PRINT("Error: Timer failed to start! \n");
				return Tfa98xx_Error_Other;
			}

			do {
				/* Reset the timer trigger at every start */
				trigger = 0;

				for (i=0; i < maxdev; i++ ) {
					if (!gCmdLine.slave_given)
						tfaContGetSlave(i, &gTfa98xxI2cSlave);

					/* Get the memtrack items data */
					error = tfa98xx_get_live_data(handlesIn, i, live_data, &nr_of_items);
					if(error == Tfa98xx_Error_Fail)
						PRINT("Error: buffer size is too short! \n");

					if (error != Tfa98xx_Error_Ok && error != Tfa98xx_Error_I2C_NonFatal
							&& error != Tfa98xx_Error_Other) {
						PRINT("last status: %d (%s)\n", error, Tfa98xx_GetErrorString(error));
						return error;
					}

					/* If the device does not have a device section skip the printing */
					if(error != Tfa98xx_Error_Other) {
						if ( gCmdLine.output_given) {
							fprintf(file, "%d: 0x%02x,", count, gTfa98xxI2cSlave);
							for(j=0; j<nr_of_items; j++)
								fprintf(file, "%f,", live_data[j]);
							fprintf(file, "\n");
						} else {
							PRINT("%d: 0x%02x,", count, gTfa98xxI2cSlave);
							for(j=0; j<nr_of_items; j++)
								PRINT("%f,", live_data[j]);
							PRINT("\n");
						}
						count++;
					}
				}
				loopcount = ( gCmdLine.count_arg == 0) ? 1 : loopcount-1;

				/* If the trigger is already set before we are in the while loop the interval time is to fast */
				if(trigger == 1) {
					PRINT("Error: We cannot reach a time interval of %d msec \n", gCmdLine.record_arg);
					return Tfa98xx_Error_Other;
				}

				/* Wait for the timer to be ready */
				while(trigger == 0) {
					/* Windows required a sleep to work */
					extern_msleep_interruptible(1);
				}

			} while (loopcount>0);
		}

		if ( gCmdLine.output_given) {
			fclose(file);
			file = NULL;
			PRINT("written to %s\n", gCmdLine.output_arg);
		}
    }

#if !(defined(WIN32) || defined(_X64))
    if ( gCmdLine.server_given ) {
    	int result=-1;
    	activeSocket = result  = server_register(gCmdLine.server_arg);
    	if(result==-1)
    		PRINT("Network error: server not registered\n");
       // PRINT("statusreg:0x%02x\n", tfa98xxReadRegister(0,handlesIn)); // read to ensure device is opened
	if ( strchr( gCmdLine.server_arg , '@' ) != 0) {
		{
			int dsp_status;
			//Get pin to verify the device is warm / cold!
			dsp_status = NXP_I2C_GetPin(8);
			dsp_status = (dsp_status & 0x2000) >> 13; //Using the tfadsp_is_configured bit
			if(dsp_status) {
				printf("ext_dsp is set to warm! \n");
				tfa_ext_set_ext_dsp(2); /* set warm */
			}
		}
		//server_register(gCmdLine.server_arg);
		//cliUdpSocketServer(&gCmdLine.server_arg[1]);
		cli_server(&gCmdLine.server_arg[1]);
	} else
		//server_register(gCmdLine.server_arg);
		//cliSocketServer(gCmdLine.server_arg); // note socket is ascii string
		cli_server(&gCmdLine.server_arg[0]);
    }
    if ( gCmdLine.client_given ) {
	//PRINT("statusreg:0x%02x\n", tfa98xxReadRegister(0,handlesIn)); // read to ensure device is opened
	cliClientServer(gCmdLine.client_arg); // note socket is ascii string
    }
#endif

    /***** pins *****/
	if ( gCmdLine.pin_given ) {
		i = 0;
		xarg = argv[optind]; // this is the remaining argv
		/* --pin=help returns the list of available pins */
		if (strncmp("help", gCmdLine.pin_arg, 4) == 0) {
			printf("list of available pins:\n");
			while (defined_pins[i] != NULL) {
				if(strncmp(defined_pins[i], "VIRT_", 5) != 0) {
					printf("(%02d) %s\n", i+1, defined_pins[i]); /* pin id's start at 1 */
				}
				else {
					printf("(%02d) %s\n", i+1+14, defined_pins[i]); /* offset for virtual pins (14) */
				}
				i++;
			}
		}
		else {
			i = 0;
			/* convert pin names (string) into (integer) string e.g. "GPIO_LED_BLUE" into "14" */
			while (defined_pins[i] != NULL)
			{
				if (strncmp(defined_pins[i], gCmdLine.pin_arg, 64) == 0) {
					if(strncmp(defined_pins[i], "VIRT_", 5) != 0) {
						sprintf(gCmdLine.pin_arg, "%d", i+1); /* pin id's start at 1 */
					}
					else {
						sprintf(gCmdLine.pin_arg, "%d", i+1+14); /* offset for virtual pins (14) */
					}
					/* printf("%s\n", optarg); */
				}
				i++;
			}

			/* wait for a HID event to occur (set pinId, set value [0|1], set timeout in msec (0=forever)) */
			if (gCmdLine.wait_given) {
				int ret = 0;
				if (xarg) {
					ret = lxScriboWaitOnPinEvent(atoi(gCmdLine.pin_arg), atoi(xarg), atoi(gCmdLine.wait_arg));
				}
				else {
					PRINT("Please provide a pinstate argument with wait argument.\n");
				}

				return ret;
			}

			if (xarg) {//set if extra arg
					int radix = strchr(xarg, 'x') == NULL ? 10 : 16;
					error = NXP_I2C_SetPin(atoi(gCmdLine.pin_arg), strtol(xarg,NULL,radix));
					PRINT("pin%d < %d\n", atoi(gCmdLine.pin_arg), (int)strtol(xarg,NULL,radix));
			} else {
					PRINT("pin%d : %d\n", atoi(gCmdLine.pin_arg), NXP_I2C_GetPin(atoi(gCmdLine.pin_arg)));
			}
		}

		return error;
	}

	//tfa_ext_exit_wait();
    /**** pins end ****/
	NXP_I2C_Close();

	free_args_info_memory();
	free_gCont();

    if ( strcmp(argv[0], "server"))
		exit (status); // normal
    else
		return status;
}

#if !(defined(WIN32) || defined(_X64))
/*
 *
 */
void cli_server(char *socket)
{
	 int length=0, i;
	 uint8_t buf[4000];

	//activeSocket=(*server_dev->init)(socket);

	if(activeSocket<0) {
		PRINT_ERROR("something wrong with socket %s\n", socket);
		return;
	}
	while(1){
		if(activeSocket >= 0)
			length = (*server_dev->read)(activeSocket, buf, sizeof(buf));
		if (socket_verbose & (length>0)) {
			PRINT("recv: ");
			for(i=0;i<length;i++)
				 PRINT("0x%02x ", buf[i]);
			PRINT("\n");
		}
		if (length>0)
		  CmdProcess(buf,  length);
		else {
			(*server_dev->close)(activeSocket);
			/*tfaRun_Sleepus(10000);*/ /*to be removed after testing*/
			activeSocket=(*server_dev->init)(socket);
		}
	}
}
/*UDP server*/
void cliUdpSocketServer(char *socketnr)
{
	int port = atoi(socketnr);
	int fd = udp_socket_init(NULL, port);
	int length, i;
	uint8_t buf[4000];

	if(fd<0) {
		PRINT_ERROR("something wrong with socket %s\n", socketnr);
		return;
	}
	activeSocket = fd; //global used in CmdProcess

	while(1){
		//length = read(activeSocket, buf, 256);
		length = udp_read(fd, (char*)buf, sizeof(buf), 0); //timeout=0 blocks
		if (socket_verbose & (length>0)) {
			PRINT("recv: ");
			for(i=0;i<length;i++)
			     PRINT("0x%02x ", buf[i]);
			PRINT("\n");
		}
		if (length>0)
		  CmdProcess(buf,  length);
		else {
			close(fd);
			tfaRun_Sleepus(10000);
			fd = udp_socket_init(NULL, port);
		}
	}

}

//TCP
void cliSocketServer(char *socket)
{
	 int length=0, i;
	 uint8_t buf[256];

	activeSocket=lxScriboListenSocketInit(socket);

	if(activeSocket<0) {
		PRINT_ERROR("something wrong with socket %s\n", socket);
		lxScriboSocketExit(1);
	}
	while(1){
		if(activeSocket >= 0)
			length = read(activeSocket, buf, 255);
		if (socket_verbose & (length>0)) {
			PRINT("recv: ");
			for(i=0;i<length;i++)
			     PRINT("0x%02x ", buf[i]);
			PRINT("\n");
		}
		if (length>0)
		  CmdProcess(buf,  length);
		else {
			close(activeSocket);
			tfaRun_Sleepus(10000);
			activeSocket=lxScriboListenSocketInit(socket);
		}
	}

}
void cliClientServer(char *server)
{
	 int length=0, i;
	 uint8_t buf[256];

	activeSocket=lxScriboSocketInit(server);

	if(activeSocket<0) {
		PRINT_ERROR("something wrong with client %s\n", server);
		exit(1);
	}

	while(1){
		if(activeSocket >= 0)
			length = read(activeSocket, buf, 255);
		if (socket_verbose & (length>0)) {
			PRINT("recv: ");
			for(i=0;i<length;i++)
			     PRINT("0x%02x ", buf[i]);
			PRINT("\n");
		}
		if (length>0)
		  CmdProcess(buf,  length);
		else {
			close(activeSocket);
			tfaRun_Sleepus(10000);
			activeSocket=lxScriboSocketInit(server);
		}
	}
}
#endif

/*
 * init the gengetopt stuff
 */
char *cliInit(int argc, char **argv)
{
	char *devicename;
	int lxScribo_verbose=0;

    cmdline_parser (argc, argv, &gCmdLine);
    if(argc==1) // nothing on cmdline
    {
		cmdline_parser_print_help();
		exit(1);
    }
    // extra command line arg for test settings 
	// argv[optind]; // this is the remaining argv
	//  lxDummyArg is now passed via the devname: e.g. -ddummy97,warm

    // generic flags
    if (gCmdLine.verbose_given) {
	cli_verbose= (gCmdLine.verbose_arg & 0x0f);
	lxScribo_verbose      =  (1 & gCmdLine.verbose_arg)!=0;
	socket_verbose 		  =  (2 & gCmdLine.verbose_arg)!=0;
#if !(defined(WIN32) || defined(_X64)) // no --server in windows
	i2cserver_set_verbose ( (4 & gCmdLine.verbose_arg)!=0);
#endif
	gTfaRun_timingVerbose =  (8 & gCmdLine.verbose_arg)!=0;
	cli_trace = 				 (0x10 & gCmdLine.verbose_arg)!=0;
		tfa_verbose(cli_verbose);
		lxScriboVerbose(lxScribo_verbose);
		lxDummyVerbose(gCmdLine.verbose_arg>>4); //use highes nibble
	}

    tfa_cnt_verbose(cli_verbose);
    tfa_cont_write_verbose(cli_verbose);

	NXP_I2C_Trace_file(gCmdLine.trace_arg); /* if 0 stdout will be used */
	NXP_I2C_Trace(gCmdLine.trace_given | gCmdLine.monitor_given<<1 );   /* if file is open it will be used */
	NXP_UDP_Trace(gCmdLine.trace_given);

    if (gCmdLine.device_given)
	devicename = gCmdLine.device_arg;
    else
	devicename = TFA_I2CDEVICE;

    return devicename;

}

void free_args_info_memory()
{
	free(gCmdLine.calibrate_arg);
	free(gCmdLine.dump_arg);
	free(gCmdLine.speaker_arg);
	free(gCmdLine.dumpmodel_arg);
	free(gCmdLine.server_arg);
	free(gCmdLine.client_arg);
	free(gCmdLine.profile_orig);
	free(gCmdLine.load_arg);
	free(gCmdLine.load_orig);
	free(gCmdLine.device_arg);
	free(gCmdLine.device_orig);
	free(gCmdLine.register_arg);
}

