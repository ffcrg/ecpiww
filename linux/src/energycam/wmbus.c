#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <dlfcn.h>
#include <energycam/ecpiww.h>
#include <extern/libwmbus.h>
#include <energycam/wmbus.h>

#define MAXSLOT 16
unsigned long   dwFrameCounter;
unsigned long   dwMeter=0;
unsigned long   MeterPresent=0;
unsigned long   MeterHasData=0;
bool            bCallbackRegistered=false;
unsigned long   myhandle=0;
uint16_t		myInfoFlag=SILENTMODE;

static ecwMBUSMeter MeterAddr[MAXSLOT];
static ecMBUSData MeterData[MAXSLOT];

void *libHandle;

void *loadLibWMBusHCI() {
    char* error;
    void* libHandle;

    // get library handle
    libHandle = dlopen(LIB_WMBUSHCI, RTLD_NOW);

    if(!libHandle) {
        printf("Error while loading: %s\n", dlerror());
        return NULL;
    }

    // loading library function pointers
    WMBus_OpenDevice                = (open_t)                      dlsym(libHandle, "WMBus_OpenDevice");
    WMBus_CloseDevice               = (close_t)                     dlsym(libHandle, "WMBus_CloseDevice");
    WMBus_GetDeviceInfo             = (getdevinfo_t)                dlsym(libHandle, "WMBus_GetDeviceInfo");
    WMBus_GetLastError              = (getlasterror_t)              dlsym(libHandle, "WMBus_GetLastError");
    WMBus_GetErrorString            = (geterrorstring_t)            dlsym(libHandle, "WMBus_GetErrorString");
    WMBus_GetSystemStatus           = (getsystemstatus_t)           dlsym(libHandle, "WMBus_GetSystemStatus");
    WMBus_GetDeviceConfig           = (deviceconfig_t)              dlsym(libHandle, "WMBus_GetDeviceConfig");
    WMBus_SetDeviceConfig           = (setdeviceconfig_t)           dlsym(libHandle, "WMBus_SetDeviceConfig");
    WMBus_GetHCIMessage             = (gethcimessage_t)             dlsym(libHandle, "WMBus_GetHCIMessage");
    WMBus_ConfigureAESDecryptionKey = (configureAESDecryptionKey_t) dlsym(libHandle, "WMBus_ConfigureAESDecryptionKey");
    WMBus_RegisterMsgHandler        = (registermsghandler_t)        dlsym(libHandle, "WMBus_RegisterMsgHandler");

    if((error = dlerror()) != NULL) {
        printf("Error linking with dlsym: %s\n", error);
        return NULL;
    }
    return libHandle;
}

void unloadLibWMBusHCI(void * libHandle) {
    if(NULL != libHandle)
       dlclose(libHandle);
}

int IMST_GetStickId(unsigned long handle,unsigned long *ID) {
    unsigned long dwReturn=APIERROR;
    unsigned char *pData;
    uint16_t Datasize=BUFFER_SIZE;
    pData = (unsigned char *) malloc(Datasize);

    memset(pData,0,sizeof(unsigned char)*Datasize);

    if(WMBus_GetDeviceInfo(handle, pData, Datasize)) {
        *ID = *(pData + 1);
        dwReturn = APIOK;
    } else
      printf("GetDeviceInfo returns 0 \n");

    if(pData) free(pData);
        return dwReturn;
}

unsigned long IMST_GetLastError(unsigned long handle) {
    unsigned long dwReturn=0;

    dwReturn = WMBus_GetLastError(handle);
    char *pData;
    pData = (char *) malloc(128);
    memset(pData, 0, sizeof(unsigned char)*128);
    WMBus_GetErrorString(dwReturn, pData, 128);
    printf("GetLastError %ld %s\n", dwReturn, pData);
    free(pData);
    return dwReturn;
}

unsigned long  IMST_SwitchMode(unsigned long handle,uint8_t Mode,uint16_t infoflag) {
    unsigned long dwReturn=0;
    uint16_t Datasize=BUFFER_SIZE;
    unsigned char *pData;
    pData = (unsigned char *) malloc(Datasize);
    memset(pData,0,sizeof(unsigned char)*Datasize);

    if((Mode == RADIOT2) || (Mode || RADIOS2)) {

        //set to S2/T2 Mode
        *(pData +0) = 3;
        *(pData +1) = 0; //Other
        *(pData +2) = Mode; //S2=2   ; T2=4
        *(pData +3) = 0xB0; //IIFlag2 Auto RSSI, Auto Rx Timestamp, RTC Control
        *(pData +4) = 1; //RSSI
        *(pData +5) = 1; //Timestamp
        *(pData +6) = 1; //RTC Control

        if((dwReturn = WMBus_SetDeviceConfig(handle,pData,7,false))>0) {
            if (infoflag > SILENTMODE) printf("IMST_SwitchMode to  %s \n",((Mode==RADIOT2) ?"T2" : "S2"));
        }
    }
    free(pData);
    return dwReturn;
}


