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
					uint8_t			ntused01;
					uint8_t			ntused02;
					uint8_t			ntused03;
					uint8_t			ntused04;
					uint8_t			ntused05;
					
					union {
						struct {
							uint8_t		ntusedbit0		:1;	//b0
							uint8_t		ntusedbit1		:1;
							uint8_t		ntusedbit2		:1;
							uint8_t		ntusedbit3		:1;
							uint8_t		ntusedbit4		:1;
							uint8_t		ntusedbit5		:1;
							uint8_t		ntusedbit6		:1;
							uint8_t		ntusedbit7		:1;	//b7
						};
						uint8_t	ntused06;
					};
				};
				uint8_t		packet3[7];
			};
		};
		uint8_t dbytes[7];
	};
} DB_TX_PACKET;


void Debug_serializeTxPacket(char *dest);


#endif