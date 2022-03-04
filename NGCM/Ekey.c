/*
 * Ekey.c
 *
 * Created: 10/22/2015 2:14:24 PM
 *  Author: mikev
 */ 
#include "Ekey.h"
#include "Io.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "BitQueue.h"
#include "Timer.h"
#include <string.h>	//for memcpy, memset
#include "System.h"

#define CRC_POLYNOM 0x1D
#define CRC_PRESET 0xFF

static BIT_QUEUE l_bq;
//----------------j100------------------------
static BIT_QUEUE l_bq2;
static BIT_QUEUE l_bq3;
static BIT_QUEUE l_bq4;
static BIT_QUEUE l_bq5;
//----------------j100------------------------
static uint8_t l_rxLength;		//number of bits to receive

static volatile bit l_carrierOn, l_eofBit, l_intFlg;
static uint8_t l_sofBits;
static uint16_t l_compareLoadValue;
static bit l_timeout, l_adv;
bit receive_flag=0; // f100
static uint32_t l_testData;
static uint32_t l_wrCmd;
static uint32_t l_readData;
static uint32_t l_readPageCmd;

#if DEBUG_RFID_TEST
static uint16_t l_lastCounter, l_thisCounter;
static uint16_t l_pwBuff[70];
static uint8_t l_bufIdx;
static uint32_t l_pwAccum;
static float l_pwF, l_TOF;
static uint16_t l_22TO, l_14TO, l_6TO, l_40TO, l_330TO;
#endif

static TAG_DATA l_tagData;

static uint32_t l_altUid1;
static uint32_t l_altUid2,l_altUid3,l_altUid4,l_altUid5; // j100  added new variables

static void transmit(void);
static bit receive(void);
static void calc_crc(unsigned char * crc, unsigned char data, unsigned char Bitcount);
static bit transceive(uint8_t cmd, uint8_t *data);
static uint32_t l_decodeUidRequest(void);
static uint8_t l_decode(uint64_t *uid, uint32_t *retval);

extern bit bag_empty;

/*
* See if an ekey is present and is valid
*/
bit Ekey_readAll(void) {
	
	uint8_t tries;
	uint8_t page;
	bit done;
	
	done = 0;
	tries = 0;
	
	memset(&l_tagData, 0, sizeof(TAG_DATA));	//zero the ekey tag image

	while(!done && (++tries<=EKEY_COMM_TRIES) ) {
#if !DEBUG_DISABLE_FMOD		
		FMOD_ON();
#endif
		MLX_MODU_OFF();	
		_delay_ms(10);
		
		l_tagData.page0 = 0;
		
		if( Ekey_uidRequest() ) {						//read the tag UID
	
			page = 0;
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));			//This will automatically read from HITAG page 1 regardless of page value.
		
			if( !done ) {
				l_tagData.page0 = Ekey_getAltUid();
				done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
			}
			//---------------------------------j100-----------------------------------------------
			//----------------------------------------------------------------
			if( !done ) {
				l_tagData.page0 = Ekey_getAltUid5();
				done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
			}
			//----------------------------------------------------------------
			if( !done ) {
				l_tagData.page0 = Ekey_getAltUid2();
				done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
			}
			//----------------------------------------------------------------
			//----------------------------------------------------------------
			if( !done ) {
				l_tagData.page0 = Ekey_getAltUid3();
				done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
			}
			//----------------------------------------------------------------
			//----------------------------------------------------------------
			if( !done ) {
				l_tagData.page0 = Ekey_getAltUid4();
				done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
			}
			//----------------------------------------------------------------
			
			//-----------------------------check ID completes-----------------------------------
			//---------------------------------j100-----------------------------------------------
			
			
		
			if(done) {		//// gk100
				page = 5;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5));  // gk100
				if(done)break;
				if(!done) {
					page = 5;
					done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5)); // gk100
					if(done)break;
				}
				if(!done) {
					page = 5;
					done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5)); // gk100
					if(done)break;
				}
				if(!done) {
					page = 5;
					done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5)); // gk100
					if(done)break;
				}
				if(!done) {
					page = 5;
					done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5)); // gk100
					if(done)break;
				}
				if(!done) {
					page = 5;
					done = Ekey_readPage(CMD_RD_PAGE, &page, &(l_tagData.page5)); // gk100
					if(done)break;
				}
					
			}
		}
	}
	
	return(done);
}

