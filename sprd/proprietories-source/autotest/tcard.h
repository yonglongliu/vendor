// 
// Spreadtrum Auto Tester
//
// anli   2012-11-12
//
#ifndef _TCARD_20121112_H__
#define _TCARD_20121112_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_tcard {
//-----------------------------------------------------------------------------
int tcardOpen( void );

int tcardIsPresent( void );
int tcardIsMount( char* mount_path );
int tcardRWTest( char* mount_path );

int tcardClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _TCARD_20121112_H__
