/*
 * climax.h
 *
 *  Created on: Apr 3, 2012
 *      Author: nlv02095
 */

#ifndef CLIMAX_H_
#define CLIMAX_H_

#include "tfaContUtil.h"
#include "cmdline.h"

char *cliInit(int argc, char **argv);
int cliCommands(char *xarg, Tfa98xx_handle_t *handlesIn, int profile);

int cliTargetDevice(char *devname);
#ifndef WIN32
void cliSocketServer(char *socket);
void cliClientServer(char *socket);
#endif
//int cliSaveParamsFile( char *filename );
//int cliLoadParamsFile( char *filename );

struct gengetopt_args_info gCmdLine; /* Globally visible command line args */

/*
 *  globals for output control
 */
extern int cli_verbose;	/* verbose flag */
extern int cli_trace;	/* message trace flag from bb_ctrl */

#define TRACEIN(F)  if(cli_trace) printf("Enter %s\n", F);
#define TRACEOUT(F) if(cli_trace) printf("Leave %s\n", F);

#endif /* CLIMAX_H_ */
