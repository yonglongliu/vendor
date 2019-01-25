/******************************************************************************
 *  File name: alwrapper_common_errcode.h
 *  Create Date:
 *
 *  Comment:
 *
 *
 ******************************************************************************/

#define ERR_WRP_COMM_SUCCESS          (0x0)

#define ERR_WRP_COMM_PARSE_BASE        (0x9700)

#define ERR_WRP_COMM_INVALID_ADDR       ( ERR_WRP_COMM_PARSE_BASE + 0x01)
#define ERR_WRP_COMM_INVALID_SIZEINFO_1   ( ERR_WRP_COMM_PARSE_BASE + 0x02)
#define ERR_WRP_COMM_INVALID_SIZEINFO_2   ( ERR_WRP_COMM_PARSE_BASE + 0x03)
#define ERR_WRP_COMM_INVALID_SIZEINFO_3   ( ERR_WRP_COMM_PARSE_BASE + 0x04)
#define ERR_WRP_COMM_INVALID_IDX            ( ERR_WRP_COMM_PARSE_BASE + 0x05)
#define ERR_WRP_COMM_INVALID_FORMAT         ( ERR_WRP_COMM_PARSE_BASE + 0x06)
#define ERR_WRP_COMM_INVALID_FUNCTION_IDX   ( ERR_WRP_COMM_PARSE_BASE + 0x07)

#define ERR_WRP_COMM_MISSING_FUNCTION(info) ( ERR_WRP_COMM_PARSE_BASE + 0x10 + info)
#define ERR_WRP_COMM_INVALID_SIZEINFO(info) ( ERR_WRP_COMM_PARSE_BASE + 0x20 + info)
#define ERR_WRP_COMM_INVALID_FUNCTION(info) ( ERR_WRP_COMM_PARSE_BASE + 0x30 + info)

