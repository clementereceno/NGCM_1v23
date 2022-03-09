/*
 * main.c
 *
 *  Created on: Jul 22, 2015
 *      Author: mikev
        Updated: adityad - rev 1.10 and above, refer system.h file for updates in code revisions
		                   hex file in release folder to be used for programming 
*/

#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "System.h"
#include "Io.h"
#include "EventStack.h"
#include "Statemachine.h"
#include "Timer.h"
#include "avr/sfr_defs.h"
#include "Uart.h"
#include "SmartLink.h"


static uint8_t l_flags;
uint8_t l_lastInputs;
static void stackEvents(void);
extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;

char treceive=0; // t100
char bagInsertedFlag1 =0, bagIdealTimeoutFlag=0, bagInsertedErrorFlag=0;//t100
bit pending_bag_inserted=0, eventSwitchReleasedFlag=0;   // j100


int main(void) {
	int8_t eventsDone, newEvent;
	unsigned char j=0; // t100
	const char *msg;
	
	System_clockInit();
	
	Io_init();						//initialize I/O pins

    Timer_initAll();				//set up software timers

    StateMachine_init();
	Uart_init();					//set up baud rate, parity, stop bits
		
	sei();							//enable interrupts
	
	l_lastInputs = PIND & 0b00011101;	//switch input bits and Rxd
	

	while(1) {						//main program loop
        eventsDone = 0;

        while(!eventsDone) {
            
            newEvent = EventStack_pop();

            if(newEvent != -1) {
                StateMachine_stateEval(newEvent); // process new event based on state machine
            } else {
                eventsDone = 1;
            }
			
	              if(bagInsertedFlag1==1) // t100
	                {
	                 if(treceive==1)	
	                  {	
			           		
		               if((Uart_rxBuf[0] == SMARTLINK_RX_PACKET_ID) && (Uart_rxBuf[1] =='0') && (Uart_rxBuf[2] == '0')) 
		                     {	
								//#ifndef DEBUG_SERIAL_SPIT
									SmartLink_serializeTxPacket(Uart_txBuf);  // t100
									Uart_transmit(Uart_txBuf);// t100
								//#endif
								                                           
						      }
								Uart_rxBufIdx = 0;
								for(j=0;j<10;j++) Uart_rxBuf[j]=0;
								treceive=0;
							}
	                            //lockSerialMode = 0;	
	                 }
         }

        if( !Timer_anyRunning() ) {     //software timers cannot wake up processor
			
			if( (l_flags == 0) && !sysFlags.lockSerialMode ) {
				
				if( (UCSR0B & (1<<TXEN0)) != 0 ) {				
					while( (UCSR0B & (1<<TXEN0)) != 0) {};		//wait for any data to be transmitted
					_delay_ms(2);								//required delay for last character
				}
				
				//stop clocks as much as possible.
				PRR = (1<<PRTWI) | (1<<PRTIM2) | (1<<PRTIM0) | (1<<PRTIM1)| /* (1<<PRUSART0) |*/ (1<<PRADC) | (1<<PRSPI);
			
					if(StateMachine_currentState == ST_IDLE){
						if(pending_bag_inserted==1) // // gk100
						{
							pending_bag_inserted=0; eventSwitchReleasedFlag=0; // clear flags
							//_delay_ms(100);
							EventStack_push(EVENT_BAG_INSERTED);
							BIT_CLR(l_lastInputs, PIND3);
							//a100------------------------------------------------
							//RED_LED_ON();   // debug  remove later
							//_delay_ms(100);  // debug remove later
							//RED_LED_OFF();  // debug remove later
							//a100------------------------------------------------
							goto out1;
							
						}
//					Uart_rxStart();
					ENABLE_RX_PIN_INT();
//					set_sleep_mode(SLEEP_MODE_IDLE);
					set_sleep_mode(SLEEP_MODE_PWR_DOWN);
				} else {
					set_sleep_mode(SLEEP_MODE_IDLE);
				}
				
		     	sleep_enable();  // j100
				sei();   // j100
				sleep_cpu(); //j100
				sleep_disable();
				cli();
		out1:		         //gk100
             	DISABLE_RX_PIN_INT();
				
            }
			
        } else {
            Timer_check();              //one of the software timers is running: check for expired
        }
		
        sei();							//enable all interrupt: interrupts will be serviced here
		asm ("nop");
		
		if( StateMachine_currentState == ST_IDLE ) {
			_delay_ms(20);
		}
		
        cli();								//disable all interrupts		
        l_flags = System_interruptFlags;	//snapshot of flags set in interrupt
        System_interruptFlags = 0;			//
        sei();								//enable all interrupts

		if(l_flags != 0) {
			stackEvents();
		} 
		
	}//end main while loop
}

