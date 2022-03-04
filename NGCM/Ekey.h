/*
 * Ekey.h
 *
 * Created: 10/22/2015 2:14:40 PM
 *  Author: mikev
 */ 


#ifndef EKEY_H_
#define EKEY_H_
#include "System.h"

typedef struct {
	uint32_t page7;
	uint32_t page6;
	
	union {
		struct {
			//page 5
			uint8_t		bagIdentifier;	//most significant byte
			uint8_t		notUsed;
			int8_t		useCounter;
			int8_t		weightCounter;	//least significant byte
		};
		//page 4
		uint32_t	page5;			//this is byte reversed
	};
	
	uint32_t page4;
	uint32_t page3;
	uint32_t page2;
	uint32_t page1;
	uint32_t page0;
	
} TAG_DATA;

typedef struct {
	union {
		struct {
			uint8_t	b1_3;
			uint8_t b1_2;
			uint8_t b1_1;
			uint8_t b1_0;
		};
		uint32_t page1;
	};
	union {
		struct {
			uint8_t b0_3;
			uint8_t b0_2;
			uint8_t b0_1;
			uint8_t b0_0;
		};
		uint32_t page0;
	};
} BIT_BUFFER_64;

typedef union {
	uint8_t arr[4];
	uint32_t word32;
} WORD32;

#define CMD_UID_REQ_STD		(0b00110)		//not bit reversed!
#define CMD_UID_REQ_ADV		(0b11000)
#define CMD_UID_REQ_FDV		(0b11010)
#define CMD_TTF				(0b11111)		//this is not a real code. For selecting TTF
#define CMD_SELECT_UID		(0b00000)
#define CMD_WR_PAGE			(0b1000)
#define CMD_RD_PAGE			(0b1100)
#define CMD_RD_BLOCK		(0b1101)
#define WR_PAGE_DATA			0x01

#define EKEY_RX_TIMEOUT (0.07)

uint8_t Ekey_crcTest(void);

#define EKEY_RECEIVE_RESPONSE 1
#define EKEY_TRANSMIT_ONLY	0

bit Ekey_uidRequest(void);
//uint8_t Ekey_hitagSelect(void);
uint8_t Ekey_doTTF(void);
bit Ekey_readAll(void);
uint8_t Ekey_dockUsage(int howManyRefillUnits, int howManyRefillUses);
uint8_t Ekey_crcTest(void);
TAG_DATA *Ekey_getTagData(void);
bit Ekey_readPage(uint8_t cmd, uint8_t *pageNum, uint32_t *dest);
bit Ekey_program(void);
uint32_t Ekey_getAltUid(void);
//---------------------j100-------------------------------
uint32_t Ekey_getAltUid2(void);
uint32_t Ekey_getAltUid3(void);
uint32_t Ekey_getAltUid4(void);
uint32_t Ekey_getAltUid5(void);
//---------------------j100-------------------------------

#endif /* EKEY_H_ */