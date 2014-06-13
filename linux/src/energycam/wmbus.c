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
#include <pthread.h>
#include <energycam/ecpiww.h>
#include <extern/libwmbus.h>
#include <energycam/wmbus.h>
#include <energycam/wmbusext.h>

unsigned long   dwFrameCounter;
unsigned long   dwMeter=0;
unsigned long   MeterPresent=0;
unsigned long   MeterHasData=0;
bool            bCallbackRegistered=false;
unsigned long   myhandle=0;
uint16_t        myInfoFlag=SILENTMODE;
uint16_t        myStickID = 0;

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

void GetDataFromStick(unsigned long handle,uint16_t stick,uint16_t infoflag);
//////////////////////////////////////////////////////////////////////////////////////

#pragma region "AMBERStick"
pthread_t   ThreadID;
pthread_mutex_t lockAPI= PTHREAD_MUTEX_INITIALIZER;
int AmberCom=-1;

//connect to AMBER Stick
int AMBER_OpenDevice(char * comport, uint32_t BaudRate) {
    // Declare variables and structures
    struct termios tios;
    speed_t speed;
    int serial;

    printf("open port (%s) with %d baud\n", comport, BaudRate);

    serial = open(comport, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
    if (serial == -1) {
        //retry
        usleep(10*SLEEP100MS);
        serial = open(comport, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL);
        if (serial == -1) {
            //retry
            printf("Error opening port (%s)\n", comport);
            return -1;
        }
    }

    memset(&tios, 0, sizeof(struct termios));

    switch (BaudRate) {
        case 9600:   speed = B9600;  break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200;break;
        default:     speed = B9600;
    }

    /* Set the baud rate */
    if ((cfsetispeed(&tios, speed) < 0) || (cfsetospeed(&tios, speed) < 0)) {
        close(serial);
        return -1;
    }

    //One Stopbit No Parity
    tios.c_cflag |= (CREAD | CLOCAL);
    tios.c_cflag &= ~CSIZE;
    tios.c_cflag |= CS8;
    tios.c_cflag &=~ CSTOPB;
    tios.c_cflag &=~ PARENB;

    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tios.c_iflag &= ~INPCK;

    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    tios.c_oflag &=~ OPOST;
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(serial, TCSANOW, &tios) < 0) {
        close(serial);
        return -1;
    }
    return serial;
}