static void stackEvents(void) {
	bool serviced = 0;
	
	if( BIT_IS_SET(l_flags, SYSTEM_INTFLG_PCINT2) ) {
//MOTOR_ON signal			
		if( Io_pinLevelHigh(PIN_SW14_MOTOR) ) {
			if(StateMachine_currentState == ST_IDLE) {
#if !DEBUG_EKEY_TESTER_MODE
				EventStack_push(EVENT_MOTOR_ON);
				sysFlags.noEOS = 0; // t100
#endif				
				serviced = 1;
			}
		}
		
		if( Io_pinLevelHigh(PIN_EOS_SWITCH) ) {
			if( !BIT_IS_SET(l_lastInputs, PIND2) ) {
				EventStack_push(EVENT_EOS_OPEN);
				serviced = 1;
			}
			BIT_SET(l_lastInputs, PIND2);
		} else {
			if( BIT_IS_SET(l_lastInputs, PIND2) ) {
				EventStack_push(EVENT_EOS_CLOSED);
				serviced = 1;
			}
			BIT_CLR(l_lastInputs, PIND2);
		}
		
		if( Io_pinLevelHigh(PIN_BAG_SWITCH) ) {
			
			if( !BIT_IS_SET(l_lastInputs, PIND3) ) {
				EventStack_push(EVENT_BAG_REMOVED);
				serviced = 1;
				
				if(StateMachine_currentState == ST_BAG_INSERTED) eventSwitchReleasedFlag=1; // gk100  // inserted again
				sysFlags.coverOpen = 0;  // t100
				sysFlags.ekeyPresent = 0; // t100
				
				#ifndef DEBUG_SERIAL_SPIT
				//nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn 
					if((bagInsertedErrorFlag==1)||(bagIdealTimeoutFlag==1)) //t100
					{SmartLink_serializeTxPacket(Uart_txBuf);  // t100
					Uart_transmit(Uart_txBuf);}// t100
				//nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn-
				#else
					if((bagInsertedErrorFlag==1)) //t100
					{SmartLink_serializeTxPacket(Uart_txBuf);  // t100
					Uart_transmit(Uart_txBuf);}// t100
				#endif
				
			}
			BIT_SET(l_lastInputs, PIND3);
		} else {
			if((StateMachine_currentState == ST_MOTOR_RUN)||(eventSwitchReleasedFlag == 1)) // j100  // gk100
			{ pending_bag_inserted=1;}
				
			else if(StateMachine_currentState == ST_IDLE)
			{ 
				if( BIT_IS_SET(l_lastInputs, PIND3) ) {
				EventStack_push(EVENT_BAG_INSERTED);
				serviced = 1;
			      }
		    }
			BIT_CLR(l_lastInputs, PIND3);
		}

		BIT_CLR(l_flags, SYSTEM_INTFLG_PCINT2);
	}  
	
	//--------------------------f100-----------------------------
	if( BIT_IS_SET(l_flags, SYSTEM_INTFLG_MOTOR_OVERLOAD) )
	{
		//_delay_ms(50); // wait time inrush current duration or confirm its not noise  v1.17 and below
		_delay_ms(75); // j100
		//RED_LED_OFF();
		if(bit_is_clear(PINB, PINB5))  // if still LOW ( i.e indicates over current)  // 28 nov 2017  2.27pm
		{
			EventStack_push(EVENT_MOTOR_OVERLOAD);   // push event OVLD
	
		}
		//}
		BIT_CLR(l_flags, SYSTEM_INTFLG_MOTOR_OVERLOAD);
	}
	//-------------------------f100------------------------------
	
	if( BIT_IS_SET(l_flags, SYSTEM_INTFLG_WDT) ) {
		EventStack_push(EVENT_WDT);
		BIT_CLR(l_flags, SYSTEM_INTFLG_WDT);
	}
	
	if( BIT_IS_SET(l_flags,SYSTEM_INTFLG_UARTRX)) {
		
		EventStack_push(EVENT_UARTRX);
		BIT_CLR(l_flags, SYSTEM_INTFLG_UARTRX);
	}
	
	if( BIT_IS_SET(l_flags,SYSTEM_INTFLG_UARTTX) ) {
		BIT_CLR(l_flags, SYSTEM_INTFLG_UARTTX);
	}
	
	
}
