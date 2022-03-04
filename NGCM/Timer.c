/** \file timer.c 
*/
#include "Timer.h"
#include "EventStack.h"
#include <avr/interrupt.h>

void Timer_initAll() {
//
//must be initialized before enabling timer0 interrupt
//
    Timer_init(&timer1, 255);
    Timer_init(&timer2, 255);
	
#if SW_TIMER_3_4_ENABLED
    Timer_init(&timer3, 255);
    Timer_init(&timer4, 255);
#endif
}

void Timer_serviceAll()
{
    Timer_service(&timer1);
    Timer_service(&timer2);
#if SW_TIMER_3_4_ENABLED
    Timer_service(&timer3);
    Timer_service(&timer4);
#endif
}

/** 
* create a new software timer
*
* @param ps_TimerData - pointer to timer to be used for new sw timer
* @param ui32_Timeout - number of milliseconds after which the timer will expire 
* @return pointer to new TIMER_SW structure
*/
void Timer_init(TIMER_SW *theTimer, uint16_t msec) {

#ifdef __XC32
//    IEC0bits.T2IE = 0;
#else
//    INTCONbits.TMR0IE = 0;
#endif
	
    TIMSK0 &= ~(1<<TOIE0);			//disable timer0 interrupts
    theTimer->b_Running = 0;
    theTimer->b_Elapsed = 0;
    theTimer->ui16_TriggerValue = msec;			//counter will count up to trigger level
    theTimer->ui16_Counter = 0;
	TCNT0 = 0;						//timer reset to zero
	TIFR0 = (1<<TOV0);				//clear timer overflow interrupt flag
    TIMSK0 |= (1<<TOIE0);			//enable timer0 interrupts

#ifdef __XC32
 //   IEC0bits.T2IE = 1;
#else
//    INTCONbits.TMR0IE = 1;
#endif
}

/**
* rewind a sw timer. timer is reset to 0.
* @param ps_TimerSW - pointer to sw timer to rewind
*/
void Timer_rewind(TIMER_SW *ps_TimerSW) {
    ps_TimerSW->ui16_Counter = 0;
}

void Timer_reset(TIMER_SW *ps_TimerSW) {
    ps_TimerSW->b_Running = 0;
    ps_TimerSW->ui16_Counter = 0;
    ps_TimerSW->b_Elapsed = 0;
}
 
/**
* start sw timer
*
*	@param ps_TimerSW - pointer to sw timer to start
*/
void Timer_start(TIMER_SW *ps_TimerSW) {
    TIMSK0 &= ~(1<<TOIE0);			//disable timer0 interrupts
    ps_TimerSW->b_Running = 1;
    TIMSK0 |= (1<<TOIE0);			//enable timer0 interrupts   
}

/**
* stop sw timer
*
*	@param ps_TimerSW - pointer to sw timer to stop
*/
void Timer_stop(TIMER_SW *ps_TimerSW) {
    ps_TimerSW->b_Running = 0;
}

uint8_t Timer_elapsed(TIMER_SW *ps_TimerSW) {
    return(ps_TimerSW->b_Elapsed);
}

void Timer_clearElapsed(TIMER_SW *ps_TimerSW) {
    ps_TimerSW->b_Elapsed = 0;
}


void Timer_service(TIMER_SW *timer) {

    if( (timer != '\0') && (timer->b_Running) ) {
//        timer->ui16_Counter++;
        if(++timer->ui16_Counter >=
            timer->ui16_TriggerValue) {
//            timer->ui16_TriggerValue-1) {
            timer->ui16_Counter = 0;
            timer->b_Elapsed = -1;
        }
    }
}

void Timer_check() {
	
    if( Timer_elapsed(&timer1) ) {		//poll for timeout of S/W timer based on timer0
            Timer_clearElapsed(&timer1);
            EventStack_push(EVENT_TIMER1);
//		FLAG_SET(app_flags,
//			APP_TIMER1);		//flag timeout for application
    }

    if( Timer_elapsed(&timer2) ) {		//poll for timeout of S/W timer based on timer0
            Timer_clearElapsed(&timer2);
            EventStack_push(EVENT_TIMER2);
//		FLAG_SET(app_flags,
//			APP_TIMER2);		//flag timeout for application
    }

#if SW_TIMER_3_4_ENABLED
    if( Timer_elapsed(&timer3) ) {		//poll for timeout of S/W timer based on timer0
            Timer_clearElapsed(&timer3);
            EventStack_push(EVENT_TIMER3);
//		FLAG_SET(app_flags,
//			APP_TIMER3);		//flag timeout for application
    }

    if( Timer_elapsed(&timer4) ) {		//poll for timeout of S/W timer based on timer0
            Timer_clearElapsed(&timer4);
            EventStack_push(EVENT_TIMER4);
//		FLAG_SET(app_flags,
//			APP_TIMER4);		//flag timeout for application
    }
#endif
}

uint8_t Timer_anyRunning() {

/* this version takes 115 bytes of code
//    static bit retVal;

//    retVal = false;
    retVal = timer1.b_Running?TRUE:retVal;
    retVal = timer2.b_Running?TRUE:retVal;
    retVal = timer3.b_Running?TRUE:retVal;
    retVal = timer4.b_Running?TRUE:retVal;
*/
    
    //this version takes 32 bytes of code
    if(timer1.b_Running) {
        return(-1);
    }
    if(timer2.b_Running) {
        return(-1);
    }
	
#if SW_TIMER_3_4_ENABLED
    if(timer3.b_Running) {
        return(-1);
    }
    if(timer4.b_Running) {
        return(-1);
    }
#endif     

    return(0);
}

void Timer_enableTimerModule(bit enable) {
	
	if(enable)	 {
		PRR &= ~(1<<PRTIM0);			//start timer0 clocks

		#ifdef F_CPU_1MHZ
		TCCR0B = (1<<CS01) | (1<<CS00);	//clock divider is 64
		#endif

		#ifdef F_CPU_8MHZ
		TCCR0B = (1<<CS02);				//clock divider is 256
		#endif
		TCNT0 = 0;						//timer reset to zero
		TIFR0 = (1<<TOV0);				//clear timer overflow interrupt flag
		TIMSK0 = (1<<TOIE0);			//enable overflow interrupt bit
	} else {
		TIMSK0 &= ~(1<<TOIE0);			//disable overflow interrupt bit
		PRR |= (1<<PRTIM0);			//stops timer0 clocks
	}
}

ISR (TIMER0_OVF_vect) {
	Timer_serviceAll();
}


