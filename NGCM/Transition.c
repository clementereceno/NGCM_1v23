#include "System.h"
#include "Transition.h"
#include "Io.h"
#include "EventStack.h"
#include "StateMachine.h"
#include "Timer.h"
#include "avr/interrupt.h"
#include "Ekey.h"
#include "Adc.h"
#include "Uart.h"
#include "SmartLink.h"
#include <avr/eeprom.h>  // 22aug2019

extern uint8_t l_lastInputs;
extern bit bag_empty; // gk100

extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;
//extern char uartdebug; //t200
extern char bagInsertedFlag1,bagInsertedErrorFlag; //t200
extern bit eventSwitchReleasedFlag;

uint16_t memWearout=0;  // 22aug19
uint8_t memInc=0;  // 22aug19
uint8_t eepromDataDirty2=0; //22aug19

static void l_transIdle(void);


void l_transIdle(void) {
	uint16_t aVal;
	
	//RED_LED_ON(); //t200
	//Uart_transmit(uartdebug);// t200
	
	if( Io_pinLevelHigh(PIN_BAG_SWITCH) ) {
		if( !BIT_IS_SET(l_lastInputs, PIND3) ) {
			EventStack_push(EVENT_BAG_REMOVED);
		}
		BIT_SET(l_lastInputs, PIND3);
	} else {
		if( BIT_IS_SET(l_lastInputs, PIND3) ) {
			EventStack_push(EVENT_BAG_INSERTED);
		}
		BIT_CLR(l_lastInputs, PIND3);
	}
	  eventSwitchReleasedFlag=0;  // gk100  clear flag
    Timer_init(&timer1,
    (uint16_t)1000);    //stop timer
	
	Timer_init(&timer2,
    (uint16_t)1000);    //stop timer
	
	Timer_enableTimerModule(0);	//stop timer module
	//make sure Melexis chip is disabled entirely
	MLX_MODU_OFF();
	FMOD_OFF();
	PCMSK0 &= ~PCINT6;	//mask interrupt from Melexis clock pin
	
	//turn all these off to save energy
	YELLOW_LED_OFF();
//	RED_LED_OFF();

	Io_setOvercurrentThreshold(0);	//set this to 0 to save energy. Threshold only needed when motor is running
	
//	Io_switchToADC();
	OPAMP_PWR_ON();			//on for soap measurement
    //---------------------------f100-----------------------------
	BIT_CLR(PCMSK0, PCINT5);	// disable ovld interrupt
	//BIT_CLR(PCICR, PCIE0);		//disable PCINT7..PCINT0
	//---------------------------f100-----------------------------
	PUMP_MOTOR_OFF();
	_delay_ms(1);
	//brake the motor
	PUMP_MOTOR_BRAKE_ON();
	_delay_ms(PUMP_MOTOR_BREAK_TIME_MS);
	PUMP_MOTOR_BRAKE_OFF();

	//signal the SW014. Mimics end-of-stroke switch
	PW18_DONE_ON();

	//Allow fixed number of dispenses after battery low condition
	if( sysFlags.battLow ) {
		//allow maximum BATT_LOW_DISPENSE_LIMIT number of dispenses during battery low conditions
		if( sysData.lowBattDispenseCtr >= BATT_LOW_DISPENSE_LIMIT ) {
			sysFlags.battDead = 1;
		} else {
			sysData.lowBattDispenseCtr++;	
		}
	}
	
	aVal = Adc_getChannel(ADC_BATT_VOLTAGE_CHAN);
	
	if( aVal <= BATT_DEAD_COUNTS ) {
		if( sysData.battDeadCounts >= BATT_DEAD_COUNT_SET_LIMIT ) {
			sysFlags.battDead = 1;
		} else {
			sysData.battDeadCounts++;
		}
	} else {
		sysData.battDeadCounts = 0;
		if( aVal <= BATT_LOW_COUNTS ) {
			if( sysData.battLowCounts >= BATT_LOW_COUNT_SET_LIMIT ) {
				if( !sysFlags.battLow ) {	//make sure following only happens ONCE
					sysFlags.battLow = 1;
					sysData.lowBattDispenseCtr = 0;
				}
			} else {
				sysData.battLowCounts++;
			}	
		} else {
			sysData.battLowCounts = 0;
		}
	}
	
	if( sysFlags.battLow ) {
		RED_LED_ON();
	} else {
		RED_LED_OFF();
	}
	
	sysData.currentSoapWeight = Adc_getWeightUnits();

#ifndef DEBUG_OP_AMP_ON	
	OPAMP_PWR_OFF();
#endif	

#if !DEBUG_RFID_TEST
	sysFlags.refillLow = (sysData.currentSoapWeight <= SOAP_LOW_UNITS);
	
	if(!sysFlags.battLow && sysFlags.refillLow) {
		System_wdtOn(SOAP_LOW_FLASH_PERIOD);	//for flashing soap low (yellow) LED
	} else {
		System_wdtOff();						//disable if not soap low
	}
#else
	System_wdtOn(SOAP_LOW_FLASH_PERIOD);						//1 second WDT timer interrupt
#endif
	
	//poll bag switch for proper state
	if( Io_pinLevelHigh(PIN_BAG_SWITCH) ) {
		BIT_SET(l_lastInputs, PIND3);
	} else {
		BIT_CLR(l_lastInputs, PIND3);
	}
	
	PRR &= ~(1<<PRUSART0);
	Uart_init();
	Uart_rxStart();							//mystery why this is needed. Required for the first RTLS dispense request.
	
	if( !sysFlags.restarted ) {    // t100 whenever there is dispense/ refill removal/refill timeout as valve status change/
		
		if(bagInsertedErrorFlag==0)  // do not transmit when ekey present/ refill error present as already one tx packet is sent in bag inserted function // t100
		{
			#ifndef DEBUG_SERIAL_SPIT
				SmartLink_serializeTxPacket(Uart_txBuf);
				Uart_transmit(Uart_txBuf);
			#endif
		}
	} else {
		sysFlags.restarted = 0;				//system restart is now over		
	}

	ENABLE_MOTOR_INT();							//stay in idle and look for MOTOR signal from above deck board

	ENABLE_BAG_INT();	
	
	
}

