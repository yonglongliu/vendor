/*
 * Copyright (C) 2017 spreadtrum.com
 */

#pragma once

__BEGIN_DECLS


/*
*Return value: zero is ok,rpmb key is writed
*/
int is_wr_rpmb_key(void);

/*
*@key_byte   key
*@key_len  key lenth
*Return value: zero is ok
*/
int wr_rpmb_key(uint8_t *key_byte, uint8_t key_len);

/*
*Return value: zero is ok, storageproxyd is running
*/
int run_storageproxyd(void);

__END_DECLS
