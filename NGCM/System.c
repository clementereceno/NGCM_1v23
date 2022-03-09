/*
 * System.c
 *
 * Created: 8/5/2015 12:22:44 PM
 *  Author: mikev
 */ 

#include "System.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
/*
// Fuse Information
#define FUSE_MEMORY_SIZE 3

// Low Fuse Byte
#define FUSE_CKSEL0 (unsigned char)~_BV(0) // Select Clock Source 
#define FUSE_CKSEL1 (unsigned char)~_BV(1) // Select Clock Source 
#define FUSE_CKSEL2 (unsigned char)~_BV(2) // Select Clock Source 
#define FUSE_CKSEL3 (unsigned char)~_BV(3) // Select Clock Source 
#define FUSE_SUT0   (unsigned char)~_BV(4) // Select start-up time 
#define FUSE_SUT1   (unsigned char)~_BV(5) // Select start-up time 
#define FUSE_CKOUT  (unsigned char)~_BV(6) // Clock output 
#define FUSE_CKDIV8 (unsigned char)~_BV(7) // Divide clock by 8 
#define LFUSE_DEFAULT (FUSE_CKSEL0 & FUSE_CKSEL2 & FUSE_CKSEL3 & FUSE_SUT0 & FUSE_CKDIV8)

// High Fuse Byte
#define FUSE_BODLEVEL0   (unsigned char)~_BV(0) // Brown-out Detector trigger level 
#define FUSE_BODLEVEL1   (unsigned char)~_BV(1) // Brown-out Detector trigger level 
#define FUSE_BODLEVEL2   (unsigned char)~_BV(2) // Brown-out Detector trigger level 
#define FUSE_EESAVE      (unsigned char)~_BV(3) // EEPROM memory is preserved through chip erase 
#define FUSE_WDTON       (unsigned char)~_BV(4) // Watchdog Timer Always On 
#define FUSE_SPIEN       (unsigned char)~_BV(5) // Enable Serial programming and Data Downloading 
#define FUSE_DWEN        (unsigned char)~_BV(6) // debugWIRE Enable 
#define FUSE_RSTDISBL    (unsigned char)~_BV(7) // External reset disable 
#define HFUSE_DEFAULT (FUSE_SPIEN)

// Extended Fuse Byte
#define FUSE_BOOTRST   (unsigned char)~_BV(0) // Select reset vector
#define FUSE_BOOTSZ0   (unsigned char)~_BV(1) // Brown-out Detector trigger level
#define FUSE_BOOTSZ1   (unsigned char)~_BV(2) // Brown-out Detector trigger level
#define EFUSE_DEFAULT (FUSE_BOOTSZ0 & FUSE_BOOTSZ1)
*/

__fuse_t __fuse __attribute__((section (".fuse"))) =
{
	.low = LFUSE_DEFAULT,
	.high = (HFUSE_DEFAULT & FUSE_DWEN),
	.extended = EFUSE_DEFAULT,
};

struct_EEPROM_MAP EEPROM l_eeprom;

SYSTEM_FLAGS sysFlags;
SYSTEM_DATA sysData;
DEBUG_DATA dbgData;
DEBUG_FLAGS dbgFlags;

extern uint8_t memInc; // 22aug19
extern uint16_t memWearout; // 22aug19
extern uint8_t eepromDataDirty2;//22aug19

void System_clockInit() {

	CLKPR = 0x80;			//enable change to cpu clock pre-scaler	
	CLKPR = CPU_PRESCALER;	//set pre-scaler value
}

void System_wdtOn(uint8_t period)
{
	cli();
	wdt_reset();
	MCUSR &= ~(1<<WDRF);
	WDTCSR = (1<<WDCE | 1<<WDE);
	/* Start timed sequence */
	// Set new prescaler(time-out) value = 128K cycles (~1.0 s)
	//interrupt enabled
	WDTCSR = (1<<WDIE | period);
	sei();
}

void System_wdtOff(void)
{
	cli();
	wdt_reset();
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Write logical one to WDCE and WDE */
	/* Keep old prescaler setting to prevent unintentional time-out */
	WDTCSR |= (1<<WDCE);
	/* Turn off WDT */
	WDTCSR = 0x00;
	sei();
}

void System_refreshEepromData(EEPROM_DATA *data) {
	
	
	if(sysFlags.eepromDataDirty) {
		eeprom_write_block( data, &l_eeprom, sizeof(EEPROM_DATA) );
		eeprom_write_word (EEPROM_ADDR_DISP_CNT+memInc, sysData.dispenseCounter); //22aug19
		sysFlags.eepromDataDirty = 0;	
		
	}
	eeprom_read_block( data, &l_eeprom, sizeof(EEPROM_DATA) );
	
	//-----------------------22aug19--------------------------------
	if((eepromDataDirty2==1) || (eepromDataDirty2==2)) 
	{
		
        eeprom_write_word (EEPROM_ADDR_WEAROUT+memInc, memWearout); // 2.2l bag //22aug19
        eeprom_write_word (EEPROM_ADDR_DISP_CNT+memInc, sysData.dispenseCounter); // 2.2l bag //22aug19  3 million dispenses
	    if(eepromDataDirty2==2) { eeprom_write_byte (EEPROM_ADDR_MEM_INC, memInc); //22aug19	  
	    }
		eepromDataDirty2=0;
		
	}
	     //
	//-----------------------22aug19--------------------------------
	
}

ISR (WDT_vect) {
	BIT_SET(System_interruptFlags, SYSTEM_INTFLG_WDT);
	wdt_reset();	
}