void Transition_idleMotorOvld(void) {
	sysFlags.noEOS = 1;
	sysFlags.highCurrent = 1;
	
	PUMP_MOTOR_OFF();
	_delay_ms(1);
	//brake the motor
	PUMP_MOTOR_BRAKE_ON();
	_delay_ms(PUMP_MOTOR_BREAK_TIME_MS);
	PUMP_MOTOR_BRAKE_OFF();
	
	Transition_errorFlash();
}

void Transition_idleMotorTimeout(void) {
	sysFlags.noEOS = 1;
	sysFlags.highCurrent = 0;
	
	if(sysFlags.battLow) {
		sysFlags.battDead = 1;		//motor stall on low battery. give up already!
	}
	
	PUMP_MOTOR_OFF();
	_delay_ms(1);
	//brake the motor
	PUMP_MOTOR_BRAKE_ON();
	_delay_ms(PUMP_MOTOR_BREAK_TIME_MS);
	PUMP_MOTOR_BRAKE_OFF();
	
	Transition_errorFlash();	
}

void Transition_idleNormal(void) {
	//sysFlags.noEOS = 0; // t100 was there in previous version 1.09
	//sysFlags.highCurrent = 0; // g100
	l_transIdle();	
}

void Transition_motorRun(void) {	
	
	bagInsertedErrorFlag = 0;// t100  clear the error flag 
	sysFlags.highCurrent = 0; // g100 clear the error flag
	
	if(!sysFlags.lockSerialMode && !sysFlags.battDead) {	//RS232 system usually used for weighing system
		Uart_stop();					//stop uart during motor run
		OPAMP_PWR_ON();					//power up the op amp for weight measurement after dispense.
		
	//stop looking for MOTOR_ON signal
		DISABLE_MOTOR_INT();
		ENABLE_EOS_INT();
		//--------------------------f100-------------------------------------
		BIT_SET(PCMSK0, PCINT5);	//enable OVLD interrupt (PB5)
		BIT_SET(PCIFR, PCIF0);		//clear interrupt flag for PCINT7..PCINT0
		BIT_SET(PCICR, PCIE0);		//enable PCINT7..PCINT0
		//--------------------------f100-------------------------------------
	
		YELLOW_LED_OFF();
		RED_LED_OFF();
	
		//increment number of dispenses		
		sysData.dispenseCounter++;  // 22aug19
		
		//------------------------------------// 22aug19--------------------------------------------------------------------
		if(sysData.dispenseCounter % 100 == 0)  /// every 100 dispenses save in memory( so on power loss only last 100 dispenses are lost) 
		{  
		     eepromDataDirty2=1;  // save in memory
		     memWearout++; 
		     if(memWearout>=9000)   // eeprome wearout write cycles 9000  so change the address to new location for saving the dispense count
		      {  memWearout = 0;  //reset
		         memInc=memInc+2;	// memwearout is 2 bytes size
				 if(memInc>=50) memInc=0; // reset to initial  // after 25 memory locations re circle back
				 eepromDataDirty2=2;  
		      }
	
		}
		//------------------------------------// 22aug19--------------------------------------------------------------------
		
		if( sysData.dispenseCounter >= 1000) {              // 1000
			sysData.dispenseCounter = 0;
			sysData.eepromData.numKdispenses++;
			sysFlags.eepromDataDirty = 1;
		}
		
		PUMP_MOTOR_ON();					//soap pump running
		//BIT_SET(l_lastInputs, PIND3); // j100
	
		Timer_enableTimerModule(1);
	
		// this is unconditional timeout of motor on
		Timer_init(&timer1,
			(uint16_t)(PUMP_MOTOR_TIMEOUT*TICKS_PER_SECOND));    //time-out for motor on
		Timer_start(&timer1);

	} else {
		StateMachine_currentState = ST_IDLE;	
	}
}

