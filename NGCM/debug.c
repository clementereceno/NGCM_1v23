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
	db_txData.ntused01 = 0;
	db_txData.ntused03 = 0;
	db_txData.ntused04 = 0;
	db_txData.ntused05 = 0;
	db_txData.ntused06 = 0;
	
}