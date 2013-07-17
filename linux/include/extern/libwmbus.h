#ifndef _LIBWMBUS_H_
#define _LIBWMBUS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>

#ifndef MAX 
    #define MAX(x, y) ((x > y) ? x : y)
#endif

#ifndef MIN 
    #define MIN(x, y) ((x > y) ? y : x)
#endif

#define BUFFER_SIZE 1024

#define LIB_WMBUSHCI "./libwmbus.so"

typedef unsigned char UINT8;
typedef unsigned int UINT32;

//Messages
#define WMBUS_MSG_HCI_MESSAGE_IND 0x00000004

/*
 *  library function ptointers
 */

typedef int (*open_t)(const char *);
typedef int (*close_t)(int);
typedef int (*getdevinfo_t)(int, UINT8[], int);
typedef int (*getlasterror_t)(int);
typedef int (*geterrorstring_t)(int, char[], int);
typedef int (*deviceconfig_t)(int, UINT8[], int);
typedef int (*setdeviceconfig_t)(int, UINT8[], int, UINT8);
typedef int (*gethcimessage_t)(int, UINT8[], int);
typedef int (*getsystemstatus_t)(int, UINT8[], int);
typedef int (*configureAESDecryptionKey_t)(int, UINT8,UINT8[],UINT8[]);
typedef void (*TWBus_CbMsgHandler)(UINT32 msg, UINT32 param);
typedef int (*registermsghandler_t)(TWBus_CbMsgHandler cbMsgHandler);

open_t               WMBus_OpenDevice;
close_t              WMBus_CloseDevice;
getdevinfo_t         WMBus_GetDeviceInfo;
getlasterror_t       WMBus_GetLastError;
geterrorstring_t     WMBus_GetErrorString;
deviceconfig_t       WMBus_GetDeviceConfig;
setdeviceconfig_t    WMBus_SetDeviceConfig;
gethcimessage_t      WMBus_GetHCIMessage;
getsystemstatus_t    WMBus_GetSystemStatus;
registermsghandler_t WMBus_RegisterMsgHandler;

configureAESDecryptionKey_t WMBus_ConfigureAESDecryptionKey;

#endif

