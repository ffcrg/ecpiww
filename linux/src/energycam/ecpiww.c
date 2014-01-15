#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <energycam/ecpiww.h>
#include <energycam/wmbus.h>


void Colour(int8_t c, bool cr) {
    printf("%c[%dm",0x1B,(c>0) ? (30+c) : c);
    if(cr)
        printf("\n");
}

//Log Reading with date info to CSV File
int Log2CSVFile(const char *path,  double Value) {
    FILE    *hFile;
    uint32_t FileSize = 0;


    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    if ((hFile = fopen(path, "rb")) != NULL) {
        fseek(hFile, 0L, SEEK_END);
        FileSize = ftell(hFile);
        fseek(hFile, 0L, SEEK_SET);
        fclose(hFile);
    }

    if ((hFile = fopen(path, "a")) != NULL) {
        if (FileSize == 0)  //start a new file with Header
            fprintf(hFile, "Date, Value \n");
        fprintf(hFile,"%d-%02d-%02d %02d:%02d, %.1f\r\n", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, Value);
        fclose(hFile);
    } else
        return APIERROR;

    return APIOK;
}


int getkey(void) {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN]  = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}

static int iTime;
static int iMinute;

int IsNewSecond(int iS)
{
    int CurTime;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    CurTime = tm.tm_hour*60*60+tm.tm_min*60+tm.tm_sec;
    if (iS > 0)
        CurTime = CurTime/iS;

    if (CurTime != iTime) {
        iTime = CurTime;
        return 1;
    }
    return 0;
}

int IsNewMinute(void) {
    int CurTime;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    CurTime = tm.tm_hour*60+tm.tm_min;

    if (CurTime != iMinute) {
        iMinute = CurTime;
        return 1;
    }
    return 0;
}

void Intro(void)
{
    printf("   \n");
    Colour(62, false);
    printf("##########################################################################\n");
    printf("## ecpiww - EnergyCam on raspberry Pi/Weezy -connected via wMBUS + wLAN ##\n");
    printf("##########################################################################\n");
    Colour(0, true);
    printf("   Usage\n");
    printf("   s   : Use S2 mode\n");
    printf("   t   : Use T2 mode\n");
    printf("   a   : Add meter\n");
    printf("   r   : Remove meter\n");
    printf("   l   : List meters\n");
    printf("   u   : Update - check for data\n");
    printf("   q   : Quit\n");
    printf("   \n");
}

void IntroShowParam(void)
{
    printf("   \n");
    Colour(62,false);
    printf("##########################################################################\n");
    printf("## ecpiww - EnergyCam on raspberry Pi/Weezy -connected via wMBUS + wLAN ##\n");
    printf("##########################################################################\n");

    Colour(0,true);
    printf("   Commandline options:\n");
    printf("   ./ecpiww -f /home/user/ecdata -p 0 -m S\n");
    printf("   -f <dir> : save readings to <dir>/<wMBUS Ident>.dat\n");
    printf("   -l VZ : log to (VZ)Volkszähler, (XML) XMLFile, (CSV) CSV File \n");
    printf("   -p 0 : Portnumber 0 -> /dev/ttyUSB0\n");
    printf("   -m S : S2 mode \n");
    printf("   -i    : show detailed infos \n\n");

}

void ErrorAndExit(const char *info) {
    Colour(PRINTF_RED, false);
    printf("%s", info);
    Colour(0, true);
    exit(0);
}

unsigned int CalcUIntBCD(  unsigned int ident)
{
    int32_t identNumBCD=0;
    #define MAXIDENTLEN 12
    uint8_t  identchar[MAXIDENTLEN];
    memset(identchar,0,MAXIDENTLEN*sizeof(uint8_t));
    sprintf((char *)identchar, "%08d", ident);
    uint32_t uiMul=1;
    uint8_t uiX=0;
    uint8_t uiLen= strlen((char const*)identchar);

    for(uiX=0; uiX < uiLen;uiX++) {
        identNumBCD += (identchar[uiLen-1-uiX] - '0')*uiMul;
        uiMul = uiMul*16;
    }
    return identNumBCD;
}

