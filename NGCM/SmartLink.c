/*
 * l.c
 *
 * Created: 10/25/2016 10:43:34 AM
 *  Author: mikev
 */
 
#include "System.h"
#include "SmartLink.h"
#include "Util.h"
#include <string.h>	//for memset
#include <stddef.h>

#define CR_LF	"\r\n"

extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;

static void l_setTxPacket(void);
static void l_packCountFields(uint8_t *buf, uint16_t litres, uint16_t dispenses);
static void l_unpackCountFields(uint8_t *buf, uint16_t *litres, uint16_t *dispenses);

SL_TX_PACKET l_txData;

void SmartLink_serializeTxPacket(char *dest) {
	char buf[2*sizeof(l_txData.packet0)+1];		//+1 for the null terminator
	
	l_setTxPacket();
	
	Util_bytesToString(dest, (uint8_t *)&l_txData.packet0, sizeof(l_txData.packet0));	
	Util_strcat(dest, CR_LF);
	
	Util_bytesToString(buf, (uint8_t *)&l_txData.packet1, sizeof(l_txData.packet1));
	Util_strcat(dest, buf);
	Util_strcat(dest, CR_LF);

	Util_bytesToString(buf, (uint8_t *)&l_txData.packet2, sizeof(l_txData.packet2));
	Util_strcat(dest, buf);
	Util_strcat(dest, CR_LF);
}

void l_setTxPacket(void) {
	
	memset(&l_txData, 0, sizeof(SL_TX_PACKET));	//zero the whole thing
	
	l_txData.deviceId0 = 0x05;		//wtf?
	l_txData.deviceId1 = 0x50;		//wtf?
	l_txData.deviceId2 = 0x51;		//wtf?
	
	l_txData.status = sysFlags.status;
	l_txData.refillSysStats = sysFlags.refillSysStats;
//	l_txData.distCode = sysData.currentDistCode;
	l_txData.distCode = sysData.eepromData.distributorCode;
	//l_txData.distCode = 0x02;
	l_txData.soapLevel = sysData.currentSoapWeight;
	
	System_refreshEepromData(&sysData.eepromData);
	
	l_packCountFields((uint8_t *)&(l_txData.packedData), sysData.eepromData.refillAmt, sysData.eepromData.numKdispenses);	//pack these into 12-bit quantities	
}

void l_packCountFields(uint8_t *buf, uint16_t litres, uint16_t dispenses) {
	*buf = litres & 0xff;
	*(buf+1) = ((litres & 0x0f00) >> 8) | ((dispenses & 0x0f) << 4);
	*(buf+2) = ((dispenses & 0x0ff0) >> 4);
}

void l_unpackCountFields(uint8_t *buf, uint16_t *litres, uint16_t *dispenses) {
	*dispenses = *(buf+2) << 4;
	*dispenses |= ((*(buf+1) & 0xf0) >> 4);
	
	*litres = *buf;
	*litres |= ((*(buf+1) & 0x00f) << 8);
}

void SmartLink_test(uint16_t *refillAmt, uint16_t *numDispenses) {
	
	*refillAmt = 0x123;
	*numDispenses = 0x456;
	
	l_packCountFields((uint8_t *)&(l_txData.packedData), *refillAmt, *numDispenses);
	
	l_unpackCountFields((uint8_t *)&(l_txData.packedData), refillAmt, numDispenses);
	refillAmt++;
	numDispenses++;
}