/*
 * SmartLink.h
 *
 * Created: 10/25/2016 10:27:13 AM
 *  Author: mikev
 */ 


#ifndef SMARTLINK_H_
#define SMARTLINK_H_

#include "System.h"

#define SMARTLINK_RX_PACKET_ID	0x11   
//#define SMARTLINK_RX_PACKET_ID	0x78  // 'x'  for debug  R100
#define SMARTLINK_DISPENSE_CODE	0x21
#define SMARTLINK_VALVE_ON_CODE	0x22  // e100
#define SMARTLINK_VALVE_OFF_CODE 0x23 // e100
#define SMARTLINK_QUERY_CODE	0x00

typedef struct {
	union {
		struct {
			union {
				struct {
					uint8_t			config;
					uint8_t			deviceId2;
			
					union {
						struct {
							uint8_t		valveOpen	:1;	//b0
							uint8_t		ekeyPresent	:1;
							uint8_t		coverOpen	:1;
							uint8_t		ntusd20		:1;
							uint8_t		ntusd21		:1;
							uint8_t		ntusd22		:1;
							uint8_t		ntusd23		:1;							
							uint8_t		ntusd24		:1;	//b7
						};
						uint8_t			refillSysStats;
					};
					
					uint8_t			soapLevel;
					uint8_t			packedData[3];
		//			uint8_t			cr2;
		//			uint8_t			lf2;
			
				};
				uint8_t		packet2[7];
			};
	//////////////////////////////////////////////////////	
			union {
				struct {
					uint8_t			refillVersion;
					uint8_t			deviceId1;
					uint8_t			status2;
					uint8_t			refillSerial[4];
		//			uint8_t			cr1;
		//			uint8_t			lf1;
				};
				uint8_t		packet1[7];
			};
	//////////////////////////////////////////////////////
			union {
				struct {
					uint8_t			version;
					uint8_t			deviceId0;
					uint8_t			distCode;
			
					union {
						struct {
							uint8_t		noEOS		:1;	//b0
							uint8_t		refillErr	:1;
							uint8_t		ntusd00		:1;
							uint8_t		battDead	:1;
							uint8_t		battLow		:1;
							uint8_t		highCurrent	:1;
							uint8_t		refillLow	:1;							
							uint8_t		ntusd01		:1;	//b7
						};
						uint8_t	status;
					};
					
					uint8_t			notUsed[3];	
		//			uint8_t			cr0;
		//			uint8_t			lf0;
		
				};
				uint8_t		packet0[7];
			};
		};
		uint8_t bytes[21];
	};	
} SL_TX_PACKET;

typedef union {
	struct {
		uint8_t		sof;	
		uint8_t		config[2];
		uint8_t		led[4];
	//	uint8_t		cr0;
	//	uint8_t		lf0;
	};
	uint8_t	bytes[7];
} SL_RX_PACKET;

void SmartLink_serializeTxPacket(char *dest);
void SmartLink_test(uint16_t *refillAmt, uint16_t *numDisp);

#endif /* SMARTLINK_H_ */