void DisplayListofMeters(int iMax, pecwMBUSMeter ecpiwwMeter)
{
int iX,iI;

    if(iMax == 0) printf("\nNo Meters defined.\n");
    else {
        iI=0;
        for(iX=0; iX<iMax; iX++) {
            if( 0 != ecpiwwMeter[iX].manufacturerID)
               iI++;
        }
        printf("\nList of active Meters (%d defined):\n", iI);
     }

    for(iX=0;iX<iMax;iX++) {
        if( 0 != ecpiwwMeter[iX].manufacturerID) {
            printf("Meter#%d : Manufactor = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].manufacturerID);
            printf("Meter#%d : Ident      = 0x%08X \r\n", iX+1, ecpiwwMeter[iX].ident);
            printf("Meter#%d : Type       = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].type);
            printf("Meter#%d : Version    = 0x%02X \r\n", iX+1, ecpiwwMeter[iX].version);
            printf("Meter#%d : Key        = 0x", iX+1);
            for(iI = 0; iI<AES_KEYLENGHT_IN_BYTES; iI++)
                printf("%02X",ecpiwwMeter[iX].key[iI]);
            printf("\r\n");
        }
    }
}

void UpdateMetersonStick(unsigned long handle,int iMax, pecwMBUSMeter ecpiwwMeter)
{
    int iX;

    for(iX=0;iX<MAXMETER;iX++)
        IMST_RemoveMeter(iX);

    for(iX=0;iX<iMax;iX++) {
        if( 0 != ecpiwwMeter[iX].manufacturerID)
            IMST_AddMeter(handle,iX,&ecpiwwMeter[iX]);
    }
}

#define XMLBUFFER (1*1024*1024)
unsigned int Log2XMLFile(const char *path,double Reading,int RSSI)
{
	char szBuf[250];
	FILE    *hFile;
	unsigned char*   pXMLIN = NULL;
	unsigned char*   pXMLTop,*pXMLMem = NULL;
	unsigned char*  pXML;
	unsigned int   dwSize=0;
	unsigned int   dwSizeIn=0;
	char  CurrentTime[250];


	time_t t = time(NULL);
	struct tm tm = *localtime(&t);


	if ( (hFile = fopen(path, "rb")) != NULL ) {
		fseek(hFile, 0L, SEEK_END);
		dwSizeIn = ftell(hFile);
		fseek(hFile, 0L, SEEK_SET);

		pXMLIN = (unsigned char*) malloc(dwSizeIn);
		if(NULL == pXMLIN) {
		    printf("Log2XMLFile - malloc failed \r\n");
		    return 0;
		}

		fread(pXMLIN, dwSizeIn, 1, hFile);
		fclose(hFile);

		pXMLTop = (unsigned char *)strstr((const char*)pXMLIN,"<ENERGYCAMOCR>\r\n"); //search on start
		if(pXMLTop){
			pXMLTop += strlen("<ENERGYCAMOCR>\r\n");
			pXMLMem = (unsigned char *) malloc(max(4*XMLBUFFER,XMLBUFFER+dwSizeIn));
			if(NULL == pXMLMem) {
				printf("Log2XMLFile - malloc %d failed \r\n",max(4*XMLBUFFER,XMLBUFFER+dwSizeIn));
				return 0;
			}
			memset(pXMLMem,0,sizeof(unsigned char)*max(4*XMLBUFFER,XMLBUFFER+dwSizeIn));
			pXML = pXMLMem;
			dwSize=0;
			dwSizeIn -= pXMLTop-pXMLIN;

			sprintf(szBuf,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
			memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);

			sprintf(szBuf,"<ENERGYCAMOCR>\r\n");
			memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);
		}
	}
	else {
		//new File
		pXMLMem = (unsigned char *) malloc(XMLBUFFER);
		memset(pXMLMem,0,sizeof(unsigned char)*XMLBUFFER);
		pXML = pXMLMem;
		dwSize=0;

		sprintf(szBuf,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
		memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);

		sprintf(szBuf,"<ENERGYCAMOCR>\r\n");
		memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);
	}




	if(pXMLMem) {
		sprintf(szBuf,"<OCR>\r\n");
		memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);

		sprintf(CurrentTime,"%02d.%02d.%d %02d:%02d:%02d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,tm.tm_hour,tm.tm_min,tm.tm_sec);
		sprintf(szBuf,"<Date>%s</Date>\r\n",CurrentTime);memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);

		sprintf(szBuf,"<Reading>%.1f</Reading>\r\n",Reading);memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);
		if(RSSI != 0) {sprintf(szBuf,"<RSSI>%d</RSSI>\r\n",RSSI);memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);}
		sprintf(szBuf,"</OCR>\r\n");memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);


		if(dwSizeIn>0){
			memcpy(pXML,pXMLTop,dwSizeIn);
		}
		else {
			sprintf(szBuf,"</ENERGYCAMOCR>\r\n");	memcpy(pXML,szBuf,strlen(szBuf));dwSize+=strlen(szBuf);pXML+=strlen(szBuf);
		}

		if ( (hFile = fopen(path, "wb")) != NULL ) {
			fwrite(pXMLMem, dwSizeIn+dwSize, 1, hFile);
			fclose(hFile);
			chmod(path, 0666);
		}
	}

	if(pXMLMem) free(pXMLMem);
	if(pXMLIN) free(pXMLIN);


	return true;
}



