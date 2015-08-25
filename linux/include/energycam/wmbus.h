#ifndef WMBUS_H
#define WMBUS_H

#define MAXSLOT          16
#define COMMANDTIMEOUT  100
#define THREADWAITING   100
#define SLEEP100MS      (100*1000)
#define BUFFER_SIZE     1024


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


////////////////////////
// AMBER Commands
////////////////////////
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

#define RADIOT2_AMB              0x08
#define RADIOS2_AMB              0x03

#define AMBER_WMBUSDATA_INCLUDECMD 0
#define AMBER_WMBUSDATA_CMDLENGTH  2

//Amber commands
uint8_t CMD_SERIALNO_REQ_Arr[]              ={0xFF, 0x0B, 0x00, 0xF4}; //GetSerial

uint8_t SET_RSSI_ENABLE_REQ_Arr[]           ={0xFF, 0x09, 0x03, 0x45, 0x01, 0x01, 0xB0};
uint8_t SET_AES_ENABLE_REQ_Arr[]            ={0xFF, 0x09, 0x03, 0x0B, 0x01, 0x01, 0xFE};

//CMD_GET_REQ
uint8_t CMD_GET_REQ_MODE_Arr[]              ={0xFF, 0x0A, 0x02, 0x46, 0x01, 0xB0};
uint8_t CMD_GET_REQ_CMDFORMAT[]             ={0xFF, 0x0A, 0x02, 0x05, 0x01, 0xF3};


uint8_t CMD_CLR_AES_KEY_REQ_Arr[]           ={0xFF, 0x51, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB};
uint8_t CMD_SET_MODE_REQ_ArrT2S2[]          ={0xFF, 0x04, 0x01, 0x00, 0x00};
uint8_t CMD_SET_MODE_REQ_ArrT2S2_PRESELECT[]={0xFF, 0x09, 0x03, 0x46, 0x01, 0x08, 0xBA};

uint8_t CMD_SET_AES_KEY_REQ_Arr[]           ={0xFF, 0x50, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


////////////////////////
// IMST Commands
////////////////////////

#define MAX_COMMAND_RESPONSE               100
#define MAX_HCI_LENGTH                     100

//HCI Message
#define LENGTH_HCI_HEADER                   0x04
#define SOF                                 0x00
#define CF_EID                              0x01
#define MID                                 0x02
#define LENGTH                              0x03
#define PAYLOAD                             0x04




#define START_OF_FRAME                      0xA5
#define CF_TIMESTAMP(x)                     (x & 0x20)
#define CF_RSSI(x)                          (x & 0x40)
#define CF_CRC16(x)                         (x & 0x80)
#define CF_ENDPOINTID(x)                    (x & 0x0F)




//List of Endpoint Identifier
#define DEVMGMT_ID                          0x01
#define RADIOLINK_ID                        0x02
#define RADIOLINKTEST_ID                    0x03
#define HWTEST_ID                           0x04


//Device Management Message Identifier
#define  DEVMGMT_MSG_PING_REQ               0x01
#define  DEVMGMT_MSG_PING_RSP               0x02
#define  DEVMGMT_MSG_SET_CONFIG_REQ         0x03
#define  DEVMGMT_MSG_SET_CONFIG_RSP         0x04
#define  DEVMGMT_MSG_GET_CONFIG_REQ         0x05
#define  DEVMGMT_MSG_GET_CONFIG_RSP         0x06
#define  DEVMGMT_MSG_RESET_REQ              0x07
#define  DEVMGMT_MSG_RESET_RSP              0x08
#define  DEVMGMT_MSG_FACTORY_RESET_REQ      0x09
#define  DEVMGMT_MSG_FACTORY_RESET_RSP      0x0A
#define  DEVMGMT_MSG_GET_OPMODE_REQ         0x0B
#define  DEVMGMT_MSG_GET_OPMODE_RSP         0x0C
#define  DEVMGMT_MSG_SET_OPMODE_REQ         0x0D
#define  DEVMGMT_MSG_SET_OPMODE_RSP         0x0E
#define  DEVMGMT_MSG_GET_DEVICEINFO_REQ     0x0F
#define  DEVMGMT_MSG_GET_DEVICEINFO_RSP     0x10
#define  DEVMGMT_MSG_GET_SYSSTATUS_REQ      0x11
#define  DEVMGMT_MSG_GET_SYSSTATUS_RSP      0x12
#define  DEVMGMT_MSG_GET_FWINFO_REQ         0x13
#define  DEVMGMT_MSG_GET_FWINFO_RSP         0x14
#define  DEVMGMT_MSG_GET_RTC_REQ            0x19
#define  DEVMGMT_MSG_GET_RTC_RSP            0x1A
#define  DEVMGMT_MSG_SET_RTC_REQ            0x1B
#define  DEVMGMT_MSG_SET_RTC_RSP            0x1C
#define  DEVMGMT_MSG_ENTER_LPM_REQ          0x1D
#define  DEVMGMT_MSG_ENTER_LPM_RSP          0x1E
#define  DEVMGMT_MSG_SET_AES_ENCKEY_REQ     0x21
#define  DEVMGMT_MSG_SET_AES_ENCKEY_RSP     0x22
#define  DEVMGMT_MSG_ENABLE_AES_ENCKEY_REQ  0x23
#define  DEVMGMT_MSG_ENABLE_AES_ENCKEY_RSP  0x24
#define  DEVMGMT_MSG_SET_AES_DECKEY_REQ     0x25
#define  DEVMGMT_MSG_SET_AES_DECKEY_RSP     0x26
#define  DEVMGMT_MSG_AES_DEC_ERROR_IND      0x27


//Radio Link Message Identifier
#define RADIOLINK_MSG_WMBUSMSG_REQ          0x01
#define RADIOLINK_MSG_WMBUSMSG_RSP          0x02
#define RADIOLINK_MSG_WMBUSMSG_IND          0x03
#define RADIOLINK_MSG_DATA_REQ              0x04
#define RADIOLINK_MSG_DATA_RSP              0x05

#endif