//send Commands to AMBER stick
bool AMBERCommand(int serial, uint8_t command[], uint8_t  *pData,bool bReq, short sWriteSize, short Datasize, uint16_t infoflag) {
    ssize_t bytes_read=0;
    ssize_t bytes_written=0;
    bool bSuccess=false;
    int i;

    uint8_t *pBuffer;
    pBuffer = (uint8_t *) malloc(BUFFER_SIZE);
    memset(pBuffer, 0, sizeof(uint8_t)*BUFFER_SIZE);

    pthread_mutex_lock(&lockAPI);

    bytes_written = write(serial, command, sWriteSize);
    if(bytes_written <= 0) {
        bSuccess=false;
    }
    else {
        if(bReq) { //Validation (only in REQ commands)
            pBuffer[3]=0xFF;//Write sample value
            usleep(5*SLEEP100MS); //time to answer
            bytes_read = read(serial, pBuffer, BUFFER_SIZE);
            if (bytes_read) {
                if((command[1]+CNF)==pBuffer[1]) {
                    switch (command[1]) {
                        case CMD_SERIALNO_REQ:
                            if(infoflag>=SHOWALLDETAILS) {
                                printf("CMD_SERIALNO_REQ \nSerial: ");
                                for(i=3;(i>=3 && i<7);i++)
                                    printf("%02X", pBuffer[i]);
                                printf("\n");
                            }
                            bSuccess=true;
                        break;

                        case CMD_GET_AES_DEV_REQ:
                            if(infoflag>=SHOWALLDETAILS) {
                                printf("CMD_GET_AES_DEV_REQ \nRegistered devices:\n");
                                printf("Bank %X \n", command[3]);

                                for(i=0; i<(bytes_read-4); i=i+8) { // bytes_read-4: 3 preceding bytes of command and CRC Checksum are not displayed
                                    if((pBuffer[3+i] | ( pBuffer[4+i] << 8))!=0) {
                                        printf("ManID %02X %02X \n",            pBuffer[4+i], pBuffer[3+i]);
                                        printf("Ident %02X %02X %02X %02X \n",  pBuffer[8+i], pBuffer[7+i], pBuffer[6+i], pBuffer[5+i]);
                                        printf("Version %02X \n",               pBuffer[9+i]);
                                        printf("Type %02X \n ",                 pBuffer[10+i]);
                                        printf("\n");
                                    }
                                }
                            }
                            bSuccess=true;
                        break;

                        case CMD_SET_AES_KEY_REQ:
                            bSuccess=false;
                            if(pBuffer[3]==0x00){
                                if(infoflag>=SHOWALLDETAILS)
                                    printf("CMD_SET_AES_KEY_REQ...OK\n");
                                bSuccess=true; //success
                                break;
                            }
                            else if(pBuffer[3]==0x01) printf("Error...Verification in memory failed\n");       //verification in memory failed
                            else if(pBuffer[3]==0x02) printf("Error...No more memory available...Error\n");    //no more memory available
                        break;

                        case CMD_CLR_AES_KEY_REQ:
                            bSuccess=false;
                            if(infoflag>=SHOWALLDETAILS) printf("CMD_CLR_AES_KEY_REQ");
                            if(pBuffer[3]==0x00){
                                if(infoflag>=SHOWALLDETAILS)
                                    printf("...OK ");
                                bSuccess=true;
                                break;
                            }
                            else if(pBuffer[3]==0x01) printf("\nError...Verification in memory failed ");
                            else if(pBuffer[3]==0x02) if(infoflag>0) printf("...OK\nMeter to remove not in list ");
                        break;

                        case CMD_SET_MODE_REQ:
                            if(infoflag>=SHOWALLDETAILS) printf("CMD_SET_MODE_REQ");
                            if(pBuffer[3]==0){ //status 0x00 -> success
                                 if(infoflag>=SHOWALLDETAILS)
                                     printf("...OK");
                                  bSuccess=true;
                            }
                            else {
                                 printf("Error...setting S2/T2 mode failed\n");
                                 bSuccess=false ;
                            }
                        break;

                        case CMD_GET_REQ:
                            if(infoflag>=SHOWALLDETAILS) printf("CMD_GET_REQ\n");
                            bSuccess=true;
                        break;

                        case CMD_SET_REQ:
                            bSuccess=false;
                            if(pBuffer[3]==0x00){
                                bSuccess=true;
                                if(infoflag>=SHOWALLDETAILS)
                                    printf("\nCMD_SET_REQ...OK ");
                                break;
                            }
                            else if(pBuffer[3]==0x01) printf("Error...Verification failed failed:\n");
                            else if(pBuffer[3]==0x02) printf("Error...invalid memory position or invalid number of bytes to be written (write access to unauthorised location)\n");
                        break;

                        default:
                            if(infoflag>=SHOWDETAILS) printf("Error...undefined command\n");
                        break;
                    }  //case
                }
                else
                    {  //  if((command[1]
                    bSuccess=false;
                }
            }//bytes read
        }
    }

    if(pData) {
        memcpy(pData, pBuffer, min(Datasize, BUFFER_SIZE));
    }
    pthread_mutex_unlock(&lockAPI);
    if(pBuffer) free(pBuffer);
    return bSuccess;
}

//disconnect AMBER device
bool AMBER_CloseDevice(int serial) {
    // Close serial port
    printf("Closing serial port ");
    if (close(serial) != 0) {
        printf( "Error\n");
        return false;
    }
    printf("OK\n");
    return true;
}

//calc CRC of AMBER Commands
uint8_t CRC_XOR(uint8_t *buffer, uint16_t buffer_length) {
    uint8_t crc=*buffer;
    int i=1;

    while(i<min(BUFFER_SIZE, buffer_length)) {
        crc^=*(buffer+i);
        i++;
    }
    return crc;
}

//change RF mode
bool AMBER_SwitchRFMode(int serial, uint8_t Mode, uint16_t infoflag) {
    bool bSuccess = false;
    short sWriteSize = (sizeof(CMD_SET_MODE_REQ_ArrT2S2_PRESELECT)/sizeof(uint8_t));

    CMD_SET_MODE_REQ_ArrT2S2_PRESELECT[5]= (Mode == RADIOS2) ? 0x03 : 0x08; //Switch to S2 : T2
    CMD_SET_MODE_REQ_ArrT2S2_PRESELECT[sWriteSize-1]=CRC_XOR(CMD_SET_MODE_REQ_ArrT2S2_PRESELECT, sWriteSize-1);

    if(AMBERCommand(serial, CMD_SET_MODE_REQ_ArrT2S2_PRESELECT, NULL, true, sWriteSize, BUFFER_SIZE, infoflag))
        bSuccess=true;
    else
        printf( "Error...writing command to stick failed\n");

    return bSuccess;
}

