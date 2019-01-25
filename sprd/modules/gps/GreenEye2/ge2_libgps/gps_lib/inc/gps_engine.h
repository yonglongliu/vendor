#ifndef GPS_ENGINE_NEW_H
#define GPS_ENGINE_NEW_H

#define SHUTDOWN                    0x00
#define COMMAND                     0x01
#define IDLEON_STAT                 0x02
#define IDLEOFF_STAT                0x04
#define IDLEON_OK                   0x08
#define IDLEOFF_OK             		0x10
#define EPHEMERIS              		0x20
#define TIME                   		0x40
#define LOCATION               		0x80
#define POWERON                		0x100
#define CHIPRESET              		0x200
#define REQUEST_VERSION        		0x201
#define REQUEST_CHIP           		0x202
#define REQUEST_GPSTIME        		0x203
#define REQUEST_CONSTELLATION  		0x204
#define SET_CPMODE             		0x205
#define SET_OUTTYPE            		0x206
#define CLEAR_CONFIG           		0x207
#define SLEEP_NOTIFY           		0x400
#define WAKEUP_NOTIFY          		0x800
#define SET_INTERVAL           		0x208
#define SET_LTE_ENABLE         		0x50
#define SET_CMCC_MODE          		0x51
#define SET_NET_MODE           		0x52
#define SET_REALEPH                 0x53
#define SET_WIFI_STATUS             0x54
#define SET_ASSERT_STATUS           0x55
#define REQUEST_TSXTEMP        		0x56     // TSXTEMP
#define SET_DELETE_AIDING_DATA_BIT  0x57
#define SET_BASEBAND_MODE           0x58

#define EUT_TEST_CW                 0x209
#define EUT_TEST_TSX                0x210
#define EUT_TEST_TCXO               0x211
#define EUT_TEST_MIDBAND            0x212
#define EUT_TEST_TCXO_TSX           0x213


// GNSS_REG_WR
#define WRITE_REG                0x214
#define READ_REG                 0x215
#define RF_DATA_CAPTURE          0x216

#define ASSIST_END    13
#define SUPL_END      14
#define ASSIST        15
#define SUPL_READY    16
#define MSA_POSITION  17
#define POSITION      18

#endif
