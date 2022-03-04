/** \file timer.h 
*
* Serial Peripheral Interface (SPI)
*/
#ifndef __TIMER_H 
#define __TIMER_H

#include "System.h"

#define EASY_STATE_TIMEOUT	3*TICKS_PER_SECOND			//5 seconds for easy schedule select timeout
#define SPRAY_LOCKOUT_TIME	180*TICKS_PER_SECOND		//3 minutes seconds for spray lockout
#define DISCO_CHECK_WAIT	2*TICKS_PER_SECOND
#define CAN_REPLACED_TIME	10*TICKS_PER_SECOND
#define DISPENSE_DELAY		5*TICKS_PER_SECOND			//delay before dispense when blue LED id flashing
#define DELAY_HALF_SECOND	(TICKS_PER_SECOND >> 1)		//1/2 second for flashing LED
#define DELAY_QUARTER_SECOND	(TICKS_PER_SECOND >> 2)

/**
* structure of a software timer
*/
typedef struct {
    uint16_t  ui16_TriggerValue;  ///< millisecond value that triggers b_elapsed
    uint16_t  ui16_Counter;	///< timer counter
    uint8_t  b_Elapsed:1;	///< used to check if time has elapsed
    uint8_t  b_Running:1;	///< set/check timer is running
} TIMER_SW;

void Timer_service(TIMER_SW *timer);
void Timer_init(TIMER_SW *theTimer, uint16_t msec);
void Timer_initAll();
void Timer_serviceAll(void);
void Timer_rewind(TIMER_SW *ps_TimerSW);
void Timer_start(TIMER_SW *ps_TimerSW);
void Timer_stop(TIMER_SW *ps_TimerSW);
void Timer_reset(TIMER_SW *ps_TimerSW);
uint8_t Timer_elapsed(TIMER_SW *ps_TimerSW);
void Timer_clearElapsed(TIMER_SW *ps_TimerSW);
void Timer_check();
uint8_t Timer_anyRunning();
void Timer_enableTimerModule(bit enable);

TIMER_SW timer1, timer2;
#if SW_TIMER_3_4_ENABLED
TIMER_SW timer3, timer4;
#endif

#endif