//read data from stick
bool AMBER_ReadFrameFromStick(int serial, uint8_t *pbuffer, int sSize, short* sSize_frame, uint16_t infoflag) {
    ssize_t bytes_read=0;
    int iBytesleft=0;
    int index=0;
    int i=0;
    int iTimeout=0;
    bool bSuccess = false;

    // check for data on port and display it on screen.
    bytes_read = read(serial, pbuffer, sSize);

    if ( bytes_read ) {
        unsigned int frame_length=pbuffer[0]+1;  //store frame length of frame
        if(frame_length==bytes_read){   //check wheter first byte of frame (frame length) is same as bytes read from serial port
            *sSize_frame=pbuffer[0];
            bSuccess=true;
        }
        else {
            if(frame_length>bytes_read) { //if frame is still not complete re-read data from serial port
                index=bytes_read;
                iBytesleft=frame_length-bytes_read;
                iTimeout=0;
                do {
                    usleep(SLEEP100MS); //time to send next bytes
                    bytes_read = read(serial, pbuffer+index, sSize-index);

                    iBytesleft-=bytes_read;
                    index+=bytes_read;
                    if(iTimeout++ > 100) break;

                } while(iBytesleft>0); //All data complete ?
                if(iBytesleft==0) bSuccess=true;
            }
        }

        if(infoflag>=SHOWALLDETAILS) {
            for(i=0; i<frame_length; i++)
                printf("%02X ", pbuffer[i]);         //show bytes recieved
            printf("\n");
        }
    }
    return bSuccess;
}

#pragma endregion

void * ThreadProc(void *arg) {
    do {
        usleep(SLEEP100MS);
        pthread_mutex_lock(&lockAPI);
        if(AmberCom != -1) GetDataFromStick(myhandle, myStickID, myInfoFlag);
        pthread_mutex_unlock(&lockAPI);
    } while( AmberCom != -1 );
    return 0;
}

#pragma region "Common"

/////////////////////////////////////////////////////////////////////////////////////////////

unsigned long wMBus_OpenDevice(char * device, uint16_t stick) {
    if(stick == iM871AIdentifier){
        pthread_mutex_init(&lockAPI, NULL);
        printf("Connect to IMST on port %s\n", device);
        //load external LIB
        libHandle = loadLibWMBusHCI();
        if(0 == libHandle) {
            printf("Library not found\n");
            return 0;
        }
        return WMBus_OpenDevice(device);
    }

    if(stick == iAMB8465Identifier) {
        pthread_mutex_init(&lockAPI, NULL);
        printf("Connect to AMBER on port %s\n", device);
        AmberCom = AMBER_OpenDevice(device, 9600);
        return (unsigned long) AmberCom;
    }
    return 0;
}
unsigned long wMBus_CloseDevice(unsigned long handle, uint16_t stick) {
    if(stick == iM871AIdentifier) {
        WMBus_CloseDevice(handle);
        unloadLibWMBusHCI(libHandle);
        pthread_mutex_destroy(&lockAPI);
        return 1;
    }

    if(stick == iAMB8465Identifier) {
        AMBER_CloseDevice((int)handle);
        AmberCom = -1; //get thread to terminate
        usleep(2*SLEEP100MS);
        pthread_join(ThreadID, NULL);
        pthread_mutex_destroy(&lockAPI);
        return 1;
    }
    return 0;
}

int wMBus_GetStickId(unsigned long handle, uint16_t stick, unsigned long *ID, uint16_t infoflag) {
    unsigned long dwReturn=(unsigned long)APIERROR;
    unsigned char *pData;
    uint16_t Datasize=BUFFER_SIZE;
    pData = (unsigned char *) malloc(Datasize);

    memset(pData,0,sizeof(unsigned char)*Datasize);
    if(NULL == ID) return dwReturn;

    if(stick == iM871AIdentifier) {
       if(WMBus_GetDeviceInfo(handle, pData, Datasize)) {
          *ID = *(pData + 1);
          dwReturn = APIOK;
        }
    }

    if(stick == iAMB8465Identifier) {
        short sWriteSize=(sizeof(CMD_SERIALNO_REQ_Arr))/(sizeof(uint8_t));
        AMBERCommand((int)handle, CMD_SERIALNO_REQ_Arr, pData, true, sWriteSize, Datasize, infoflag);
        *ID = *(pData + 3);
         dwReturn = APIOK;
    }

    if(pData) free(pData);
    return dwReturn;
}

