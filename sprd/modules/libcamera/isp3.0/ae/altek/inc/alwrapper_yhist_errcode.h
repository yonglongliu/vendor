/**
*	\file	alwrapper_flicker_errcode.h
*	\brief	Flicker Wrapper related error code.
*	\author		Hubert Huang
*	\version	1.0
*	\date		2016/01/05
*******************************************************************************/
#ifndef ALYHISTWRP_ERR_H_
#define ALYHISTWRP_ERR_H_

/* error code */
#define ERR_WPR_YHIST_SUCCESS    (0x0)
#define ERR_WPR_YHIST_BASE       (0x9600)

#define ERR_WRP_YHIST_EMPTY_METADATA       (ERR_WPR_YHIST_BASE +  0x01 )
#define ERR_WRP_YHIST_INVALID_INPUT_PARAM  (ERR_WPR_YHIST_BASE +  0x02 )
#define ERR_WRP_YHIST_INVALID_STATS_ADDR   (ERR_WPR_YHIST_BASE +  0x03 )

#endif  //ALYHISTWRP_ERR_H_