/******************************************************************************
 *  File name: alwrapper_3a_errcode.h
 *  Create Date:
 *
 *  Comment:
 *
 *
 ******************************************************************************/

#define ERR_WRP_SUCCESS          (0x0)
#define ERR_WRP_METADATA_PARSER  (0x9000)
#define ERR_WRP_DL_PARSER        (0x9100)

#define ERR_WRP_NULL_METADATA_ADDR   (ERR_WRP_METADATA_PARSER +  0x01 )
#define ERR_WRP_EMPTY_METADATA       (ERR_WRP_METADATA_PARSER +  0x02 )
#define ERR_WRP_ALLOCATE_BUFFER      (ERR_WRP_METADATA_PARSER +  0x03 )
#define ERR_WRP_INVALID_AE_BLOCKS    (ERR_WRP_METADATA_PARSER +  0x04 )
#define ERR_WRP_INVALID_INPUT_PARAM  (ERR_WRP_METADATA_PARSER +  0x05 )
#define ERR_WRP_INVALID_INPUT_WB     (ERR_WRP_METADATA_PARSER +  0x06 )
#define ERR_WRP_MISMACTHED_STATS_VER (ERR_WRP_METADATA_PARSER +  0x07 )
#define ERR_WRP_MISMACTHED_ISPSTATS_VER (ERR_WRP_METADATA_PARSER +  0x08 )
#define ERR_WRP_MISMACTHED_AFSTATS_VER (ERR_WRP_METADATA_PARSER +  0x09 )

#define ERR_WRP_INVALID_DL_PARAM     (ERR_WRP_DL_PARSER       +  0x01 )
#define ERR_WRP_INVALID_DL_OPMODE    (ERR_WRP_DL_PARSER       +  0x02 )