unsigned long wMBus_GetLastError(unsigned long handle, uint16_t stick) {
    unsigned long dwReturn=0;

    if(stick == iM871AIdentifier) {
        dwReturn = WMBus_GetLastError(handle);
        char *pData;
        pData = (char*)malloc(128);
        memset(pData, 0, sizeof(unsigned char)*128);
        WMBus_GetErrorString(dwReturn, pData, 128);
        printf("GetLastError %ld %s\n", dwReturn, pData);
        free(pData);
    }
    return dwReturn;
}

unsigned long  wMBus_SwitchMode(unsigned long handle, uint16_t stick, uint8_t Mode, uint16_t infoflag) {
    unsigned long dwReturn=0;
    uint16_t Datasize=BUFFER_SIZE;
    unsigned char *pData;
    pData = (unsigned char *) malloc(Datasize);
    memset(pData,0,sizeof(unsigned char)*Datasize);

    if((Mode == RADIOT2) || (Mode == RADIOS2)) {
        if(stick == iM871AIdentifier){
            //set to S2/T2 Mode
            *(pData +0) = 3;
            *(pData +1) = 0;    //Other
            *(pData +2) = Mode; //S2=2   ; T2=4
            *(pData +3) = 0xB0; //IIFlag2 Auto RSSI, Auto Rx Timestamp, RTC Control
            *(pData +4) = 1;    //RSSI
            *(pData +5) = 1;    //Timestamp
            *(pData +6) = 1;    //RTC Control

            if((dwReturn = WMBus_SetDeviceConfig(handle, pData, 7, false))>0) {
                  if (infoflag > SILENTMODE) printf("IMST SwitchMode to  %s \n", ((Mode==RADIOT2) ?"T2" : "S2"));
              }
        }
        if(stick == iAMB8465Identifier) {
            if((dwReturn = AMBER_SwitchRFMode((int)handle, Mode, infoflag))>0) {
                 if (infoflag > SILENTMODE) printf("AMBER SwitchMode to  %s \n", ((Mode==RADIOT2) ?"T2" : "S2"));
            }
        }
    }
    free(pData);
    return dwReturn;
}

unsigned long  wMBus_GetRadioMode(unsigned long handle, uint16_t stick, unsigned long *dwD, uint16_t infoflag) {
    unsigned long  dwReturn=(unsigned long)APIERROR;
    unsigned char * pData = (unsigned char*)malloc(BUFFER_SIZE);
    if(NULL == pData)     return 0;
    if(0 == handle)       return 0;
    memset(pData, 0, sizeof(unsigned char)*BUFFER_SIZE);

    if(stick == iM871AIdentifier) {
        if(WMBus_GetDeviceConfig(handle,pData,BUFFER_SIZE)) {
            if (infoflag > SILENTMODE) {
                switch(*(pData +3)) {
                case RADIOT2: printf("T2 Mode \n");   break;
                case RADIOS2: printf("S2 Mode \n");   break;
                default:      printf("undefined \n"); break;
                }
            }
            dwReturn=APIOK;
            *dwD = *(pData +3);
        }
    }
    if(stick == iAMB8465Identifier) {
        short mode=0;
        short sWriteSize=(sizeof(CMD_GET_REQ_MODE_Arr))/(sizeof(uint8_t));
        if(AMBERCommand((int)handle, CMD_GET_REQ_MODE_Arr, pData, true, sWriteSize, BUFFER_SIZE, infoflag)) {
            switch(*(pData + 5)) {
                case RADIOT2_AMB: mode=RADIOT2; break;
                case RADIOS2_AMB: mode=RADIOS2; break;
                default:                        break;
            }
            if (infoflag > SILENTMODE) {
                switch(*(pData + 5)) {
                    case RADIOT2_AMB: printf("T2 Mode \n");   break;
                    case RADIOS2_AMB: printf("S2 Mode \n");   break;
                    default:          printf("undefined \n"); break;
                }
            }
            dwReturn=APIOK;
            *dwD = mode;
        }
    }
    free (pData);
    return dwReturn;
}