//Log Reading with date info to CSV File
int Log2File(char *DataPath, uint16_t mode,uint16_t meterindex,uint16_t infoflag, float metervalue,int RSSI,uint32_t ident) {

char   param[_MAX_PATH];
char     datFile[_MAX_PATH];
FILE    *hDatFile;
time_t t;
struct tm curtime;

switch(mode){
	case LOGTOCSV : sprintf(param, "/var/www/ecpiww/data/ecpiwwM%d.csv", meterindex+1);
					Log2CSVFile(param, metervalue); //log kWh
					return APIOK;
					break;
	case LOGTOXML : if (strlen(DataPath) == 0)	{
					  getcwd(param, sizeof(param));
					  sprintf(param, "%s/%08X.xml",param, ident);
				    }else{
					  sprintf(datFile, "%s/%08X.xml",DataPath, ident);
				    }
					Log2XMLFile(param,metervalue,RSSI); //log kWh
					return APIOK;
					break;
	case LOGTOVZ :  if( access( "add2vz.sh", F_OK ) != -1 ) {
							memset(param, '\0', sizeof(FILENAME_MAX));
							sprintf(param, "./add2vz.sh %d %ld ",meterindex+1,(long int)metervalue*1000);
							int ret=system(param);
							t = time(NULL);
							curtime = *localtime(&t);

							if(0x100 == ret){
								 if(infoflag > SILENTMODE) printf("%02d:%02d Calling %s \n",curtime.tm_hour,curtime.tm_min,param);
							}else{
								 if(infoflag > SILENTMODE) printf("%02d:%02d Calling %s returned with 0x%X\n",curtime.tm_hour,curtime.tm_min,param,ret);
							}
						}
					return APIOK;
					break;
	case LOGTODAT:
					sprintf(datFile, "%s/%08X.dat",DataPath, ident);
					  if ((hDatFile = fopen(datFile, "wb")) != NULL) {
						fprintf(hDatFile,"%.1f\r\n",  metervalue);
						fclose(hDatFile);
					  }
					return APIOK;
					break;
				}
return APIERROR;
}

//support commandline
int parseparam(int argc, char *argv[],char *filepath,uint16_t *infoflag,uint16_t *Port,uint16_t *Mode,uint16_t *LogMode) {
	int c;

  if((NULL == LogMode) || (NULL == infoflag) || (NULL == Port)  || (NULL == Mode) ) return 0;


	opterr = 0;
	while ((c = getopt (argc, argv, "f:hil:m:p:x")) != -1){
		switch (c){
			case 'f':
			 if (NULL != optarg){
				 strcpy(filepath,optarg);
			 }
			 break;
			case 'i':
				 *infoflag = SHOWDETAILS;
				 break;
			case 'l':
			 if (NULL != optarg){
				 if(0 == strcmp("VZ",optarg))  *LogMode=LOGTOVZ;
				 if(0 == strcmp("XML",optarg)) *LogMode=LOGTOXML;
				 if(0 == strcmp("DAT",optarg)) *LogMode=LOGTODAT;
			 }
			 break;
			case 'p':
				 if (NULL != optarg){
					 *Port = atoi(optarg);
				 }
				 break;
			case 'm':
				 if (NULL != optarg){
					 if(0 == strcmp("S",optarg)) *Mode=RADIOS2;
				 }
				 break;
			case 'h':
				IntroShowParam();
				exit (0);
				break;

			case '?':
				 if (optopt == 'f')
				   fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				 else if (isprint (optopt))
				   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				 else
				   fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
				 return 1;
			default:
				abort ();
		}
	}
return 0;
}

