/*
 * main.h
 *
 * Created: 3/4/2022 10:31:22 AM
 *  Author: clemente
 */ 

#ifndef DEBUG_H_
#define DEBUG_H_

#include "System.h"



typedef struct {
	union {
		struct {
			union {
				struct {
					int8_t			weightReceived;
					int8_t			SoapRemainingWeight;
					int8_t			SoapRemainingFills;
					uint8_t			ReservoirWeightNow;
					uint8_t			initialWeight;
					int8_t			deltaWeight;
					int8_t			newWeightUnitsDifference;
					uint16_t		unitsReceivedCtr;
					uint8_t			ntused09;
					uint8_t			ntused10;
					
					union {
						struct {
							uint8_t		bagGoodFlag		:1;	//b0
							uint8_t		deltaWeightIncreasing		:1;  //have more units been added since last check?
							uint8_t		ntusedbit2		:1;
							uint8_t		ntusedbit3		:1;
							uint8_t		ntusedbit4		:1;
							uint8_t		ntusedbit5		:1;
							uint8_t		ntusedbit6		:1;
							uint8_t		ResetKey		:1;	//b7
						};
						uint8_t	status;
					};
				};
				uint8_t		packet3[12];
			};
		};
		uint8_t dbytes[12];
	};
} DB_TX_PACKET;


void Debug_serializeTxPacket(char *dest);


#endif