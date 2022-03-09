/*
 * system.h
 *
 *  Created on: Jul 22, 2015
 *      Author: mikev
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <inttypes.h>
#include <stdbool.h>
#include <avr/io.h>

//#define FW_VERSION	"1.03"
//#define FW_VERSION	"1.06"		//in-house testing March 24 2017
//#define FW_VERSION	"1.07"		//in-house testing April 3 2017
//#define FW_VERSION	"1.09"			//added ekey error flash April 26 2017. Fixed error with BATT_VOLTAGE_RATIO based on change of R39 to 1.2M.
									//added ekey reset functionality. Potentially used for prototype production run. Added switch to skip no soap lockout.
//#define FW_VERSION	"1.10"			//- ekey present status implemented
                                    //- cover open status was cleared
                                    //- EOS status was set but was cleared before sending the tx packet
//#define FW_VERSION	"1.11"			//- valve drive pulse change

//#define FW_VERSION	"1.12"			//- valve drive pulse change back to 200ms and valve ON 0x22 and Valve OFF 0x23 command added in smartlink for test
//#define FW_VERSION	"1.13"			// ekey issue( intermittent read) - updated the RF freqn to 125khz(previous 118khz in all revisions)and commented the sleep related code in receive() function
//#define FW_VERSION	"1.14"			// Motor Current sense logic implementation                                  
//#define FW_VERSION	"1.15"			// issue of OVLd status not updating in smartlink corrected - check for reference g100 for updates
//#define FW_VERSION	"1.16"			// valve drive time changed to 25ms ( previous versions had 200ms) - check for reference h100 for updates
//#define FW_VERSION	"1.17"	   // refill lockout enabled check references i100
//#define FW_VERSION	"1.18"	   // refill lockout kept enabled check references j100
//#define FW_VERSION	"1.19"	   // only page 5 read 6 times in ekey function and communication tries to 10 previous 5 // check references gk100 for updates
//#define FW_VERSION	"1.20"	   // use two unique memory locations for the strain gauge offset and for the lifetime use counter and for 1.2l bag its 0x258 (600 decimal) 
                              // check references 19aug19 and 22aug19 for updates
							  
//#define FW_VERSION	"1.21"	 //	nominal value gain for 2.2l bag is 0x320 (800 decimal) // check references 19aug19_2 for updates
#define FW_VERSION	"1.23"	 //	nominal value gain for 2.2l bag is 0x320 (800 decimal) // check references 19aug19_2 for updates

#define DEBUG_RFID_TEST			0
#define DEBUG_SKIP_OPTO_CAL_DELAY 0
#define DEBUG_DISABLE_FMOD		0
#define DEBUG_SKIP_EKEY_CHECK	0
#define DEBUG_EKEY_TESTER_MODE	0
#define DEBUG_SKIP_USAGE_DOCK	1
#define DEBUG_SKIP_WEIGHT_DOCK	0
#define DEBUG_SKIP_SOAP_LOCKOUT	0	// 1 = refill valve will open even if the usage counter has zero units / 0 = refill valve will lockout //i100
#define DEBUG_SKIP_LV_CHECKS	0

#define DEBUG_SERIAL_SPIT_EN	1
#define DEBUG_WRTBCKFULL2EKEY	0

//#define DEBUG_OP_AMP_ON
//#define DEBUG_ENABLE_SILLY_LIGHT_SHOW
#define DEBUG_BYPASS_MINIMUM_VALVE_CYCLE_TIME 0
//#define DEBUG_RFID_TEST_UID ((uint32_t)0x750d9081)
//#define DEBUG_RFID_TEST_UID ((uint32_t)0x190d9081)
#define DEBUG_CRYPTO_DISABLE

//#define EKEY_COMM_TRIES		5  //v1.18 <=
#define EKEY_COMM_TRIES		10    // v1.19  // gk100  

#if DEBUG_EKEY_TESTER_MODE
#undef EKEY_COMM_TRIES
#define EKEY_COMM_TRIES	2
#undef DEBUG_BYPASS_MINIMUM_VALVE_CYCLE_TIME
#define DEBUG_BYPASS_MINIMUM_VALVE_CYCLE_TIME 1
#undef DEBUG_SKIP_USAGE_DOCK
#define DEBUG_SKIP_USAGE_DOCK 1
#undef DEBUG_SKIP_WEIGHT_DOCK
#define DEBUG_SKIP_WEIGHT_DOCK 1
#endif

#if DEBUG_SERIAL_SPIT_EN
#undef DEBUG_SERIAL_SPIT
#define DEBUG_SERIAL_SPIT	1
#endif
//Select CPU frequency here. Maximum is 8MHz
#define F_CPU_BASE	8000000UL

#define F_CPU_8MHZ
//#define F_CPU_1MHZ

#ifdef F_CPU_8MHZ
#define CPU_PRESCALER	0
#endif

#ifdef F_CPU_1MHZ
#define CPU_PRESCALER	3
#endif

#define F_CPU	(F_CPU_BASE<<CPU_PRESCALER)

#include <util/delay.h>

typedef bool bit;
#define BIT_SET(x,y) (x |= _BV(y)) //set bit - using bitwise OR operator
#define BIT_CLR(x,y) (x &= ~(_BV(y))) //clear bit - using bitwise AND operator
#define BIT_TOG(x,y) (x ^= _BV(y)) //toggle bit - using bitwise XOR operator
#define BIT_IS_SET(x,y) ((x & _BV(y)) == _BV(y)) //check if the y'th bit of register 'x' is

//#define BQ_BUFFER_TYPE uint8_t
#define BQ_BUFFER_TYPE uint64_t

uint8_t System_interruptFlags;
//uint8_t eepromDataDirty2=0; //22aug19

typedef volatile union {
	struct {
		union {
			struct {
				uint8_t	noEOS		:1;	//b0
				uint8_t	refillErr	:1;	//wrong ekey code
				uint8_t	st_ntusd0	:1;
				uint8_t	battDead	:1;
				uint8_t	battLow		:1;
				uint8_t	highCurrent	:1;
				uint8_t	refillLow	:1;
				uint8_t	st_ntusd1	:1;	//b7
			};
			uint8_t status;
		};
		
		union {
			struct {
				uint8_t valveOpen	:1;	//b0
				uint8_t ekeyPresent	:1;
				uint8_t coverOpen	:1;
				uint8_t rf_ntusd0	:1;
				uint8_t rf_ntusd1	:1;
				uint8_t rf_ntusd2	:1;
				uint8_t rf_ntusd3	:1;
				uint8_t rf_ntusd4	:1;	//b7
			};
			uint8_t refillSysStats;
		};
		
		uint8_t lockSerialMode			:1;
		uint8_t unlockSerialModeFlag	:1;
		uint8_t dispensePending			:1;
		uint8_t	restarted				:1;
		uint8_t eepromDataDirty			:1;
		uint8_t rtlsActive				:1;
		uint8_t usingResetKey			:1;
	};
	uint32_t allFlags;
} SYSTEM_FLAGS;

typedef volatile union {
	struct {
		union {
			struct {
				uint8_t	bagGoodFlag				:1;	//b0
				uint8_t	deltaWeightIncreasing	:1;	//
				uint8_t	ntusedbit2	:1;
				uint8_t	ntusedbit3	:1;
				uint8_t	ntusedbit4		:1;
				uint8_t	ntusedbit5	:1;
				uint8_t	ntusedbit6	:1;
				uint8_t	ResetKey	:1;	//b7
			};
			uint8_t status;
		};
		
		union {
			struct {
				uint8_t ntusedbit8	:1;	//b0
				uint8_t ntusedbit9	:1;
				uint8_t ntusedbit10	:1;
				uint8_t ntusedbit11	:1;
				uint8_t ntusedbit12	:1;
				uint8_t ntusedbit13	:1;
				uint8_t ntusedbit14	:1;
				uint8_t ntusedbit15	:1;	//b7
			};
			uint8_t ntusedStats;
		};
		
		uint8_t ntusedbit16			:1;
		uint8_t ntusedbit17			:1;
		uint8_t ntusedbit18			:1;
		uint8_t	ntusedbit19			:1;
		uint8_t ntusedbit20			:1;
		uint8_t ntusedbit21			:1;
		uint8_t ntusedbit22			:1;
	};
	uint32_t allFlags;
} DEBUG_FLAGS;

#define SYSTEM_INTFLG_PCINT2			((uint8_t)1)
#define SYSTEM_INTFLG_MOTOR_OVERLOAD	((uint8_t)2)
#define SYSTEM_INTFLG_PCINT0			((uint8_t)3)
#define SYSTEM_INTFLG_T1_COMPB			((uint8_t)4)
#define SYSTEM_INTFLG_WDT				((uint8_t)5)
#define SYSTEM_INTFLG_UARTRX			((uint8_t)6)
#define SYSTEM_INTFLG_UARTTX			((uint8_t)7)

#define EVENT_STACK_DEPTH		(5)

#define EVENT_MOTOR_ON			((uint8_t)0)
#define EVENT_MOTOR_OVERLOAD	((uint8_t)1)
#define EVENT_BAG_INSERTED		((uint8_t)2)
#define EVENT_BAG_REMOVED		((uint8_t)3)
#define EVENT_EOS_OPEN			((uint8_t)4)
#define EVENT_EOS_CLOSED		((uint8_t)5)
#define EVENT_WDT				((uint8_t)6)
#define EVENT_TIMER1            ((uint8_t)7)
#define EVENT_TIMER2            ((uint8_t)8)
#define EVENT_UARTRX			((uint8_t)9)
#define EVENT_TIMER3            ((uint8_t)10)
#define EVENT_TIMER4            ((uint8_t)11)

#define PUMP_MOTOR_TIMEOUT				(2.0)	//pump motor timeout in seconds
#define PUMP_MOTOR_BREAK_TIME_MS		(100)
#define BAG_INSERTED_INACTIVITY_TIMEOUT	(120)
#define BAG_OUT_VALVE_CLOSE_DELAY		(3)

#define SOAP_LOW_FLASH_PERIOD			(WDT_2S)
#define ERROR_FLASH_PERIOD				(WDT_1S)
#define ERROR_FLASH_TIMEOUT				(8.0)

//#define MINIMUM_VALVE_CYCLE_TIME		(4)		//minimum valve cycling time to allow capacitors to charge

#define ADC_REF_VOLTS	((float)3.6)
#define ADC_BATT_VOLTAGE_CHAN	7
#define ADC_WEIGHT_CHAN			6

//-----------------------------------------22aug19---------------------------------------------------------------------

#define EEPROM_ADDR_OFFSET		((uint16_t *)80) // 19aug19  changed to 100  before 000
#define EEPROM_ADDR_GAIN		((uint16_t *)82) // 19aug19  changed to 102  before 002


#define EEPROM_ADDR_MEM_INC		((uint8_t *)128) // 22aug19 
#define EEPROM_ADDR_WEAROUT		((uint16_t *)130) // 22aug19  // 50 bytes 130 to 179 address reserved for wearout location
#define EEPROM_ADDR_DISP_CNT		((uint16_t *)180) // 22aug19 // 50 bytes 180 to 229 address reserved for dispense counter 
//----------------------------------------22aug19----------------------------------------------------------------------
//----------------------------------------20jan 2020-------------------------------------------------------------------
#define EEPROM_ADDR_UNITS_RECEIVED_CTR		((uint8_t *)230) // unitsReceivedCtr   is save here
//----------------------------------------20 jan 2020-----------------------------------------------------------------

#define ADC_FULL_SCALE_COUNTS	((float)1024.0)

//voltage threshold settings
#define ADC_FULL_SCALE_VOLTAGE	((float)3.6)

#if DEBUG_SKIP_LV_CHECKS
#define BATT_LOW_VOLTAGE			((float)1.7)
#define BATT_DEAD_VOLTAGE			((float)1.3)	//soap valve will be shut unconditionally
#else
#define BATT_LOW_VOLTAGE			((float)4.7)
#define BATT_DEAD_VOLTAGE			((float)4.3)	//soap valve will be shut unconditionally
#endif

#define BATT_RESET_STATS_VOLTAGE	((float)6.8)

#define BATT_VOLTAGE_RATIO		((float)(1.2/(1.2+3.3)))
#define SOAP_LOW_VOLTAGE		((float)1.0)	//you get soap low warning at this point

#define SOAP_FULL_VOLTAGE		((float)3.3)
#define SOAP_EMPTY_VOLTAGE		((float)0.8)

//convert to adc counts to prevent use of floating point ops
#define BATT_LOW_COUNTS			(uint16_t)((BATT_LOW_VOLTAGE * BATT_VOLTAGE_RATIO)/ADC_FULL_SCALE_VOLTAGE*ADC_FULL_SCALE_COUNTS)
#define BATT_DEAD_COUNTS		(uint16_t)((BATT_DEAD_VOLTAGE * BATT_VOLTAGE_RATIO)/ADC_FULL_SCALE_VOLTAGE*ADC_FULL_SCALE_COUNTS)
#define BATT_RESET_STATS_COUNT	(uint16_t)((BATT_RESET_STATS_VOLTAGE * BATT_VOLTAGE_RATIO)/ADC_FULL_SCALE_VOLTAGE*ADC_FULL_SCALE_COUNTS)
	
#define BATT_DEAD_COUNT_SET_LIMIT	5	//number of battery dead measurements before setting low battery lockout
#define BATT_LOW_COUNT_SET_LIMIT	5	//number consecutive battery low measurements before setting low battery condition

#define BATT_LOW_DISPENSE_LIMIT	2000	//number of dispenses allowed under low battery set conditions

#define BAG_ID_RESET	3

#define NUMBER_READS_TO_IMPRINT 0x03	//must be greater than 1

//#define MAX_SOAP_UNITS_DECREMENT	4	//maximum number of soap units to decrement during one sample period.
#define MAX_SOAP_UNITS_DECREMENT	1	//maximum number of soap units to decrement during one sample period.

#define DISPENSER_FULL_SCALE_SOAP_UNITS	100.0

#define WEIGHT_GAIN_CHECK_MAX	0x3ff
#define WEIGHT_GAIN_CHECK_MIN	0x0f
#define DEFAULT_WEIGHT_GAIN		0x180

#define WEIGHT_OFFSET_CHECK_MAX	0x380
#define WEIGHT_OFFSET_CHECK_MIN	0x001
#define DEFAULT_WEIGHT_OFFSET	0x000

#define SOAP_LOW_UNITS			(20.0 / 100.0 * DISPENSER_FULL_SCALE_SOAP_UNITS)	//soap low is 20% of full

//Timer0 clock used for software timers
#define T0_CLOCK				(F_CPU)		//T0 internal clock source

#ifdef F_CPU_1MHZ
#define T0_DIVIDER              (64.0)		//prescaler
#endif

#ifdef F_CPU_8MHZ
#define T0_DIVIDER				(256.0)
#endif

#define T0_FREQ                 (T0_CLOCK/T0_DIVIDER)       //T0 counting frequency
#define TICKS_PER_SECOND		(T0_FREQ/256.0)             //T0 overflow interrupt frequency
#define TICKS_PER_MILLISECOND   (TICKS_PER_SECOND/1000.0)

//option to include software timers 3 and 4
#define SW_TIMER_3_4_ENABLED	0


//Timer1 used for ekey transceiver
#define TIMER1_PRESCALER	1

#if TIMER1_PRESCALER == 1		//TODO - expand for all other pre-scaler values
#define TIMER1_DIV_CONST	1	//value that gets loaded to TCCR1B to pre-scale timer1 clock
#elif TIMER1_PRESCALER == 8
#define TIMER1_DIV_CONST	2	//value that gets loaded to TCCR1B to pre-scale timer1 clock
#endif

#define TIMER1_FREQ F_CPU/TIMER1_PRESCALER

#define USE_MANCHESTER_TTF_ENCODING	0 //note that Manchester encoding results in an asymmetrical clock signal so tag frequency determination is adversely effected
//#define RF_FREQ	118000.0		//nominal RFID tag frequency
#define RF_FREQ	125000.0		//nominal RFID tag frequency
#define TO ((uint16_t)((float)TIMER1_FREQ/RF_FREQ+0.5))	//basic timing unit for RFID tag

#define EKEY_READALL_TRIES	3	//number of tries to read soap RFID tag
//////

#define WDT_PER_SECOND		(uint8_t)(1.0/.016)

#define WDT_16MS	(0)
#define WDT_32MS	(1<<WDP0)
#define WDT_64MS	(1<<WDP1)
#define WDT_128MS	(1<<WDP1 | 1<<WDP0)
#define WDT_250MS	(1<<WDP2)
#define WDT_500MS	(1<<WDP2 | 1<<WDP0)
#define WDT_1S		(1<<WDP2 | 1<<WDP1)
#define WDT_2S		(WDT_1S | 1<<WDP0)
#define WDT_4S		(1<<WDP3)
#define WDT_8S		(1<<WDP3 | 1<<WDP0)

#define UART_BAUD_RATE		9600.0
#define UART_RX_BUF_SIZE	20
#define UART_TX_BUF_SIZE	200  // 250 previous C100

typedef struct {
	int8_t initialWeight;
	int8_t ekeyRemainingWeightUnits;
	int8_t ekeyRemainingFills;
	int8_t weightUnitsReceived;
	int8_t noSoapAddedCtr;
	int8_t ekeyDockageErrCtr;
	uint8_t refillBagEmptied;	
} FILL_START_STATS;

typedef union {
	struct {
		int8_t b0;
		int8_t b1;
		int8_t b2;
		int8_t b3;	
	};
	uint32_t uint32;
} QUAD_INT8;	

typedef struct {
	uint32_t	refillAmt;
	uint16_t	numKdispenses;		//counts thousands of dispenses
	uint8_t		distributorCode;	
} EEPROM_DATA;

//EEPROM map
#define EEPROM __attribute__((section(".eeprom")))
typedef struct {
	EEPROM_DATA data;
} struct_EEPROM_MAP;

void System_clockInit();
void System_wdtOn(uint8_t period);
void System_wdtOff(void);
void System_refreshEepromData(EEPROM_DATA *data);

FILL_START_STATS System_fillStats;

typedef struct {
	uint8_t tickCounter;
	uint8_t imprintCounter;
	uint8_t currentDistCode;
	uint8_t currentSoapWeight;

	uint8_t battLowCounts;
	uint8_t battDeadCounts;
	
	uint16_t dispenseCounter;		//counts individual dispenses up to 1000 for RTLS message
	
	uint16_t lowBattDispenseCtr;	//counts number of dispenses under low battery condition
	
	uint16_t unitsReceivedCtr;		//counts number of soap units received for statistical purposes
	
	EEPROM_DATA eepromData;

} SYSTEM_DATA;

typedef struct {
	int8_t deltaWeight;
	int8_t newWeightUnitsDifference;


} DEBUG_DATA;

#endif /* SYSTEM_H_ */
