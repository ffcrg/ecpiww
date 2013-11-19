#ifndef WMBUS_H
#define WMBUS_H

//Modes
#define RADIOT2     4
#define RADIOS2     2

#define AES_KEYLENGHT_IN_BYTES     16
#define iM871AIdentifier           0x33

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

#define APL_VIF_ENERGY_WH                       0x00U
#define APL_VIF_VOLUME_M3                       0x10U
#ifndef APL_DIF_DATA_FIELD_SPECIAL_FILLER
#define APL_DIF_DATA_FIELD_SPECIAL_FILLER       0x2F
#define APL_DIF_DATA_FIELD_32_INT               0x04U
#define APL_DIF_DATA_FIELD_12_BCD               0x0EU
#endif

//Messages
#define WMBUS_MSG_HCI_MESSAGE_IND       0x00000004
#define WMBUS_MSGID_AES_DECRYPTIONERROR 0x27

#define WMBUS_MSGLENGTH_AESERROR        9
#define WMBUS_PAYLOADLENGTH_ENCRYPTED   30

//wMBus handling
unsigned long IMST_OpenDevice(char * device);
unsigned long IMST_CloseDevice(unsigned long handle);
int IMST_GetStickId(unsigned long handle, unsigned long *ID);

unsigned long  IMST_InitDevice(unsigned long handle,uint16_t infoflag);

unsigned long  IMST_SwitchMode(unsigned long handle,uint8_t Mode,uint16_t infoflag);
unsigned long  IMST_GetRadioMode(unsigned long handle, unsigned long *dwD,uint16_t infoflag);
unsigned long  IMST_IsNewData(unsigned long handle,uint16_t infoflag);
unsigned long  IMST_AddMeter(unsigned long handle, int slot, pecwMBUSMeter NewMeter );

unsigned long IMST_GetMeterList();
unsigned long IMST_GetMeterDataList();

#endif