bit Ekey_program(void) {
	bit done;
	uint8_t page;
	uint8_t tries;
	
	done = 0;
	tries = 0;
	
	while(!done && (++tries<=EKEY_COMM_TRIES) ) {
#if !DEBUG_DISABLE_FMOD		
		FMOD_ON();
#endif
		MLX_MODU_OFF();
		_delay_ms(10);
		
		if( Ekey_uidRequest() ) {														//read the tag UID
			page = 0;
			_delay_ms(10);
			if(Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1)) ) {				//This will automatically read from HITAG page 1 regardless of page value.
				page = 7;
				if( transceive(CMD_WR_PAGE, &page) ) {									//enable page write
					l_tagData.page7 = 0x7AAA5555;										//some dummy data to page 7
					if( transceive(WR_PAGE_DATA, (uint8_t *)(&l_tagData.page7)) ) {		//send data

						if(transceive(CMD_RD_PAGE, &page)) {							//read back for verify
							l_readData = ((l_bq.array & 0xffffffff00)>>8);
							if(l_readData == l_tagData.page7) {
								page = 1;
								if( transceive(CMD_WR_PAGE, &page) ) {									//enable page write
									l_tagData.page1 = 0xC96000AA;										//configuration page
									if( transceive(WR_PAGE_DATA, (uint8_t *)(&l_tagData.page1)) ) {		//send data

										if(transceive(CMD_RD_PAGE, &page)) {							//read back for verify
											l_readData = ((l_bq.array & 0xffffffff00)>>8);
											l_readData |= 0xc0000000;									//no '1' will be read until a '0' is read
											if(l_readData == l_tagData.page1) {
												done = 1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	return(done);
}
/*
* make the RFID tag transmit first to retrieve the UID and soap data
*
*/
uint8_t Ekey_doTTF(void) {
	bit done;
	
	done = 0;
	
	MLX_MODU_OFF();
	_delay_ms(8);		//delay to allow ekey to reset.requires at least 4.8msec
		
	if( transceive(CMD_TTF, 0) ) {
		
		memcpy(&l_tagData, &l_bq.array, sizeof(TAG_DATA));	//make a buffered copy of tag pages 4,5
//		l_uid = ((l_bq.array & 0xffffffff00000000) >> 32);
		MLX_MODU_OFF();
		_delay_ms(8);		//delay to shut down tag
		done = 1;
	}
	
	return(done);
}

bit Ekey_uidRequest(void) {
	bit retval;
	retval = false;
	
	if(transceive( CMD_UID_REQ_ADV, 0)) {
		//32 bits + 3 sof bits + twresp (=208TO = 1.66msec). 35 bits@2khz = 17.5msec + 1.66msec
		//can wait a little longer since it will be in Init state
	//	_delay_ms(20);				//wait before sending next command. Can't read AC message anyway
		l_testData = ~l_bq.array;
		l_tagData.page0 = l_decodeUidRequest();
		retval = true;
	}
//	l_uid = 0x890e9081;
	return retval;
}

uint8_t Ekey_dockUsage(int howManyRefillUnits, int howManyRefillUses) {
	uint8_t page;		
	bit done;
	
	done = 0;
	page = 5;		//page '5'
		
	l_tagData.weightCounter -= howManyRefillUnits;							//b0 is the fill units remaining counter
	l_tagData.useCounter -= howManyRefillUses;
	
	if(l_tagData.weightCounter < 0) {
		l_tagData.weightCounter = 0;										//do not let it be less than zero
	}
	
	if(l_tagData.useCounter < 0) {
		l_tagData.useCounter = 0;
	}
		
	if( transceive(CMD_WR_PAGE, &page) ) {									//enable page write
		
		if( transceive(WR_PAGE_DATA, (uint8_t *)(&l_tagData.page5)) ) {		//send data

			if(transceive(CMD_RD_PAGE, &page)) {							//read back for verify
				l_readData = ((l_bq.array & 0xffffffff00)>>8);
				if(l_readData == l_tagData.page5) {
					done = 1;
				}
			}
		}
	}
	if(l_tagData.weightCounter == 0) bag_empty = 1; // gk100
	return(done);
}

bit Ekey_readPage(uint8_t cmd, uint8_t *pageNum, uint32_t *dest) {
	uint32_t data;
	uint8_t crc, crcR; 
	int i;
	uint8_t *ptr;
	
	static bit allOK;
	
	allOK = 0;
	
	if( transceive(cmd, pageNum) ) {
		if(l_adv) {	
			data = ((l_bq.array & 0xffffffff00) >> 8);	// in CMD_SELECT_UID read -> (CON0 | CON1 | CON2 | RESERVED | CRC8)
			crcR = (l_bq.array & 0xff);					// CRC8 checksum
			
			ptr = (uint8_t *)&data;
			crc = CRC_PRESET;							// initialize crc algorithm
			
			if(cmd == CMD_SELECT_UID) {
				data |= 0xc0000000;						// fudge patch. Receiver does not produce the 'c'!
			}
			
			for(i=3; i>=0; i--) {
				calc_crc(&crc, ptr[i], 8);				// compute crc
			}				
					
			if( crc == crcR ){							//checksum comparison
				allOK = 1;
				*dest = data;
				
			} else {									//check case of bit inversion
				
				data = ((l_bq.array & 0xffffffff00) >> 8);	// in CMD_SELECT_UID read -> (CON0 | CON1 | CON2 | RESERVED | CRC8)
				data <<= 1;
				crcR <<= 1;
				
				ptr = (uint8_t *)&data;
				crc = CRC_PRESET;							// initialize crc algorithm
			
				if(cmd == CMD_SELECT_UID) {
					data |= 0xc0000000;						// fudge patch. Receiver does not produce the 'c'!
				}
			
				for(i=3; i>=0; i--) {
					calc_crc(&crc, ptr[i], 8);				// compute crc
				}
				
				if( (crc == crcR) || (crc == (crcR | 0x01)) ){							//checksum comparison					
					allOK = 1;
					*dest = data;
				}
			}
		} else {
			*dest = ~l_bq.array;						//if not advanced, there is no checksum
			allOK = 1;			
		}
	}
	
	return(allOK);
}

static bit transceive(uint8_t cmd, uint8_t *data) {
	uint32_t uid;
	uint8_t crc, tmp;
	int8_t i;
	bit success, response;
	
	success = 0;
	response = 0;
			
	switch(cmd) {
		
		case CMD_TTF:					//TTF responds after certain carrier on time
		
#if !USE_MANCHESTER_TTF_ENCODING
			SET_MLX_MODE_BIPHASE();		//if HITAG is programmed to TTF, it can also be configured with either Manchester or Biphase encoding
#endif
			FMOD_OFF();
			MLX_MODU_ON();				//no data modulation for TTF so this just stays on
			response = 1;				//expecting response from tag
			l_rxLength = 64;			//tag pre-configured to transmit page 4 data. Will repeat so that actually read in the same data twice into local copy page 4 and page 5.
			SET_MLX_DATA_SPEED_2K();	//data rate pre-configured for 2k in tag
			l_sofBits = 1;
			_delay_ms(4);				//delay ~500TO try to avoid noise on clock
			
#if DEBUG_RFID_TEST
//			PRR &= ~(1<<PRTIM1);		//power up timer/counter 1 for pulse width measurements	
//			TCCR1B = TIMER1_DIV_CONST;	//see system.h for details
#endif			
			break;

		case CMD_UID_REQ_ADV:	
		case CMD_UID_REQ_STD:
			BitQueue_init(&l_bq, 5);	//set up data buffer for 5 bit command
			l_bq.array = cmd;
			l_bq.front = 5;				//force data to be queued
			
			SET_MLX_MODE_BIPHASE();		//use Biphase decoder for anti-collision coding
			SET_MLX_DATA_SPEED_4K();	//AC receive data rate is 2k but set filter for 4k for anti-collision coding.
			response = 1;				//expecting response from tag but it is AC encoded. '00' is 0b1 and '11' is 0b0
			l_rxLength = 64;			//uid is 32 bits, but will be 64bits in anti-collision coding
			
			transmit();					//contains 330TO carrier on time before modulating
			
//			success = 1;				//tag responds but Melexis chip cannot decode AC modulation, so just set as successful
			if(cmd == CMD_UID_REQ_ADV) {
				l_sofBits = 7;
				l_adv = 1;				//set ekey comms mode to advanced for subsequent communications
			} else {
				l_adv = 0;				//set ekey comms mode to standard for subsequent communications
				l_sofBits = 2;
			}
			_delay_ms(1);				//short delay before receiving to reduce effects of noise on receiver
			break;
			
		case CMD_SELECT_UID:
			uid = l_tagData.page0;
			crc = CRC_PRESET;				// initialize crc algorithm
			calc_crc(&crc, cmd<<(8-5), 5);	// compute 5 crc Bits only	
			tmp = (uid & 0xff000000)>>24;
			calc_crc(&crc, tmp, 8);
			tmp = (uid & 0x00ff0000)>>16;
			calc_crc(&crc, tmp, 8);
			tmp = (uid & 0x0000ff00)>>8;
			calc_crc(&crc, tmp, 8);
			tmp = uid & 0x000000ff;			
			calc_crc(&crc, tmp, 8);			//crc computed
			
			BitQueue_init(&l_bq, 45);
			l_bq.array = uid;			//load in the HITAG device's UID
			l_bq.array <<= 8;			//push it over for the CRC
			l_bq.array |= crc;			//put in the CRC
			l_bq.front = 45;			//this is the total number of bit in the transmit packet
			
			SET_MLX_MODE_MANCH();		//use manchester for select UID
			SET_MLX_DATA_SPEED_4K();	//data rate is 4k for SELECT UID std and adv
			transmit();					//send if out
			
			response = 1;				//expecting a response
							
			if(l_adv) {					//received data for advanced mode
				l_sofBits = 6;
				l_rxLength = 5*8;		// ( CON0 | CON1 | CON2 | RESERVED | CRC8 ) expected
			} else {					//received data for standard mode
				l_sofBits = 1;
				l_rxLength = 4*8;		// ( CON0 | CON1 | CON2 | RESERVED ) expected
			}
			_delay_ms(1);				//short delay before receiving to reduce effects of noise
			break;
			
		case CMD_WR_PAGE:					//this is actually similar to a 'write enable' command
			crc = CRC_PRESET;				// initialize crc algorithm
			calc_crc(&crc, cmd<<(8-4), 4);	// compute 4 crc Bits only for write page
			calc_crc(&crc, *data, 8);		//this is the page address
			
			BitQueue_init(&l_bq, 20);
			l_bq.array = ((uint32_t)cmd<<16);	//command byte into the buffer
			l_bq.array |= ((uint16_t)*data<<8);	//page address into the buffer
			l_bq.array |= crc;					//crc into the buffer
			l_bq.front = 20;					//total length of transmit message
			l_wrCmd = l_bq.array;				//for debug examination of the write command
			SET_MLX_DATA_SPEED_4K();			//data rate is 4k for write page std and adv
			transmit();							//transmit the data
			
			response = 1;				//response expected
			if(l_adv) {					//for advanced mode
				l_sofBits = 6;
			} else {
				l_sofBits = 1;
			}
			l_rxLength = 2;				// 01 (ack) expected
			_delay_ms(1);				//short delay before receiving to reduce effects of noise
			break;
			
		case CMD_RD_PAGE:
//		case CMD_RD_BLOCK:
			crc = CRC_PRESET;				//initialize crc algorithm
			calc_crc(&crc, cmd<<(8-4), 4);	//compute 4 crc Bits only for write page
			calc_crc(&crc, *data, 8);		//this is the page address
			
			BitQueue_init(&l_bq, 20);		//20 bits to transmit
			
			l_bq.array = ((uint32_t)cmd<<16);
			l_bq.array |= ((uint16_t)*data<<8);
			l_bq.array |= crc;
			l_bq.front = 20;				//force data to be queued
			l_readPageCmd = l_bq.array;
			
			SET_MLX_DATA_SPEED_4K();	//rx data rate is 4k for READ PAGE std and adv
			
			transmit();						//transmit the data
	
			response = 1;					//response expected
			
			if(l_adv) {
				l_sofBits = 6;
				l_rxLength = 5*8;			// ( DATA0 | DATA1 | DATA2 | DATA3 | CRC8 ) expected
			} else {
				l_sofBits = 1;
				l_rxLength = 4*8;			// ( DATA0 | DATA1 | DATA2 | DATA3 ) expected
			}
			_delay_ms(1);				//short delay before receiving to reduce effects of noise
//			TEST_LO();
			break;
			
		case WR_PAGE_DATA:
			crc = CRC_PRESET;			//initialize crc algorithm
			for( i=3; i>=0; i--) {
				calc_crc(&crc, data[i], 8);		
			}
			
			BitQueue_init(&l_bq, 5*8);	// ( DATA0 | DATA1 | DATA2 | DATA3 | CRC8 )
			l_bq.array = *((uint32_t *)data);
			l_bq.array <<= 8;
			l_bq.array |= crc;
			l_bq.front = 5*8;

			SET_MLX_DATA_SPEED_4K();	//data rate is 4k for write page std and adv
			transmit();
//			TEST_LO();
			
			response = 1;				//response expected
			if(l_adv) {					//for advanced mode
				l_sofBits = 6;
			} else {
				l_sofBits = 1;
			}
			
			l_rxLength = 2;				// 01 'ack' expected
			_delay_ms(3);				//short delay before receiving to reduce effects of noise
										//Tprog programming delay
			break;
			
		default:
			break;	
	}
	
	if(response) {
		if( receive() ) {
			success = 1;
		}
	}
	
	PRR |= (1<<PRTIM1);		//power down timer/counter 1	

//	TEST_HI();
	
#if !USE_MANCHESTER_TTF_ENCODING
	if(cmd == CMD_TTF) {
#if DEBUG_RFID_TEST
		l_pwAccum = 0;
		
		for(i=2; i<64; i++) {
			l_pwAccum += l_pwBuff[i];
		}
		
		l_pwF = (float)l_pwAccum/(float)62.0;
		l_TOF = l_pwF/64.0;
//static uint16_t l_22TO, l_14TO, l_6TO, l_40TO, l_330TO;	//TODO- calculate all these
#endif		
		SET_MLX_MODE_MANCH();		
	}
#endif
	return(success);
}

static void transmit(void) {
	bit txDone;
		
	PRR &= ~(1<<PRTIM1);		//power up timer/counter 1	
	TCCR1B = 0;					//timer is off and not running
	TCCR1A = 0;					//nothing fancy. Basic operation	
	
	TCNT1 = 0;					//set 16-bit counter to 0
	OCR1B = 330*TO;				//compare value to trigger interrupt (initial carrier on time) Twfc
	l_compareLoadValue = OCR1B + 6*TO;	//compare value for the carrier off portion of the first bit
	
	TIFR1 = (1<<OCF1B);			//clear interrupt flag
	TIMSK1 = (1<<OCIE1B);		//compare interrupt enabled

	TCCR1B = TIMER1_DIV_CONST;	//see system.h for details

	l_carrierOn = 1;			//flags for interrupts
	l_eofBit = 0;
	l_intFlg = 0;
	FMOD_OFF();
	MLX_MODU_ON();				//turn on carrier if not already on
	
	txDone = 0;					//default is 'not done'
		
	while(!txDone) {
		
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_enable();
		sleep_cpu();
		sleep_disable();			
		
		if(l_carrierOn) {
			l_compareLoadValue = OCR1B + 6*TO;	//this is duration of carrier 'off'		
		} else {							//zero portion of a new bit started
			if(BitQueue_getLength(&l_bq) == 0) {
				if(l_eofBit) {
					FMOD_OFF();
					MLX_MODU_ON();				//keep carrier going
					txDone = 1;					//transmit has completed
					TIMSK1 &= ~(1<<OCIE1B);		//compare interrupt for transmitter disabled. 				
				} else {
					l_compareLoadValue = OCR1B + 40*TO;	//'EOF' end of transmission
					l_eofBit = 1;
				}
			} else {
				if(BitQueue_dequeue(&l_bq)) {			//bit is a 'one'
					l_compareLoadValue = OCR1B + 22*TO;		
				} else {								//bit is a 'zero'
					l_compareLoadValue = OCR1B + 14*TO;
				}
			}
		}	
	}
}

static bit receive(void) {
	bit rxDone, success;

#if DEBUG_RFID_TEST	
	l_bufIdx = 0;//for calculating the pw average
#endif
	BitQueue_init(&l_bq, l_rxLength);
	
	Timer_enableTimerModule(1);
	
	// this is unconditional timeout of receive process
	Timer_init(&timer2,
	(uint16_t)(EKEY_RX_TIMEOUT*TICKS_PER_SECOND));    //time-out value
	//        (uint16_t)(4*TICKS_PER_SECOND));
	Timer_start(&timer2);
	
	l_timeout = 0;				//for debugging detection of timeout
	
	BIT_SET(PCMSK0, PCINT0);	//enable Melexis clock interrupt (PB0)
	BIT_SET(PCIFR, PCIF0);		//clear interrupt flag for PCINT7..PCINT0
	BIT_SET(PCICR, PCIE0);		//enable PCINT7..PCINT0
	receive_flag = 1; // f100
	l_intFlg = 0;
	rxDone = 0;
	success = 0;

	while(!rxDone) {
		//set_sleep_mode(SLEEP_MODE_IDLE);
		//sleep_enable();
		//sleep_cpu();
		//sleep_disable();
		
		if(l_intFlg) {
			l_intFlg = 0;
			if(BitQueue_getLength(&l_bq)>=l_rxLength) {
				rxDone = 1;
				success = 1;
			}
		} else {
			if(Timer_elapsed(&timer2)) {
				rxDone = 1;
				if(BitQueue_getLength(&l_bq) >= (l_rxLength-2) ) {
					l_bq.array <<= 1;
					success = 1;
				}
				l_timeout = 1;
			}
		}
	}
	
	BIT_CLR(PCMSK0, PCINT0);	//mask interrupt from Melexis clock pin
	BIT_CLR(PCICR, PCIE0);		//disable PCINT7..PCINT0
	receive_flag = 0; // f100
	Timer_init(&timer2, 1000); //resets/stops timer	
	Timer_enableTimerModule(0);

	return(success);	
}

/*
* crc routine taken from HITAG S data sheet
*/
static void calc_crc(unsigned char * crc, unsigned char data, unsigned char Bitcount) {

	*crc ^= data; // crc = crc (exor) data
	do {
		if( *crc & 0x80 ) { // if (MSB-CRC == 1)			
			*crc<<=1; // CRC = CRC Bit-shift left
			*crc ^= CRC_POLYNOM; // CRC = CRC (exor) CRC_POLYNOM
		} else {
			*crc<<=1; // CRC = CRC Bit-shift left
		}
	} while(--Bitcount);
}

uint8_t Ekey_crcTest(void) {
	const uint8_t cmd=0x0c; /* 5 Bit command, aligned to MSB. This is select(UID) command */
//	const uint8_t ident[4]={0x2C, 0x68, 0x0D, 0xB4 };	//order matters!
//	const uint8_t ident[4]={0x0D, 0xB4, 0x2C, 0x68 };
//	const uint8_t ident[4]={0x00, 0x00, 0x00, 0x00};
	const uint8_t ident[1]={0x05};
	unsigned char crc;
	int i;
	
	crc = CRC_PRESET; /* initialize crc algorithm */
	calc_crc(&crc, cmd, 4); /* compute 5 crc Bits only */
	for(i=0; i<1; i++)
	calc_crc(&crc, ident[i], 8);
	/* crc = 0x9E at this point */
	return(crc);
}

/*
uint32_t Ekey_getUID(void) {
	return(l_uid);
}
*/
TAG_DATA *Ekey_getTagData() {
	return(&l_tagData);
}

uint32_t Ekey_getAltUid(void) {
	return l_altUid1;
}

//-------------------------j100------------------------
uint32_t Ekey_getAltUid2(void) {
	return l_altUid2;
}
uint32_t Ekey_getAltUid3(void) {
	return l_altUid3;
}
uint32_t Ekey_getAltUid4(void) {
	return l_altUid4;
}
uint32_t Ekey_getAltUid5(void) {
	return l_altUid5;
}
//-------------------------j100------------------------

uint32_t l_decodeUidRequest() {
	uint32_t retval;
	
	retval = 0;
	
	l_decode(&(l_bq.array), &retval);
	
	l_bq2.array = l_bq3.array = l_bq4.array = l_bq5.array= l_bq.array; // j100
		
	l_bq.array >>= 1;
	l_decode(&(l_bq.array), &l_altUid1);	
	
	//--------------------j100---------------------------------
	l_bq2.array&=0xFFFFFFFFFFFFFFF0;
	l_decode(&(l_bq2.array), &l_altUid2); // for code 11
	
	l_bq3.array&=0xFFFFFFFFFFFFFFF0;
	l_bq3.array|=0x0000000000000003;
	l_decode(&(l_bq3.array), &l_altUid3); // for code 10
	
	l_bq4.array&=0xFFFFFFFFFFFFFFF0;
	l_bq4.array|=0x0000000000000008;
	l_decode(&(l_bq4.array), &l_altUid4); // for code 01
	
	l_bq5.array&=0xFFFFFFFFFFFFFFF0;
	l_bq5.array|=0x000000000000000F;
	l_decode(&(l_bq5.array), &l_altUid5); // for code 00
	//--------------------j100---------------------------------		
	
	return(retval);
}

uint8_t l_decode(uint64_t *uid, uint32_t *retval) {
	uint64_t maskVal;
	uint8_t testVal;
	uint8_t i;
	uint8_t bitErrCtr;
	
	bitErrCtr = 0; i = 0;
	
	while ( ( bitErrCtr <= 2) & (i<32) ) {
		
		maskVal = (uint64_t)0b11 << ((32-i)*2-2);
		testVal = (uint8_t)((*uid & maskVal) >> (((32-i)*2)-2) );
		*retval <<= 1;
		
		if( (testVal == 0) || (testVal == 2) ) {
			*retval |= 1;
		} else {
			if(testVal != 3) {
				bitErrCtr++;	
			}
		}
		i++;
	}
	return(bitErrCtr);	
}
// interrupt for RFID receiver
ISR (PCINT0_vect) {
	//if(StateMachine_currentState ==ST_BAG_INSERTED)   // then receive
	if(receive_flag) // when interrupt due to clock receive function
{
		
	
	if(BIT_IS_SET(PINB, PINB0)) {	//clock has to be high for valid data		

#if DEBUG_RFID_TEST		
		l_thisCounter = (uint16_t)TCNT1;		
		if(l_sofBits == 0) {		//skip some clock transitions at message beginning
			l_pwBuff[l_bufIdx++] = l_thisCounter - l_lastCounter;
#else
		if(l_sofBits == 0) {		//skip some clock transitions at message beginning
#endif
			if(BitQueue_getLength(&l_bq)<l_rxLength) {

				if(BIT_IS_SET(PINB, PINB2)) {	//data
					BitQueue_enqueue(&l_bq, 1);
				} else {
					BitQueue_enqueue(&l_bq, 0);
				}	
				l_intFlg = 1;	
			}		
		} else {
			l_sofBits--;
		}
#if DEBUG_RFID_TEST		
		l_lastCounter = l_thisCounter;
#endif
	}
}
			
	//------------------------------------f100------------------------------------------
else {	 // motor OVLD interrupt
	
	//if(StateMachine_currentState == ST_MOTOR_RUN)
	//{
		if(bit_is_clear(PINB, PINB5)) // check the status of line if low then ovld
		{ 
			//_delay_ms(10);
			//RED_LED_ON();
			//_delay_ms(40);
			//RED_LED_OFF();
			//if(bit_is_clear(PINB, PINB5)) EventStack_push(EVENT_MOTOR_OVERLOAD);	
			BIT_SET(System_interruptFlags, SYSTEM_INTFLG_MOTOR_OVERLOAD);	// OVLD bit set
		}
	//}
     }
	//------------------------------------f100------------------------------------------

}

// interrupt for RFID transmitter
ISR (TIMER1_COMPB_vect) {
	
	OCR1B = l_compareLoadValue;
	
	if(l_carrierOn) {
		if(!l_eofBit) {
//			TEST_LO();
#if  !DEBUG_DISABLE_FMOD
			FMOD_ON();
#endif			
			MLX_MODU_OFF();	
		}
	} else {
//		TEST_HI();
		FMOD_OFF();
		MLX_MODU_ON();
	}
	
	l_carrierOn = !l_carrierOn;
	
}
