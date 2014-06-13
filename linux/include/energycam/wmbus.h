#ifndef WMBUS_H
#define WMBUS_H

#define MAXSLOT          16
#define COMMANDTIMEOUT  100
#define THREADWAITING   100
#define SLEEP100MS      (100*1000)

//offset in wM-Bus data
#define OFFSETPAYLOAD        3
#define OFFSETMANID          1
#define OFFSETMBUSID         3
#define OFFSETVERSION        7
#define OFFSETTYPE           8
#define OFFSETACCESSNUMBER  10
#define OFFSETSTATUS        11
#define OFFSETCONFIGWORD    12
#define OFFSETDECRYPTFILLER 14

//wM-Bus data defines
#define APL_VIF_UNITCODE                        0x78U
#define APL_VIF_ENERGY_WH                       0x00U
#define APL_VIFE_TRANS_CTR                      0x08U /*! E000 1000: Unique telegram identification (transmission counter) */
#define APL_VIF_VOLUME_M3                       0x10U
#define APL_VIF_SECOND_EXTENSION                0xFDU

#ifndef APL_DIF_DATA_FIELD_SPECIAL_FILLER
#define APL_DIF_DATAFIELD                       0x0FU
#define APL_DIF_DATA_FIELD_SPECIAL_FILLER       0x2FU
#define APL_DIF_DATA_FIELD_16_INT               0x02U
#define APL_DIF_DATA_FIELD_32_INT               0x04U
#define APL_DIF_DATA_FIELD_12_BCD               0x0EU

#define APL_DIF_FUNCTIONFIELD                   0x30U
#define APL_DIF_FUNC_INSTANEOUS                 0x00U
#define APL_DIF_FUNC_ERROR                      0x30U // Value during error state 

#define APL_VIF_EXTENSION_BIT                   0x80U
/*! E000 1000: Unique telegram identification (transmission counter) */
#define APL_VIFE_TRANS_CTR                      0x08U
#endif

//IMST Messages
#define WMBUS_MSG_HCI_MESSAGE_IND       0x00000004
#define WMBUS_MSGID_AES_DECRYPTIONERROR 0x27

#define WMBUS_MSGLENGTH_AESERROR         9
#define WMBUS_PAYLOADLENGTH_ENCRYPTED   30
#define WMBUS_PAYLOADLENGTH_DEFAULT     25



// AMBER Commands
#define CMD_DATA_REQ             0x00
#define CMD_DATARETRY_REQ        0x02
#define CMD_DATA_IND             0x03
#define CMD_SET_MODE_REQ         0x04
#define CMD_RESET_REQ            0x05
#define CMD_SET_CHANNEL_REQ      0x06
#define CMD_SET_REQ              0x09
#define CMD_GET_REQ              0x0A
#define CMD_SERIALNO_REQ         0x0B
#define CMD_FWV_REQ              0x0C
#define CMD_RSSI_REQ             0x0D
#define CMD_SETUARTSPEED_REQ     0x10
#define CMD_FACTORYRESET_REQ     0x11
#define CMD_DATA_PRELOAD_REQ     0x30
#define CMD_DATA_CLR_PRELOAD_REQ 0x31
#define CMD_SET_AES_KEY_REQ      0x50
#define CMD_CLR_AES_KEY_REQ      0x51
#define CMD_GET_AES_DEV_REQ      0x52

#define CNF                      0x80

//Settings
#define DATA_WITH_BLOCK1         0x20

//Amber commands

uint8_t CMD_SERIALNO_REQ_Arr[]              ={0xFF, 0x0B, 0x00, 0xF4}; //GetSerial
uint8_t SET_BLOCK1_ADD_ENABLE_REQ[]         ={0xFF, 0x09, 0x03, 0x30, 0x01, 0x00};

//CMD_SET_REQ
uint8_t SET_RSSI_ENABLE_REQ_Arr[]           ={0xFF, 0x09, 0x03, 0x45, 0x01, 0x01, 0xB0};
uint8_t SET_RSSI_DISABLE_REQ_Arr[]          ={0xFF, 0x09, 0x03, 0x45, 0x01, 0x00, 0xB1};
uint8_t SET_AES_ENABLE_REQ_Arr[]            ={0xFF, 0x09, 0x03, 0x0B, 0x01, 0x01, 0xFE};
uint8_t SET_AES_DISABLE_REQ_Arr[]           ={0xFF, 0x09, 0x03, 0x0B, 0x01, 0x00, 0xFF};

//CMD_GET_REQ
uint8_t CMD_GET_REQ_RSSI_ENABLED_Arr[]      ={0xFF, 0x0A, 0x02, 0x45, 0x01, 0xB3};
uint8_t CMD_GET_REQ_MODE_Arr[]              ={0xFF, 0x0A, 0x02, 0x46, 0x01, 0xB0};


uint8_t CMD_GET_AES_DEV_REQ_Arr0[]          ={0xFF, 0x52, 0x01, 0x00, 0xAC}; //Bank1...16 Devices
uint8_t CMD_GET_AES_DEV_REQ_Arr1[]          ={0xFF, 0x52, 0x01, 0x01, 0xAD}; //Bank2
uint8_t CMD_GET_AES_DEV_REQ_Arr2[]          ={0xFF, 0x52, 0x01, 0x02, 0xAE}; //Bank3
uint8_t CMD_GET_AES_DEV_REQ_Arr3[]          ={0xFF, 0x52, 0x01, 0x03, 0xAF}; //Bank4
uint8_t CMD_CLR_AES_KEY_REQ_Arr[]           ={0xFF, 0x51, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB};
uint8_t CMD_SET_MODE_REQ_ArrT2S2[]          ={0xFF, 0x04, 0x01, 0x00, 0x00};
uint8_t CMD_SET_MODE_REQ_ArrT2S2_PRESELECT[]={0xFF, 0x09, 0x03, 0x46, 0x01, 0x08, 0xBA};

uint8_t CMD_SET_AES_KEY_REQ_Arr[]           ={0xFF, 0x50, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif
