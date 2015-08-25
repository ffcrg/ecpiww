#ifndef WMBUSEXT_H
#define WMBUSEXT_H

//Modes
#define RADIOT2     4
#define RADIOS2     2

#define AES_KEYLENGHT_IN_BYTES     16
#define iM871AIdentifier           0x33
#define iAMB8465Identifier         0x27
#define iAMB8665Identifier         0x53

//Returns
#define APIERROR   (-1)
#define APIOK      0

//METERS
#define METER_ELECTRICITY          0x02
#define METER_GAS                  0x03
#define METER_WATER                0x07

#define PACKET_WAS_ENCRYPTED       0x01
#define PACKET_WAS_NOT_ENCRYPTED   0x02
#define PACKET_IS_ENCRYPTED        0x04
#define PACKET_DECRYPTIONERROR     0x08

//wMBus handling
int wMBus_OpenDevice(char* device, uint16_t stick);
int wMBus_CloseDevice( int handle, uint16_t stick);
int           wMBus_GetStickId(  int handle, uint16_t stick, uint8_t* ID, uint16_t infoflag);

unsigned long wMBus_InitDevice(  int handle, uint16_t stick, uint16_t infoflag);

unsigned long wMBus_SwitchMode(  int handle, uint16_t stick, uint8_t Mode, uint16_t infoflag);
unsigned long wMBus_GetRadioMode(int handle, uint16_t stick, uint8_t* Mode, uint16_t infoflag);
unsigned long wMBus_AddMeter(    int handle, uint16_t stick, uint8_t slot, pecwMBUSMeter NewMeter, uint16_t infoflag);
int           wMBus_RemoveMeter(  int Index);
unsigned long wMBus_GetData4Meter(int Index, psecMBUSData data);

unsigned long wMBus_GetMeterList();
unsigned long wMBus_GetMeterDataList();

#endif