void Transition_bagInserted(void) {
	uint8_t i, distCode;
	bit goodToGo;
	
	sysFlags.coverOpen = 1;  // c100 now switch closed indication
    bagInsertedFlag1 = 1; // t200
	bagInsertedErrorFlag=0; // clear error
	bag_empty=0; // gk100
	Uart_rxStart();		//resets receive buffer t100
	
	//if(!sysFlags.lockSerialMode && !sysFlags.battDead) {                               t100  was present in earlier version 1.09
		//Uart_stop();						//stop serial service when bag is inserted   t100  was present in earlier version 1.09
		if(!sysFlags.battDead) {
		YELLOW_LED_OFF();
		RED_LED_OFF();
		DISABLE_EOS_INT();
		DISABLE_MOTOR_INT();
		
//		Timer_enableTimerModule(1);
		goodToGo = 1;						//default all is well with the ekey
		
#if !DEBUG_SKIP_EKEY_CHECK
		i=0;
		while( !Ekey_readAll() && (i<EKEY_READALL_TRIES) ) {	//if this is successful, all the key data will have been read successfully
			_delay_ms(5);
			i++;
		}
		//RED_LED_ON();
		//_delay_ms(40);
		//RED_LED_OFF();
		//_delay_ms(50);
		
		if( i<EKEY_READALL_TRIES ) {	//if ekey read was successful
		     sysFlags.ekeyPresent = 1; // t100
#if !DEBUG_SKIP_USAGE_DOCK		
			Ekey_dockUsage(0, 1);		//subtract 1 usage
#endif		
			MLX_MODU_OFF();
			goodToGo = 1;
			
			System_refreshEepromData(&sysData.eepromData);
			distCode = (Ekey_getTagData()->bagIdentifier & 0x3f);							//only 6 bits of it
			sysFlags.refillErr = 0;															//default to correct key detected
			
			if(distCode != BAG_ID_RESET) {
				if( sysData.eepromData.distributorCode == 0xff ) {							//distributor code has not been set
					if( sysData.currentDistCode == distCode ) {								//last read code is the same as this
						if( ++sysData.imprintCounter >= (NUMBER_READS_TO_IMPRINT-1) ) {		//coincidence? I think not. Time to imprint.
							sysData.eepromData.distributorCode = distCode;					//
							sysFlags.eepromDataDirty = 1;									//
						}
					} else {
						sysData.currentDistCode = distCode;
						sysData.imprintCounter = 0;	
					}
				} else {																	//has already been imprinted
					sysData.currentDistCode = sysData.eepromData.distributorCode;			//get code that has been imprinted
					goodToGo = (sysData.currentDistCode == distCode);						//compare to code in the ekey
					if(!goodToGo)	{
						sysFlags.refillErr = 1;												//wrong ekey code	//wrong ekey code
					}
				}
				
				if( goodToGo ) {															//all is well as far as the distributor code goes. Check for soap left.
					
#if DEBUG_SKIP_SOAP_LOCKOUT == 0															//soap bag empty will lock out the fill attempt					
					goodToGo = ( ((int8_t)Ekey_getTagData()->weightCounter > (int8_t)0)
					&& ((int8_t)Ekey_getTagData()->useCounter > (int8_t)0) );				//check the usage counters
#else
					goodToGo = 1;															//no check for zero unit counter
#endif
				}
				
				sysFlags.usingResetKey = 0;							//this is not the reset key
				//Clem
				//#ifdef DEBUG_SERIAL_SPIT
				//	Uart_transmit("This is NOT RESET KEY\r\n");
				//#endif				
				
			} else {	//this is reset key
				sysData.imprintCounter = 0;							//probably redundant
				sysData.currentDistCode = BAG_ID_RESET;				//for Smartlink to report reset key is installed
				
				if( sysData.eepromData.distributorCode != 0xff  ) {
					sysData.eepromData.distributorCode = 0xff;		//allow dispenser to imprint again.
					sysFlags.eepromDataDirty = 1;						
				}
				
				sysFlags.usingResetKey = 1;
				//Clem
				//#ifdef DEBUG_SERIAL_SPIT
				//	Uart_transmit("This is a RESET KEY\r\n");
				//#endif				
			}
						
#endif	//!DEBUG_SKIP_EKEY_CHECK
			if( goodToGo ) {	//correct key with some soap left!
				
				//read the current weight of the soap in the dispenser
		
				//get remaining weight/fills from the ekey validation. There will be some since the validation would have failed.
				System_fillStats.ekeyRemainingFills = Ekey_getTagData()->useCounter;			//number of fills left in bag
				System_fillStats.ekeyRemainingWeightUnits = Ekey_getTagData()->weightCounter;	//number of weight units in bag
				System_fillStats.weightUnitsReceived = 0;		//number of soap weight units delivered to the dispenser
				System_fillStats.ekeyDockageErrCtr = 0;			//counts unsuccessful ekey counter decrement attempts
				System_fillStats.noSoapAddedCtr = 0;			//counts number of weight polls with no weight increase
				System_fillStats.refillBagEmptied = 0;			//tracks whether refill bag was emptied on this transaction

#if !DEBUG_EKEY_TESTER_MODE
				OPAMP_PWR_ON();
				_delay_ms(500);				//delay to prevent immediate open while valve still closing
				sysData.currentSoapWeight = Adc_getWeightUnits();
				//Uart_transmit(uartdebug);// t200
				System_fillStats.initialWeight = sysData.currentSoapWeight;
				YELLOW_LED_ON();
				
				Io_valve(IO_VALVE_OPEN);
				_delay_ms(100);				//for power supply to settle before looking at power supply for power removal
				sysFlags.valveOpen = 1;
				
				Io_switchToComparator();	//gotta watch for power removal now that opening valve.

				sysData.tickCounter = BAG_INSERTED_INACTIVITY_TIMEOUT;
#else
				YELLOW_LED_ON();
#endif

				System_wdtOn(WDT_1S);			//1 second WDT timer interrupt while filling bag
				
			}	//else did not pass the distributor code check or soap bag is reported empty
#if !DEBUG_SKIP_EKEY_CHECK
		} else {	//failed read
			goodToGo = 0; 
			sysFlags.ekeyPresent = 0; // t100
			sysFlags.refillErr = 1; // t100
		}
#endif		
		if( !goodToGo ) {		//failed read or wrong code or no soap left or usage counter is zero
			//sysFlags.ekeyPresent = 0; // t100
			Transition_errorFlash(); 
			bagInsertedErrorFlag=1;
			
			//-----------------------debug-------a200--------------------------------------------
			//RED_LED_ON();_delay_ms(100); RED_LED_OFF();_delay_ms(100);RED_LED_ON();_delay_ms(100);RED_LED_OFF();
			
			//------------------------------a200--------------------------------------------
		}
	} else {	//serial debug mode is locked in. No ekey actions are executed
		StateMachine_currentState = ST_IDLE;	
	}
	#ifndef DEBUG_SERIAL_SPIT	
		//nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
		SmartLink_serializeTxPacket(Uart_txBuf);  // t100
		Uart_transmit(Uart_txBuf);// t100
		//nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn-
	#endif
	
	//PRR &= ~(1<<PRUSART0);
	//Uart_init();
	//Uart_rxStart();							//mystery why this is needed. Required for the first RTLS dispense request.

}

void Transition_biToIdle(void) {
	
	Io_switchToADC();				//switch to ADC mode from comparator mode to start using ADC for battery measurement
									//do not want valve closing to trigger the battery dead voltage compare interrupt	
#if !DEBUG_EKEY_TESTER_MODE
	Io_valve(IO_VALVE_CLOSE);
	
	
#endif

	sysFlags.valveOpen = 0;
	bagInsertedFlag1 = 0; // t100  clear flag when bag removed or due to timeout of 2 mins

	Transition_idleNormal();	
}

void Transition_errorFlash(void) {
	
	MLX_MODU_OFF();
	RED_LED_OFF();
	YELLOW_LED_OFF();
			
//	RED_LED_ON();
	
	System_wdtOn(ERROR_FLASH_PERIOD);
	
	Timer_enableTimerModule(1);
			
	// this is unconditional timeout of motor on
	Timer_init(&timer1,
	(uint16_t)(ERROR_FLASH_TIMEOUT*TICKS_PER_SECOND));    //time-out for motor on
	Timer_start(&timer1);

	StateMachine_currentState = ST_ERROR_FLASH;
	bagInsertedFlag1=0;
}