unsigned long  IMST_GetRadioMode(unsigned long handle,unsigned long *dwD,uint16_t infoflag) {
    unsigned long  dwReturn=APIERROR;
    uint16_t Datasize=BUFFER_SIZE;
    unsigned char *pData;
    pData = (unsigned char *) malloc(Datasize);
    if(NULL == pData)
        return 0;
    if(0 == handle)
        return 0;
    memset(pData,0,sizeof(unsigned char)*Datasize);

    if(WMBus_GetDeviceConfig(handle,pData,Datasize)) {

		if (infoflag > SILENTMODE) {
			switch(*(pData +3)) {
			case RADIOT2 : printf("T2 Mode \n");break;
			case RADIOS2 : printf("S2 Mode \n");break;
			default : printf("undefined \n");break;
			}
		}

        dwReturn=APIOK;
        *dwD = *(pData +3);
    }
    free (pData);
    return dwReturn;
}

unsigned long  IMST_IsNewData(unsigned long handle,uint16_t infoflag) {
    unsigned long  dwReturn=0;
    unsigned long  dwNewData=0;
    uint16_t Datasize=BUFFER_SIZE;
    unsigned char *pData;
    pData = (unsigned char *) malloc(Datasize);
    if(NULL == pData)
        return 0;
    if(0 == handle)
        return 0;
    memset(pData,0,sizeof(unsigned char)*Datasize);

    if(WMBus_GetSystemStatus(handle,pData,Datasize)) {
        dwNewData = *((unsigned long*) (pData+23));
        if(dwFrameCounter == 0)
            dwReturn = 0; //start condition
        else
            dwReturn = dwNewData - dwFrameCounter;
        if (infoflag > SILENTMODE) printf("IMST_IsNewData %ld Bytes -> new %ld \n", dwNewData, dwReturn);
        dwFrameCounter = dwNewData;
    } else {
        if (infoflag > SILENTMODE) printf("WMBus_GetSystemStatus returns 0\n");
	}

    free (pData);
    return dwReturn;
}

void GetDataFromStick(unsigned long handle,uint16_t infoflag);

void IMST_Callback(UINT32 msg, UINT32 param) {
    if(msg == WMBUS_MSG_HCI_MESSAGE_IND) {
        //IMST_IsNewData(myhandle);
        GetDataFromStick(myhandle,myInfoFlag);
    }
}

unsigned long  IMST_InitDevice(unsigned long handle,uint16_t infoflag) {
    if(!bCallbackRegistered) {
        bCallbackRegistered=true;
        myhandle=handle;
        WMBus_RegisterMsgHandler(&IMST_Callback);
    }

    myInfoFlag = infoflag;
    //clear Array
    memset(MeterAddr, 0, MAXSLOT*sizeof(ecwMBUSMeter));
    memset(MeterData, 0, MAXSLOT*sizeof(ecMBUSData));

    unsigned char Filter[8];
    unsigned char Key[16];
    int iX;
    for (iX=0;iX<AES_KEYLENGHT_IN_BYTES;iX++)
        Key[iX] = iX;

    Filter[0] = 0x25B3>>8;
    Filter[1] = 0xB3;
    Filter[2] = 0x12;
    Filter[3] = 0x34>>16;
    Filter[4] = 0x56>>8;
    Filter[5] = 0x70+iX;
    Filter[6] = 0x01;
    Filter[7] = 0x02;

    //clear all Key Slots
    for (iX=0;iX<16;iX++) {
        Filter[5] = 0x70+iX;
        WMBus_ConfigureAESDecryptionKey(handle,iX,Filter,Key);
    }
    return 1;
}

unsigned long  IMST_AddMeter(unsigned long handle,int slot,pecwMBUSMeter NewMeter ) {
    int i;
    bool exist=false;
    if(slot<MAXSLOT) {
        dwMeter = slot;
        for( i=0; i<=MAXSLOT; i++) {
            if(MeterAddr[i].manufacturerID == NewMeter->manufacturerID &&
               MeterAddr[i].ident          == NewMeter->ident          &&
               MeterAddr[i].version        == NewMeter->version        &&
               MeterAddr[i].type           == NewMeter->type) {
            exist=true;
            }
        }

        if(!exist) {//if Meter does not exist...Add new one
            unsigned char Filter[8];

            MeterPresent |= 0x01 << slot;
            MeterAddr[dwMeter].manufacturerID   = NewMeter->manufacturerID;
            MeterAddr[dwMeter].ident            = NewMeter->ident;
            MeterAddr[dwMeter].version          = NewMeter->version;
            MeterAddr[dwMeter].type             = NewMeter->type;
            memcpy(MeterAddr[dwMeter].key,NewMeter->key,AES_KEYLENGHT_IN_BYTES);

            Filter[0] = NewMeter->manufacturerID;
            Filter[1] = NewMeter->manufacturerID>>8;
            Filter[2] = NewMeter->ident;
            Filter[3] = NewMeter->ident>>8;
            Filter[4] = NewMeter->ident>>16;
            Filter[5] = NewMeter->ident>>24;
            Filter[6] = NewMeter->version;
            Filter[7] = NewMeter->type;

            WMBus_ConfigureAESDecryptionKey(handle, slot, Filter,(unsigned char*) MeterAddr[dwMeter].key);
            dwMeter++;
            exist=false;
            return 1;
        } else {
            return 0;
        }
    } else
        printf("All slots full \n");

    return 0;
}

