#ifndef __LIBBT_SPRD_BT_EUT_H__
#define __LIBBT_SPRD_BT_EUT_H__

#define BT_NPI_ATKEY "AT+SPBTTEST"
#define BLE_NPI_ATKEY "AT+SPBLETEST"
#define BT_CUS_GETBTSTATUS_ATKEY "AT+GETBTSTATUS"

typedef int(*cmd_handle_t)(char *arg, char *res);

typedef struct {
    char *cmd;
    cmd_handle_t cmd_req;
    cmd_handle_t cmd_set;
} bt_eut_action_t;


#endif