//////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int      key    = 0;
    int      iCheck = 0;
    int      iX;
    int      iK;
    char     KeyInput[_MAX_PATH];
    char     Key[3];
    char     CommandlineDatPath[_MAX_PATH];
    double   csvValue;
    int      Meters =0;
    unsigned long ReturnValue;
    FILE    *hDatFile;

    uint16_t InfoFlag = SILENTMODE;
    uint16_t Port = 0;
    uint16_t Mode = RADIOT2;
	uint16_t LogMode = LOGTOCSV;

    char     comDeviceName[100];
    int      hStick;

    ecwMBUSMeter ecpiwwMeter[MAXMETER];
    memset(ecpiwwMeter,0,MAXMETER*sizeof(ecwMBUSMeter));

    memset(CommandlineDatPath,0,_MAX_PATH*sizeof(char));



    if(argc > 1)
      parseparam(argc,argv,CommandlineDatPath,&InfoFlag,&Port,&Mode,&LogMode);


    //read config back
    if ((hDatFile = fopen("meter.dat", "rb")) != NULL) {
        Meters = fread((void*)ecpiwwMeter, sizeof(ecwMBUSMeter), MAXMETER, hDatFile);
        fclose(hDatFile);
    }

    Intro();

    //open wMBus Stick
    sprintf(comDeviceName, "/dev/ttyUSB%d",Port);
    hStick = IMST_OpenDevice(comDeviceName);

    if(hStick <= 0)
         ErrorAndExit("wMBUS Stick not found\n");

    if((APIOK == IMST_GetStickId(hStick, &ReturnValue)) && (iM871AIdentifier == ReturnValue)){
        if(InfoFlag > SILENTMODE) printf("iM871A Stick found\n");
	}
    else
        ErrorAndExit("wMBUS Stick not found\n");

    if(APIOK == IMST_GetRadioMode(hStick, &ReturnValue,InfoFlag)){
        if(InfoFlag > SILENTMODE) printf("wMBUS %s Mode\n", (ReturnValue == RADIOT2) ? "T2" : "S2");
        if (ReturnValue != Mode)
           IMST_SwitchMode( hStick, Mode,InfoFlag);
	   }
    else
        ErrorAndExit("wMBUS Stick not found\n");

    IMST_InitDevice(hStick,InfoFlag);

    UpdateMetersonStick(hStick, Meters, ecpiwwMeter);

    IsNewMinute();

    while (!((key == 0x1B) || (key == 'q'))) {
        usleep(500*1000);   //sleep 500ms

        key = getkey();

        /*key =fgetc(stdin);
        while(key!='\n' && fgetc(stdin) != '\n');
        printf("Key=%d",key);*/


        //add a new Meter
        if (key == 'a') {
            iX=0;
            while(0 != ecpiwwMeter[iX].manufacturerID) {
                iX++;
                if(iX == MAXMETER-1)
                  continue;
              }
            //check entry in list of meters
            if(iX < MAXMETER) {
                printf("\nAdding Meter #%d \n",iX+1);
                printf("Enter Meter Ident (12345678): ");
                if(fgets(KeyInput, _MAX_PATH,stdin))
                    ecpiwwMeter[iX].ident=CalcUIntBCD(atoi(KeyInput));

                printf("Enter Meter Type (2 = Electricity ; 3 = Gas ; 7 = Water) : ");
                if(fgets(KeyInput, _MAX_PATH,stdin)) {
                    switch(atoi(KeyInput))  {
                        case METER_GAS  :        ecpiwwMeter[iX].type = METER_GAS;          break;
                        case METER_WATER:        ecpiwwMeter[iX].type = METER_WATER;        break;
                        default: printf(" - wrong Type ; default to Electricity");
                        case METER_ELECTRICITY : ecpiwwMeter[iX].type = METER_ELECTRICITY;  break;
                    }

                }
                ecpiwwMeter[iX].manufacturerID = FASTFORWARD;
                ecpiwwMeter[iX].version        = 0x01;

                printf("Enter Key (0 = Zero ; 1 = Default ; 2 = Enter the 16 Bytes) : ");
                if(fgets(KeyInput, _MAX_PATH, stdin))
                {
                    switch(atoi(KeyInput)) {
                        case 0  : for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                    ecpiwwMeter[iX].key[iK] = 0;
                                  break;
                        default:
                        case 1  : for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                    ecpiwwMeter[iX].key[iK] = (0x1C + 3*iK);
                                 break;
                        case 2  :
                                printf("Key:");
                                fgets(KeyInput,_MAX_PATH,stdin);
                                    for(iK = 0; iK<AES_KEYLENGHT_IN_BYTES; iK++)
                                        ecpiwwMeter[iX].key[iK] = 0;
                                if((strlen(KeyInput)-1) < AES_KEYLENGHT_IN_BYTES*2)
                                    printf("Key is too short - default to Zero\n");
                                else {
                                    memset(Key,0,sizeof(Key));
                                    for(iK = 0; iK<(strlen(KeyInput)-1)/2; iK++) {
                                        Key[0] =  KeyInput[2*iK];
                                        Key[1] =  KeyInput[2*iK+1];
                                        ecpiwwMeter[iX].key[iK] = strtoul(Key,NULL,16);
                                        }
                                }
                                break;
                    }
                }

                Meters++;
                Meters = min(Meters, MAXMETER);
                DisplayListofMeters(Meters, ecpiwwMeter);
                UpdateMetersonStick(hStick, Meters, ecpiwwMeter);
            } else
                printf("All %d Meters defined\n", MAXMETER);
        }

        // display list of meters
        if(key == 'l')
            DisplayListofMeters(Meters, ecpiwwMeter);

        //remove a meter from the list
        if(key == 'r') {
            printf("Enter Meterindex to remove: ");
            if(fgets(KeyInput, _MAX_PATH, stdin)) {
                iX = atoi(KeyInput);
                if(iX-1 <= Meters-1) {
                    printf("Remove Meter #%d\r\n",iX);
                    memset(&ecpiwwMeter[iX-1], 0, sizeof(ecwMBUSMeter));
                    DisplayListofMeters(Meters, ecpiwwMeter);
                    UpdateMetersonStick(hStick, Meters, ecpiwwMeter);
                 } else
                    printf(" Index not defined\n");
            }
        }

        // switch to S2 mode
        if(key == 's')
            IMST_SwitchMode( hStick, RADIOS2,InfoFlag);

        // switch to T2 mode
        if(key == 't')
            IMST_SwitchMode( hStick, RADIOT2,InfoFlag);

        //check whether there are new data from the EnergyCams
        if (IsNewMinute() || (key == 'u')) {
            if(IMST_GetMeterDataList() > 0) {
                iCheck = 0;
                for(iX=0; iX<Meters; iX++){
                    if((0x01<<iX) & IMST_GetMeterDataList()) {
                        ecMBUSData RFData;
                        int iMul=1;
                        int iDiv=1;
                        IMST_GetData4Meter(hStick, iX, &RFData);
                        if(RFData.exp < 0) {  //GAS
                            for(iK=RFData.exp; iK<0; iK++)
                               iDiv=iDiv*10;
                            csvValue = ((float)RFData.value)/iDiv;
                        } else {
                            for(iK=0; iK<RFData.exp; iK++)
                                iMul=iMul*10;
                            csvValue = (float)RFData.value*iMul;
                        }
                        if(InfoFlag > SILENTMODE) {
							Colour(PRINTF_GREEN, false);
							printf("Meter #%d : %4.1f %s", iX+1, csvValue, (ecpiwwMeter[iX].type == METER_ELECTRICITY) ? "Wh" : "m'3");
						}
                        if(ecpiwwMeter[iX].type == METER_ELECTRICITY)
                            csvValue = csvValue/1000.0;

                        if(InfoFlag > SILENTMODE) {
							if((RFData.pktInfo & PACKET_WAS_ENCRYPTED)      == PACKET_WAS_ENCRYPTED)     printf(" Decryption OK");
							if((RFData.pktInfo & PACKET_WAS_NOT_ENCRYPTED)  == PACKET_WAS_NOT_ENCRYPTED) printf(" not encrypted");
							if((RFData.pktInfo & PACKET_IS_ENCRYPTED)       == PACKET_IS_ENCRYPTED)      printf(" is encrypted");
							printf(" RSSI=%i dbm, #%d \n", RFData.rssiDBm, RFData.accNo);
							Colour(0,false);
						}

						Log2File(CommandlineDatPath,LogMode,iX,InfoFlag,csvValue,RFData.rssiDBm,ecpiwwMeter[iX].ident);

                    }
                }
            } else {
			  if(InfoFlag > SILENTMODE) {
				  Colour(PRINTF_YELLOW, false);
				  printf("%02d ", iCheck);
				  Colour(0, false);
				  if((++iCheck % 20) == 0) printf("\r\n");
				}
            }
        }
    } // end while

    if(hStick >0) IMST_CloseDevice(hStick);

    //save Meter config to file
    if(Meters > 0) {
        if ((hDatFile = fopen("meter.dat", "wb")) != NULL) {
            fwrite((void*)ecpiwwMeter, sizeof(ecwMBUSMeter), MAXMETER, hDatFile);
            fclose(hDatFile);
        }
    }
    return 0;
}
