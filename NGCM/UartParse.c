/*
 * UartParse.c
 *
 * Created: 4/5/2016 4:22:52 PM
 *  Author: mikev
 */ 
#include "UartParse.h"
#include "Uart.h"
#include <string.h>
#include "Io.h"
#include "Util.h"
#include "Ekey.h"
#include <avr/eeprom.h>
#include "Adc.h"
#include "SmartLink.h"
#include "debug.h"		//Clem

extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;

static const char HELP_MENU[] = {
	"\r\n============ Help Menu\r\n\r\n"
	"d\tdisplay F/W compile date\r\n"
	"01g,g\tdo/display weight gain calibration\r\n"
	"h\tdisplay this help menu\r\n"
	"01o,o\tdo/display weight offset calibration\r\n"
	"r\tdisplay last read ekey data\r\n"              // v1.19  r  before t  // gk100
	"u\tunlock serial communications mode\r\n"
	"v\tdisplay firmware version\r\n"
	"w\tdisplay weight measurement\r\n"
	"\r\n============\r\n\r\n"
};

//const char COMPILE_DATE[]=__DATE__;
//const char COMPILE_TIME[]=__TIME__;
static const char COMPILE_INFO[] = __DATE__"\r\n"__TIME__"\r\n";
	
static const char MSG_OK[] = "OK";
static const char VERSION_STR[] = "V"FW_VERSION"\r\n";

bit allOK = false;
bit sendBuf = false;
bit lockSerialMode;

void UartParse_rxMsg() {
	const char *cptr;
	uint8_t msgLen;
	uint16_t temp, temp1, temp2;
	TAG_DATA *td;
	char buf[60];
	uint8_t configCode;
	
	allOK = 1;
	lockSerialMode = 1;
	cptr = NULL;
	
	msgLen = strlen(Uart_rxBuf);
	Uart_txBuf[0] = '\0';
	
	if(msgLen > 0) {
		
		if( Uart_rxBuf[0] == SMARTLINK_RX_PACKET_ID ) {
			
			Uart_rxBuf[3] = '\0';	//set for ascii back conversion
			configCode = Util_hexStringToLong(&Uart_rxBuf[1]);
			
			if( configCode == SMARTLINK_DISPENSE_CODE ) {				//code for 'do a dispense'
				sysFlags.dispensePending = 1;				
			}
			//------------------------e100----------------------------------
			
			if( configCode == SMARTLINK_VALVE_ON_CODE ) {				
				Io_valve(IO_VALVE_OPEN);
			}
			if( configCode == SMARTLINK_VALVE_OFF_CODE ) {
				Io_valve(IO_VALVE_CLOSE);
			}
			//------------------------e100----------------------------------
			
			//if( configCode == SMARTLINK_QUERY_CODE ) {				//code for 'send SmartLink data'
			if((Uart_rxBuf[0] == SMARTLINK_RX_PACKET_ID) && (Uart_rxBuf[1] =='0') && (Uart_rxBuf[2] == '0'))
			{	//------------------weight measurment--------------------------------------------------------
				OPAMP_PWR_ON();
				_delay_ms(100);// check if this delay can be optimized // C100
				sysData.currentSoapWeight = Adc_getWeightUnits(); // t100
				System_fillStats.initialWeight = sysData.currentSoapWeight; //t100
				OPAMP_PWR_OFF(); //t100 
				SmartLink_serializeTxPacket(buf);
				//------------------weight measurment--------------------------------------------------------
				cptr = buf;
			}
			
			lockSerialMode = 0;
		} else {
			switch( Uart_rxBuf[msgLen-1] ) { //command character

				//read ekey
				case 'd':
					cptr = COMPILE_INFO;
	//				cptr = "this is data";
					break;
				
				case 'g':
					if(msgLen == 3) {
						Uart_rxBuf[2] = '\0';
					
						if( strcmp((const char *)Uart_rxBuf, "01") == 0 ) {
							temp = Adc_getChannel(ADC_WEIGHT_CHAN);
							temp1 = eeprom_read_word(EEPROM_ADDR_OFFSET);	
							if(temp > temp1) {
								temp -= temp1;					//correct for offset
							} else {
								temp = 1;						//prevent from being 0
							}
							eeprom_write_word (EEPROM_ADDR_GAIN, temp);
							Util_strcpy(Uart_txBuf, MSG_OK);
						}
					} else {
						temp = Adc_getWeightGain();
						Util_wordToString(Uart_txBuf, temp);
					}
					break;
				
				case 'h':
					cptr = HELP_MENU;
					break;				
			
				//reset ekey
				case 'o':
					if(msgLen == 3) {
						Uart_rxBuf[2] = '\0';
						if( strcmp((const char *)Uart_rxBuf, "01") == 0 ) {
							temp = Adc_getChannel(ADC_WEIGHT_CHAN);
							eeprom_write_word (EEPROM_ADDR_OFFSET, temp);
							Util_strcpy(Uart_txBuf, MSG_OK);
						}
					} else {
						temp = Adc_getWeightOffset();
						Util_wordToString(Uart_txBuf, temp);
					}
					break;
		
			
				case 'r':
					td = Ekey_getTagData();
					Util_longToString(buf, td->page0);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page1);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page2);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page3);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page4);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page5);
					Util_strcat(Uart_txBuf, buf);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page6);
					Util_strcat(Uart_txBuf, "\r\n");
				
					Util_longToString(buf, td->page7);
					Util_strcat(Uart_txBuf, buf);
	//				Util_strcat(Uart_txBuf, "\r\n");
					break;
				
				case 's':
					Ekey_program();
					MLX_MODU_OFF();
					break;
					
				case 'c':			//Clem
					sysData.currentSoapWeight = Adc_getWeightUnits();
					Util_byteToString(Uart_DtxBuf, (uint8_t)sysData.currentSoapWeight);
					Util_strcat(Uart_DtxBuf, "\r\n");
					Uart_transmit(Uart_DtxBuf);
									
				break;					
				
				case 'u':
					sysFlags.unlockSerialModeFlag = 1;		//unlock the serial mode after this transmission
					Util_strcpy(Uart_txBuf, "UNLOCKED");
					break;
				
				case 'v':
					cptr = VERSION_STR;
					break;
					
				case 'w':
					sysData.currentSoapWeight = Adc_getWeightUnits();
					Util_byteToString(Uart_txBuf, (uint8_t)sysData.currentSoapWeight);
					
					break;
			
				default:
					allOK = false;
					break;
			}//switch
		}//first character
			
	} else {//msgLen>0
		allOK = false;
	}

	if(allOK) {
		if( lockSerialMode ) {
			sysFlags.lockSerialMode = 1;			//valid message received and executed. Lock into serial mode
			OPAMP_PWR_ON();								//put this on as weight system is being used
		}
		if(!sysFlags.dispensePending) {				//SmartLink may set this pending. No response required.
			if(cptr == NULL) {
				Util_strcat(Uart_txBuf, "\r\n");
				Uart_transmit(Uart_txBuf);
			
			} else {
				Uart_transmit( (char *)cptr );
			}
		}

	}
	
	Uart_rxStart();		//resets receive buffer
}
