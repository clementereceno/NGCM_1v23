/*
 * debug.c
 *
 * Created: 3/7/2022 10:06:07 PM
 *  Author: clemente
 */ 
#include "System.h"
#include "debug.h"
#include "Util.h"
#include <string.h>	//for memset
#include <stddef.h>

DB_TX_PACKET db_txData;

extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;
extern DEBUG_DATA dbgData;
extern DEBUG_FLAGS dbgFlags;

static void db_setTxPacket(void);

void Debug_serializeTxPacket(char *dest) {
	char buf[2*sizeof(db_txData.packet3)+1];		//+1 for the null terminator
	
	db_setTxPacket();
	
	Util_bytesToString(dest, (uint8_t *)&db_txData.packet3, sizeof(db_txData.packet3));
	Util_strcat(dest, "\r\n");
	
}


void db_setTxPacket(void) {
	
	memset(&db_txData, 0, sizeof(DB_TX_PACKET));	//zero the whole thing
	
	
	//assign here need to print
	db_txData.weightReceived = System_fillStats.weightUnitsReceived;
	db_txData.SoapRemainingWeight = System_fillStats.ekeyRemainingWeightUnits;
	db_txData.SoapRemainingFills = System_fillStats.ekeyRemainingFills;	
	db_txData.ReservoirWeightNow = sysData.currentSoapWeight;
	db_txData.initialWeight = System_fillStats.initialWeight;	
	
	db_txData.status = dbgFlags.status;
	db_txData.deltaWeight = dbgData.deltaWeight;
	db_txData.newWeightUnitsDifference = dbgData.newWeightUnitsDifference;
	db_txData.unitsReceivedCtr = sysData.unitsReceivedCtr;
	
	
	System_refreshEepromData(&sysData.eepromData);
}