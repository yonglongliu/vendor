/*
 * tfaOsal.c
 *
 *  Operating specifics
 */


#ifndef TFAOSAL_H_
#define TFAOSAL_H_

/* load hal plugin from dynamic lib */
void * tfaosal_plugload(char *libarg);
/*generic function to load library and symbol from the library*/
extern void * load_library(char *libname);
extern void * load_symbol(void *dlib_handle, char *dl_sym);

int tfaosal_filewrite(const char *fullname, unsigned char *buffer, int filelenght );

#if defined (WIN32) || defined(_X64)
//suppress warnings for unsafe string routines.
#pragma warning(disable : 4996)
#pragma warning(disable : 4054)
char *basename(const char *fullpath);
#define bzero(ADDR,SZ)	memset(ADDR,0,SZ)
#define isblank(C) (C==' '||C=='\t')
#endif

void tfaRun_Sleepus(int us);

/*
 * Read file
 */
int  tfaReadFile(char *fname, void **buffer);

#endif
