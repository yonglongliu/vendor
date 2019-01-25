/**
*	\file	alwrapper_flicker_errcode.h
*	\brief	Flicker Wrapper related error code.
*	\author		Hubert Huang
*	\version	1.0
*	\date		2016/01/05
*******************************************************************************/
#ifndef ALFLICKERWRP_ERR_H_
#define ALFLICKERWRP_ERR_H_



////Error Code
#define ERR_WPR_FLICKER_SUCCESS    (0x0)
#define ERR_WPR_FLICKER_BASE       (0x9500)

#define ERR_WRP_FLICKER_EMPTY_METADATA       (ERR_WPR_FLICKER_BASE +  0x01 )
#define ERR_WRP_FLICKER_INVALID_INPUT_PARAM  (ERR_WPR_FLICKER_BASE +  0x02 )
#define ERR_WRP_FLICKER_INVALID_INPUT_HZ     (ERR_WPR_FLICKER_BASE +  0x03 )
#define ERR_WRP_FLICKER_INVALID_INPUT_SETVAL (ERR_WPR_FLICKER_BASE +  0x04 )
#define ERR_WRP_FLICKER_INVALID_STATS_SIZE   (ERR_WPR_FLICKER_BASE +  0x05 )
#define ERR_WRP_FLICKER_INVALID_STATS_ADDR   (ERR_WPR_FLICKER_BASE +  0x06 )

#endif  //ALFLICKERWRP_ERR_H_