#ifndef ECPIWW_H
#define ECPIWW_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXMETER 16 // same value as define MAXSLOT in wmbus.c

#define FASTFORWARD             0x18C4
#define AES_KEYLENGHT_IN_BYTES  16

#pragma pack(push,1) 
typedef struct _WMBUS_METER {
    uint16_t  manufacturerID;   
    uint32_t  ident; 
    uint8_t   version;        
    uint8_t   type;        
    uint8_t   key[AES_KEYLENGHT_IN_BYTES];       
} ecwMBUSMeter, *pecwMBUSMeter;
#pragma pack(pop)

#define RFDATA_ENCRYPTED 0x01
#define RFDATA_NOT_ENCRYPTED 0x02

typedef struct _RF_DATA {
    uint32_t time;      // UNIX epoch time
    uint32_t value;     // OCR value in integer
    int8_t   exp;       // OCR value exponent
    uint8_t  accNo;     // RF packet access number, should be ascending and wraps
    int8_t   rssiDBm;   // rssi in dbm
    uint8_t  pktInfo;   // EC packet info
} ecMBUSData, *psecMBUSData;

#endif