int IMST_IsInArray(ecwMBUSMeter p,ecwMBUSMeter MeterData[]) {
    int i;
    for(i=0;i<MAXSLOT;i++) {
        if(MeterData[i].manufacturerID == p.manufacturerID &&
           MeterData[i].ident          == p.ident          &&
           MeterData[i].version        == p.version        &&
           MeterData[i].type           == p.type)
        break;
    }
    return i;
}

int IMST_RemoveMeter(int Index) {
    MeterPresent &= ~(0x01<<Index);
    memset(&MeterAddr[Index],0,sizeof(ecwMBUSMeter));
    return 0;
}

unsigned long IMST_GetMeterList() {
    return MeterPresent;
}

unsigned long IMST_GetMeterDataList() {
    return MeterHasData;
}

bool saBCD12ToUINT32(uint8_t* pBcd12, uint8_t size, uint32_t* pV) {
    if(NULL == pV)
        return false;

    uint32_t v = 0;
    uint32_t base = 1;
    int i;
    *pV = 0;

    for(i = 0; i < size; i++) {
        unsigned char c;

        c = (*(pBcd12+size-i-1) & 0x0F);
        if (c > 9)
            return false;
        v += c * base;
        base *= 10;

        c = ((*(pBcd12+size-i-1) & 0xF0)>>4);
        if (c > 9)
            return false;
        v += c * base;
        base *= 10;
    }
    *pV = v;
    return true;
}