unsigned long  wMBus_IsNewData(unsigned long handle, uint16_t stick, uint16_t infoflag) {
    unsigned long  dwReturn=0;
    unsigned long  dwNewData=0;

    unsigned char *pData = (unsigned char *) malloc(BUFFER_SIZE);
    if(NULL == pData) return 0;
    if(0 == handle) return 0;
    memset(pData, 0, sizeof(unsigned char)*BUFFER_SIZE);

    if(stick == iM871AIdentifier){
        if(WMBus_GetSystemStatus(handle, pData, BUFFER_SIZE)) {
            dwNewData = *((unsigned long*) (pData+23));
            if(dwFrameCounter == 0)
                dwReturn = 0; //start condition
            else
                dwReturn = dwNewData - dwFrameCounter;
            if (infoflag > SILENTMODE) printf("wMBus_IsNewData %ld Bytes -> new %ld \n", dwNewData, dwReturn);
            dwFrameCounter = dwNewData;
        }
        else {
            if (infoflag > SILENTMODE) printf("WMBus_GetSystemStatus returns 0\n");
        }
    }
    free (pData);
    return dwReturn;
}

void wMBus_Callback(UINT32 msg, UINT32 param) {
    if(msg == WMBUS_MSG_HCI_MESSAGE_IND) {
        GetDataFromStick(myhandle, myStickID, myInfoFlag);
    }
}

unsigned long  wMBus_InitDevice(unsigned long handle, uint16_t stick, uint16_t infoflag) {
    myInfoFlag = infoflag;
    myStickID = stick;
    //clear Array
    memset(MeterAddr, 0, MAXSLOT*sizeof(ecwMBUSMeter));
    memset(MeterData, 0, MAXSLOT*sizeof(ecMBUSData));

    if(stick == iM871AIdentifier) {
        if(!bCallbackRegistered) {
            bCallbackRegistered=true;
            myhandle=handle;
            WMBus_RegisterMsgHandler(&wMBus_Callback);
        }
        unsigned char Filter[8];
        unsigned char Key[16];
        int iX;
        for (iX=0;iX<AES_KEYLENGHT_IN_BYTES;iX++)
            Key[iX] = (unsigned char) iX;

        Filter[0] = 0x25B3>>8;
        Filter[1] = 0xB3;
        Filter[2] = 0x12;
        Filter[3] = 0x34>>16;
        Filter[4] = 0x56>>8;
        Filter[5] = (unsigned char) (0x70+iX);
        Filter[6] = 0x01;
        Filter[7] = 0x02;

        //clear all Key Slots
        for (iX=0;iX<16;iX++) {
            Filter[5] =  (unsigned char) (0x70+iX);
            WMBus_ConfigureAESDecryptionKey(handle, iX, Filter, Key);
        }
    }
    if(stick == iAMB8465Identifier){
        short sWriteSize=0;

       //Enable AES
       sWriteSize=(sizeof(SET_AES_ENABLE_REQ_Arr))/(sizeof(uint8_t));
       if(AMBERCommand((int)handle,SET_AES_ENABLE_REQ_Arr, NULL, true, sWriteSize, BUFFER_SIZE, infoflag))
            if(infoflag>=SHOWALLDETAILS) printf("AES\n");

       //Enable RSSI
       sWriteSize=(sizeof(SET_RSSI_ENABLE_REQ_Arr))/(sizeof(uint8_t));
       if(AMBERCommand((int)handle,SET_RSSI_ENABLE_REQ_Arr, NULL, true, sWriteSize, BUFFER_SIZE, infoflag))
            if(infoflag>=SHOWALLDETAILS) printf("RSSI\n");

       //Start thread
        pthread_create(&ThreadID, NULL, ThreadProc, NULL);
    }
    return 1;
}

