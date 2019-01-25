#ifndef __RAW_ADDRESS_H__
#define __RAW_ADDRESS_H__

#include <sys/types.h>

typedef struct {
    uint8_t address[6];
} __attribute__((packed))RawAddress;

typedef struct {
    uint8_t address[6];
} __attribute__((packed))bt_bdaddr_t;

#endif
