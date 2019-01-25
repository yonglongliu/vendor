#include <assert.h>
#include <string.h>
#if defined(WIN32) || defined(_X64)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <NXP_I2C.h>
#ifndef WIN32
#include <inttypes.h>
#endif
#include "tfa.h" /* top tfa interface */
#include "tfaContUtil.h"
#include "tfa_container.h"
#include "dbgprint.h"
#include "tfaOsal.h" /* tfaReadFile */
#include "tfa98xxLiveData.h"
#include "Tfa98API.h"
#include "tfa_service.h"

#ifndef WIN32
#define Sleep(ms) usleep((ms)*1000)
#define _GNU_SOURCE   /* to avoid link issues with sscanf on NxDI? */
#endif

#define MAX_DEVICES 4 

int load_container_file(char *fname,  nxpTfaContainer_t **buffer) {
	int size;

	size = tfaReadFile(fname, (void**) buffer); //mallocs file buffer

	if ( size==0 )
		printf("error reading %s\n", fname);

	return size;
}

enum Tfa98xx_Error recordLiveData(char* item_name) {
        enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
        Tfa98xx_handle_t handlesIn[] ={-1, -1};
        unsigned char slaveAddress;
        int i=0, length=0, nr_of_items=0, found_index=0, nameCounter=0, maxdev=tfa98xx_cnt_max_device();
        float live_data[MEMTRACK_MAX_WORDS] = {0};
        char buffer[256];
        char *record_names=NULL;

        /* get list of live data items */
        for (i=0; i < maxdev; i++ ) {	
                tfaContGetSlave(i, &slaveAddress); /* get device I2C address */

                if( handlesIn[i] == -1)
                        err = Tfa98xx_Open(slaveAddress*2, &handlesIn[i]);

                if(err != Tfa98xx_Error_Ok)
                        return err;
        }

        buffer[0] = '\0';
        
        /* Clear the buffer before the first use! */
        length = 0;
        for (i=0; i < maxdev; i++ ) {
                err = tfa98xx_get_live_data_items(handlesIn, i, buffer+length);
                if (err != Tfa98xx_Error_Ok)
                        return err;

                length = (int)(strlen(buffer));
        }
        *(buffer+length) = '\0';

        /* dump buffer to screen: */
        puts(buffer);
        fflush(stdout);

        /* return error if tracked item is not in list */
        if (strstr(buffer, item_name) == NULL)
                return tfa_error_bad_param;

        /* get the index of the tracked item */
        record_names = strtok (buffer,",");
        while (record_names != NULL)
        {
                if( strstr(record_names, item_name) != NULL)
                        found_index = nameCounter;
                nameCounter++;
                record_names = strtok (NULL, ",");
        }
        printf ("item name: %s found at index: %d\n", item_name, found_index);

        for (i=0; i < maxdev; i++ ) {
			err = tfa98xx_set_live_data(handlesIn, i);
			if (err != Tfa98xx_Error_Ok)
					return err;
        }

        /* get the livedata value for each device periodically */
        for (i=0; i < maxdev; i++ ) {
                tfaContGetSlave(i, &slaveAddress);
                err = tfa98xx_get_live_data(handlesIn, i, live_data, &nr_of_items);
                if (err != Tfa98xx_Error_Ok)
                        return err;
                printf("%s [0x%02x]: %f\n", item_name, slaveAddress, live_data[found_index]);	
        }
        return Tfa98xx_Error_Ok;
}

int main(int argc, char* argv[])
{
	enum Tfa98xx_Error err = Tfa98xx_Error_Ok;
	nxpTfaContainer_t *cntbuf;
	int length;
	int i = 0, j = 0, vsteps[MAX_DEVICES]={0,0,0,0};
	int profile;
	unsigned char  tfa98xxI2cSlave; // for i2c address

	char target[FILENAME_MAX];

	printf("Starting application.\n");

	/* Get the container file from command line */ 
	if ( argc < 2 ) {
		printf("Please supply a container file as command line argument.\n");
		return 0;
	} else if ( argc < 3 ) {
		strcpy(target, "dummy"); /* no target specified, use default*/
	} else {
		strncpy(target, argv[2], sizeof(target)); /* target specified */
	}
	//	tfa_cnt_verbose( argc>3);

	/* load the container file into the memory */
	length = load_container_file(argv[1], &cntbuf);
	if (length ==0)  { // read params
		fprintf(stderr, "Load container failed\n");
		return 1;
	} else {
		printf("Loaded container file %s.\n", argv[1]);
	}

	/* pass the container file to the tfa */
	tfa_load_cnt( (void*) cntbuf, length);

	/* open the target device */
	if (NXP_I2C_Open(target) == -1 ) { /* use input device */
		fprintf(stderr, "Could not open target device:%s.\n The second argument can be used to specify a device\n.", target);
		return 1;
	}
	NXP_I2C_Trace(0);

	/* Make sure device family type is set */
	if (tfa98xx_get_cnt() != NULL) {		
		//tfa_soft_probe_all(tfa98xx_get_cnt());
		if( tfa_probe_all(tfa98xx_get_cnt()) != Tfa98xx_Error_Ok ) {
			goto error_exit;
		}
	}

	/* print container files details */
	// for debugging use: tfaContShowContainer();
	for (i=0; i < tfa98xx_cnt_max_device(); i++ ) {
		tfaContGetSlave(i, &tfa98xxI2cSlave); /* get device I2C address */
		printf("\tFound Device[%d]: %s at 0x%02x.\n", i, tfaContDeviceName(i), tfa98xxI2cSlave);
		for (j=0; j < tfaContMaxProfile(i); j++ ) 
			printf("\t\tProfile [%d]: %s, %d vsteps.\n", j, tfaContProfileName(i,j), tfacont_get_max_vstep(i,j));
	}

	printf ("Select a profile [0-%d]: ", (tfaContMaxProfile(0)-1));
	j = scanf ("%d", &profile);

	/* always load vstep[0] */
	err = (enum Tfa98xx_Error)tfa_start(profile, vsteps);

	/* loop up through all vsteps if > 0 */
	for(i=0;i<tfacont_get_max_vstep(0,profile);i++){
		for(j=0;j<tfa98xx_cnt_max_device();j++) /* put all to same vstep */
			vsteps[j]=i;
		err = (enum Tfa98xx_Error)tfa_start(profile, vsteps);
		if ( err!=Tfa98xx_Error_Ok)
			goto error_exit;
	}

	/* loop down through all vsteps if > 0 */
	for(i=tfacont_get_max_vstep(0,profile)-1;i>0;i--){
		for(j=0;j<tfa98xx_cnt_max_device();j++) /* put all to same vstep */
			vsteps[j]=i;
		err = (enum Tfa98xx_Error)tfa_start(profile, vsteps);
		if ( err!=Tfa98xx_Error_Ok)
			goto error_exit;
	}

        err = recordLiveData(argv[3]); /* third argument is the livedata item to be tracked */
        if (err != Tfa98xx_Error_Ok)
                goto error_exit;

	printf ("Play music at 48kHz. Do you want to quit? [type number/char to quit] ");
	j = scanf ("%d", &i);
	if(!j) {
		/**** Erroneous input, get rid of it and retry! */
		scanf ("%*[^\n]");
	}

error_exit:
    if ( err!=Tfa98xx_Error_Ok) {
    	printf("an error occured, code:%d\n",err );
    }
	printf ("\nPowering down.\n");

	tfa_stop();
	tfa_deinit();

	NXP_I2C_Close();

	return 0;
}