unsigned long  wMBus_AddMeter(unsigned long handle,uint16_t stick,int slot,pecwMBUSMeter NewMeter,uint16_t infoflag) {
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
            unsigned char Filter[sizeof(CMD_SET_AES_KEY_REQ_Arr)];
            memset(Filter,0,sizeof(CMD_SET_AES_KEY_REQ_Arr));

            MeterPresent |= 0x01 << slot;
            MeterAddr[dwMeter].manufacturerID   = NewMeter->manufacturerID;
            MeterAddr[dwMeter].ident            = NewMeter->ident;
            MeterAddr[dwMeter].version          = NewMeter->version;
            MeterAddr[dwMeter].type             = NewMeter->type;
            memcpy(MeterAddr[dwMeter].key, NewMeter->key, AES_KEYLENGHT_IN_BYTES);

            Filter[ 3] = (unsigned char) NewMeter->manufacturerID;
            Filter[ 4] = (unsigned char)(NewMeter->manufacturerID>>8);
            Filter[ 5] = (unsigned char) NewMeter->ident;
            Filter[ 6] = (unsigned char)(NewMeter->ident>>8);
            Filter[ 7] = (unsigned char)(NewMeter->ident>>16);
            Filter[ 8] = (unsigned char)(NewMeter->ident>>24);
            Filter[ 9] = NewMeter->version;
            Filter[10] = NewMeter->type;

            if(stick == iM871AIdentifier) WMBus_ConfigureAESDecryptionKey(handle, (unsigned char)slot, &Filter[3],(unsigned char*) MeterAddr[dwMeter].key);
            if(stick == iAMB8465Identifier) {
                Filter[0]=CMD_SET_AES_KEY_REQ_Arr[0]; //first 3 bytes used for set AES key message
                Filter[1]=CMD_SET_AES_KEY_REQ_Arr[1];
                Filter[2]=CMD_SET_AES_KEY_REQ_Arr[2];

                for(i = 0;i<AES_KEYLENGHT_IN_BYTES;i++) //Key Registration
                    Filter[11+i]=NewMeter->key[i];

                Filter[sizeof(CMD_SET_AES_KEY_REQ_Arr)-1]=CRC_XOR(Filter, sizeof(CMD_SET_AES_KEY_REQ_Arr)-1); //CRC

                if(AMBERCommand((int)handle, Filter, NULL, true, sizeof(CMD_SET_AES_KEY_REQ_Arr), BUFFER_SIZE, infoflag) == 0) {
                    printf("Error...writing command failed\n");
                    return 0;
                }
            }

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

bool wMBus_IsInArray(ecwMBUSMeter p, ecwMBUSMeter MeterData[],int *Index) {
    int i;
    if((p.manufacturerID == 0) || (p.ident == 0)) return false;

    for(i=0;i<MAXSLOT;i++) {
        if( MeterData[i].manufacturerID==p.manufacturerID && MeterData[i].ident==p.ident && MeterData[i].version==p.version && MeterData[i].type==p.type) {
            break;
        }
    }
    *Index = i;
    return true;
}

int wMBus_RemoveMeter(int Index) {
    MeterPresent &= ~(0x01<<Index);
    memset(&MeterAddr[Index], 0, sizeof(ecwMBUSMeter));
    return 0;
}

unsigned long wMBus_GetMeterList() {
    return MeterPresent;
}

unsigned long wMBus_GetMeterDataList() {
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

void GetDataFromStick(unsigned long handle, uint16_t stick, uint16_t infoflag) {
    unsigned long   dwReturn=0;
    int             PayLoadLength;
    unsigned long   TimeStamp=0;
    int8_t          RSSI=0;
    bool            bIsDecrypted=false;
    int             Offset=0;
    short           sSize = 1024;
    short           sSize_frame = 0;
    unsigned char   *pBuffer;

    pBuffer = (unsigned char *) malloc(sSize);
    memset(pBuffer, 0, sizeof(unsigned char)*sSize);
    if(stick == iM871AIdentifier)   dwReturn = WMBus_GetHCIMessage(handle, pBuffer, sSize);
    if(stick == iAMB8465Identifier) dwReturn = AMBER_ReadFrameFromStick(AmberCom, pBuffer+2, sSize, &sSize_frame, infoflag); //AMBER has bytes less in header than IMST: Length(8Bit)->>>Payload

    if(dwReturn) {
        PayLoadLength = *(pBuffer+2);
        if (infoflag > SILENTMODE) printf(" PayloadLength %d ", PayLoadLength);

        if(stick == iM871AIdentifier) {
            if(*(pBuffer) & 0x20) { //If TimeStamp attached
                TimeStamp = *( (unsigned long*) (pBuffer+3+PayLoadLength));
                if (infoflag > SILENTMODE) printf("Timestamp=0x%08X ", (unsigned int)TimeStamp);
            }
            if(*(pBuffer) & 0x40) { //If RSSI attached
                uint8_t RSSIfromBuf = *(pBuffer+7+PayLoadLength);
                double b = -100.0 - (4000.0 / 150.0);
                double m = 80.0 / 150.0;
                RSSI=(int8_t)(m * (double)RSSIfromBuf + b);
                if (infoflag > SILENTMODE) printf("RSSI=%i", RSSI);
            }
        }

        if(stick == iAMB8465Identifier) {
            //RSSI is enabled and the last byte of the pBuffer
            uint8_t RSSIfromBuf = pBuffer[2+PayLoadLength]; //
            if(RSSIfromBuf>=128)
                RSSI=(int8_t)(((double)RSSIfromBuf-256.0)/2.0-74.0);
            else if(RSSIfromBuf<128){
                RSSI=(int8_t)((double)RSSIfromBuf/2.0-74.0);}
            else{
                RSSI=0;
            }
        }
        if (infoflag > SILENTMODE) printf("\n");

        ecMBUSData   RFData;    //struct to store value + rssi + timestamp
        ecwMBUSMeter RFSource;  //struct to store Source Address

        //data received with wrong key
        //1F 44 C4 18 63 18 76 15 01 02 7A FF 00 00 85 F1 9D 9F 21 25 93 54 26 6B 35 C0 C4 04 8B 43 93 47
        //Payload 31 RSSIfromBuf 47

        //data received with correct key
        //1F 44 C4 18 63 18 76 15 01 02 7A 00 00 00 85 2F 2F 04 05 11 09 04 00 02 FD 08 80 84 2F 2F 2F 49
        //Payload 31 RSSIfromBuf 49

        // When decryption was successful there are APL_DIF_DATA_FIELD_SPECIAL_FILLER at the offset OFFSETDECRYPTFILLER
        bIsDecrypted=false;
        unsigned short Filler = *((unsigned short*)(pBuffer+OFFSETPAYLOAD+OFFSETDECRYPTFILLER));
        unsigned short sF = APL_DIF_DATA_FIELD_SPECIAL_FILLER;
        sF = (sF<<8) | APL_DIF_DATA_FIELD_SPECIAL_FILLER ;
        if(Filler == sF) bIsDecrypted=true;

        memset(&RFData,   0, sizeof(ecMBUSData));
        memset(&RFSource, 0, sizeof(ecwMBUSMeter));

        RFData.time=TimeStamp;
        RFData.rssiDBm= RSSI;
        RFData.accNo  = *(pBuffer+OFFSETPAYLOAD+OFFSETACCESSNUMBER); //Access number
        RFData.status = *(pBuffer+OFFSETPAYLOAD+OFFSETSTATUS); //Status
        RFData.configWord = *((unsigned short*)(pBuffer+OFFSETPAYLOAD+OFFSETCONFIGWORD));
        RFData.mbusID =
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+3)<<24)+
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+2)<<16)+
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+1)<<8)+
             *(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID);

        uint8_t DIF=0;
        uint8_t VIF=0;

        //RFData.pktInfo = ((CW & 0x0500) == 0x0500) ? PACKET_WAS_ENCRYPTED : PACKET_WAS_NOT_ENCRYPTED;
        //The stick does the decryption and removes the flag
        RFData.pktInfo = PACKET_WAS_NOT_ENCRYPTED;

        Offset = 17;  //start to check DIF on Offset 17
        DIF = *(pBuffer+Offset++);
        while(DIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER) {
            DIF = *(pBuffer+Offset++);
            if(Offset > 100) break;
        }
        VIF = *(pBuffer+Offset++);
        while(VIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER) {
            VIF = *(pBuffer+Offset++);
            if(Offset > 100) break;
        }
        RFData.exp   = 0;
        RFData.value = 0;

        //current PayLoadLength
        //20 is not encrypted
        //30 encrypted

        if((PayLoadLength < WMBUS_PAYLOADLENGTH_ENCRYPTED) || bIsDecrypted ) {
            if((WMBUS_MSGLENGTH_AESERROR == PayLoadLength) && (*(pBuffer+1) == WMBUS_MSGID_AES_DECRYPTIONERROR)) {
                RFData.pktInfo=PACKET_DECRYPTIONERROR;
            }
            else {
                if(stick == iM871AIdentifier) {
                    if(PayLoadLength > WMBUS_PAYLOADLENGTH_DEFAULT)
                        RFData.pktInfo = PACKET_WAS_ENCRYPTED;
                }
                if(stick == iAMB8465Identifier) {
                    if(PayLoadLength > (WMBUS_PAYLOADLENGTH_DEFAULT+1)) //RSSI is attached
                        RFData.pktInfo = PACKET_WAS_ENCRYPTED;
                }
                RFData.valDuringErrState = (DIF & APL_DIF_FUNCTIONFIELD) == APL_DIF_FUNC_ERROR;
                if(APL_VIF_ENERGY_WH == (VIF & APL_VIF_UNITCODE)) {
                    RFData.exp = (VIF&0x07)-3;

                    if(APL_DIF_DATA_FIELD_32_INT == (DIF & APL_DIF_DATAFIELD))
                        RFData.value = *((unsigned long *)(pBuffer+Offset));
                    if(APL_DIF_DATA_FIELD_12_BCD == (DIF & APL_DIF_DATAFIELD)) {
                        uint8_t bcdbytes[12/2];
                        memcpy(bcdbytes, pBuffer+Offset,12/2);
                        saBCD12ToUINT32(bcdbytes,12/2, &RFData.value);
                    }
                }
                if(APL_VIF_VOLUME_M3 == (VIF & APL_VIF_UNITCODE)) {
                    RFData.exp = (VIF&0x07)-6;
                    if(APL_DIF_DATA_FIELD_32_INT == (DIF & APL_DIF_DATAFIELD))
                        RFData.value = *((unsigned long *)(pBuffer+Offset));
                    if(APL_DIF_DATA_FIELD_12_BCD == (DIF & APL_DIF_DATAFIELD)) {
                        uint8_t bcdbytes[12/2];
                        memcpy(bcdbytes, pBuffer+Offset, 12/2);
                        saBCD12ToUINT32(bcdbytes, 12/2, &RFData.value);
                    }
                }

                // get the next DIF filled but ignore 0x2f
                DIF = *(pBuffer+Offset++);
                while(DIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER) {
                    DIF = *(pBuffer+Offset++);
                    if(Offset > 100) break;
                }

                // get the next VIF filled but ignore 0x2f
                VIF = *(pBuffer+Offset);
                while(VIF == APL_DIF_DATA_FIELD_SPECIAL_FILLER || (VIF & APL_VIF_EXTENSION_BIT)) {
                    VIF = *(pBuffer+Offset++);
                    if(Offset > 100) break;
                }

                // read the tx and pic counters
                if ( (DIF & APL_DIF_DATAFIELD) == APL_DIF_DATA_FIELD_16_INT && VIF == APL_VIFE_TRANS_CTR ) {
                    RFData.utcnt_tx  = *(pBuffer+Offset++);
                    RFData.utcnt_pic = *(pBuffer+Offset++);
                }
            }
        }
        else {
            //iPayLoadLength show a encrypted message
            if(!bIsDecrypted) {
                RFData.pktInfo = PACKET_DECRYPTIONERROR;
            }
        }

        RFSource.manufacturerID = (*(pBuffer+OFFSETPAYLOAD+OFFSETMANID+1) << 8) + *(pBuffer+OFFSETPAYLOAD+OFFSETMANID);
        RFSource.ident          =
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+3)<<24) +
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+2)<<16) +
            (*(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID+1)<<8)  +
             *(pBuffer+OFFSETPAYLOAD+OFFSETMBUSID);
        RFSource.version        =  *(pBuffer+OFFSETPAYLOAD+OFFSETVERSION);
        RFSource.type           =  *(pBuffer+OFFSETPAYLOAD+OFFSETTYPE);

        if (infoflag > SILENTMODE) printf("Meter  %04X %08X %02X %02X %d (exp) %d \n", RFSource.manufacturerID, RFSource.ident, RFSource.version, RFSource.type, RFData.value, RFData.exp);

        int MeterIndex;
        if(wMBus_IsInArray(RFSource,MeterAddr,&MeterIndex)) {
            if(MeterIndex < MAXSLOT) {
                //If decryption doesn't work 2 Messages are sent - keep Decryption Error Status
                if(PACKET_DECRYPTIONERROR == MeterData[MeterIndex].pktInfo)
                    RFData.pktInfo=PACKET_DECRYPTIONERROR;
                memcpy(&MeterData[MeterIndex],&RFData,sizeof(ecMBUSData));
                MeterHasData=MeterHasData | (0x01<<MeterIndex); //set bit which MeterData was recieved
            }
        }
    }
    if(pBuffer) free(pBuffer);
}

unsigned long wMBus_GetData4Meter(int Index, psecMBUSData data) {
    unsigned long dwReturn;

    if (Index > MAXSLOT)
        return 0;

    dwReturn = MeterHasData & (0x01<<Index); //get Bit out of mask
    if(NULL != data) memcpy(data, &MeterData[Index], sizeof(ecMBUSData));
    memset(&MeterData[Index], 0, sizeof(ecMBUSData));
    MeterHasData &= ~(0x01<<Index); //clear Bit
    return dwReturn;
}
#pragma endregion