void GetDataFromStick(unsigned long handle,uint16_t infoflag) {
    unsigned long dwReturn;
    int iX;
    int PayLoadLength;
    unsigned long TimeStamp=0;
    int8_t RSSI=0;
    bool Decrypted=false;
    int Offset=0;
    short sSize = 1024;
    unsigned char *buffer;

    buffer = (unsigned char *) malloc(sSize);
    memset(buffer, 0, sizeof(unsigned char)*sSize);
    dwReturn = WMBus_GetHCIMessage(handle, buffer, sSize);

    if(dwReturn) {
        PayLoadLength = *(buffer+2);
        if (infoflag > SILENTMODE) printf(" PayloadL %d ", PayLoadLength);

        if(*(buffer) & 0x20) { //If TimeStamp attached
            TimeStamp = *( (unsigned long*) (buffer+3+PayLoadLength));
            if (infoflag > SILENTMODE) printf("Timestamp=0x%08X ", (unsigned int)TimeStamp);
        }
        if(*(buffer) & 0x40) { //If RSSI attached
            uint8_t RSSIfromBuf = *(buffer+7+PayLoadLength);
            double b = -100.0 - (4000.0 / 150.0);
            double m = 80.0 / 150.0;
            RSSI=(int8_t)(m * (double)RSSIfromBuf + b);
            if (infoflag > SILENTMODE) printf("RSSI=%i",RSSI);
        }
        if (infoflag > SILENTMODE) printf("\n");

        /*if (infoflag > SILENTMODE){
          for(iX=0; iX<24; iX++)
             printf("%02X ",*(buffer+iX));
          printf("\n");
        }*/

        ecMBUSData   RFData;    //struct to store value + rssi + timestamp
        ecwMBUSMeter RFSource;  //struct to store Source Address

        Decrypted=true;
        for(iX=1; iX<8; iX++) {
            if(*(buffer+2+PayLoadLength-iX) != APL_DIF_DATA_FIELD_SPECIAL_FILLER)
                Decrypted=false;
        }

        memset(&RFData,   0, sizeof(ecMBUSData));
        memset(&RFSource, 0, sizeof(ecwMBUSMeter));

        RFData.time=TimeStamp;
        RFData.rssiDBm= RSSI;
        RFData.accNo = *(buffer+13); //Access number

        //Status = *(buffer+14)
        unsigned short CW = *((unsigned short*)(buffer+15));


        uint8_t DIF=0;
        uint8_t VIF=0;

        //RFData.pktInfo = ((CW & 0x0500) == 0x0500) ? PACKET_WAS_ENCRYPTED : PACKET_WAS_NOT_ENCRYPTED;
        //The stick does the decryption and removes the flag
        RFData.pktInfo = PACKET_WAS_NOT_ENCRYPTED;

        Offset = 17;  //start to check DIF on Offset 17
        DIF = *(buffer+Offset++);
        while(DIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER) {
            DIF = *(buffer+Offset++);
        }
        VIF = *(buffer+Offset++);
        while(VIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER) {
            VIF = *(buffer+Offset++);
        }
        RFData.exp = 0;
        RFData.value =0;

        //current PayLoadLength
        //20 is not encrypted
        //30 encrypted

        if((PayLoadLength < WMBUS_PAYLOADLENGTH_ENCRYPTED) || Decrypted ) {
            if((WMBUS_MSGLENGTH_AESERROR == PayLoadLength) && (*(buffer+1) == WMBUS_MSGID_AES_DECRYPTIONERROR)) {
                RFData.pktInfo=PACKET_DECRYPTIONERROR;
            } else {
                if(PayLoadLength > WMBUS_PAYLOADLENGTH_DEFAULT)
                RFData.pktInfo = PACKET_WAS_ENCRYPTED;

                if(APL_VIF_ENERGY_WH == (VIF & 0x78)) {
                    RFData.exp = (VIF&0x07)-3;

                    if(APL_DIF_DATA_FIELD_32_INT == DIF)
                            RFData.value = *((unsigned long *)(buffer+Offset));
                    if(APL_DIF_DATA_FIELD_12_BCD == DIF) {
                        uint8_t bcdbytes[12/2];
                        memcpy( bcdbytes,buffer+Offset,12/2);
                        saBCD12ToUINT32( bcdbytes,12/2, &RFData.value);
                    }
                }
                if(APL_VIF_VOLUME_M3 == (VIF & 0x78)) {
                    RFData.exp = (VIF&0x07)-6;
                    if(APL_DIF_DATA_FIELD_32_INT == DIF)
                            RFData.value = *((unsigned long *)(buffer+Offset));
                    if(APL_DIF_DATA_FIELD_12_BCD == DIF) {
                        uint8_t bcdbytes[12/2];
                        memcpy(bcdbytes,buffer+Offset, 12/2);
                        saBCD12ToUINT32(bcdbytes, 12/2, &RFData.value);
                    }
                }
            }
        } else
            RFData.pktInfo = PACKET_IS_ENCRYPTED;

        RFSource.manufacturerID = (*(buffer+ 5) << 8) +  *(buffer+4);                                   //dez
        RFSource.ident          = (*(buffer+ 9) <<24) + (*(buffer+8)<<16)+(*(buffer+7)<<8)+*(buffer+6); //dez
        RFSource.version        =  *(buffer+10);
        RFSource.type           =  *(buffer+11);

        if (infoflag > SILENTMODE) printf("Meter  %04X %08X %02X %02X %d (exp) %d \n", RFSource.manufacturerID, RFSource.ident, RFSource.version, RFSource.type, RFData.value, RFData.exp);

        int MeterIndex;
        MeterIndex = IMST_IsInArray(RFSource,MeterAddr);

        if(MeterIndex < MAXSLOT) {
            //If decryption doesn't work 2 Messages are sent - keep Decryption Error Status
            if(PACKET_DECRYPTIONERROR == MeterData[MeterIndex].pktInfo)
                RFData.pktInfo=PACKET_DECRYPTIONERROR;
            memcpy(&MeterData[MeterIndex],&RFData,sizeof(ecMBUSData));
            MeterHasData=MeterHasData | (0x01<<MeterIndex); //set bit which MeterData was recieved
        }
    }
}

unsigned long IMST_OpenDevice(char * device) {
    //load external LIB
    libHandle = loadLibWMBusHCI();
    if(0 == libHandle)
         ErrorAndExit("Library not found\n");
   return WMBus_OpenDevice(device);
}
unsigned long IMST_CloseDevice(unsigned long handle) {
   WMBus_CloseDevice(handle);
   unloadLibWMBusHCI(libHandle);
   return 1;
}

unsigned long IMST_GetData4Meter(unsigned long handle,int Index, psecMBUSData data) {
    unsigned long dwReturn;

    if (Index > MAXSLOT)
        return 0;

    dwReturn = MeterHasData & (0x01<<Index); //get Bit out of mask
    if(NULL != data) memcpy(data, &MeterData[Index], sizeof(ecMBUSData));
    memset(&MeterData[Index], 0, sizeof(ecMBUSData));
    MeterHasData &= ~(0x01<<Index); //clear Bit
    return dwReturn;
}
