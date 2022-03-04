#include <math.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>  // 19aug19
#include "System.h"
#include "StateMachine.h"
#include "EventStack.h"
#include "Io.h"
#include "Transition.h"
#include "Adc.h"
#include "Ekey.h"
#include "Timer.h"
#include "UartParse.h"
#include "SmartLink.h"

extern SYSTEM_FLAGS sysFlags;
extern SYSTEM_DATA sysData;

extern struct_EEPROM_MAP EEPROM l_eeprom; // 19aug19
extern char bagIdealTimeoutFlag,bagInsertedErrorFlag;

extern uint8_t memInc; // 22 aug 2019
extern uint16_t memWearout; // 22 aug 2019

static void l_lightShow(void);
//do nothing function
static void noAction(void);

static void l_checkIdReset(void);
char uartdebug[]={'T','E','S','T','\0'};//t200 
bit bag_empty = 0;// gk100

//int ( *state[ ] ) (void) = { ( void* ) &entryState, ( void* ) &fooState,
//                            ( void* ) &barState, ( void* ) &exitState };
//  Look-up table describing state transitions
// first array element is the current state
// second array element is the current event
// using these indices, pull the stateElement containing action to perform and the next state.
//
const stateElement stateMatrix[4][10] = {  //first index is state, second is event

//ST_IDLE
 {
#if !DEBUG_RFID_TEST	 
	{ST_MOTOR_RUN, Transition_motorRun},//EVENT_MOTOR_ON
	{ST_IDLE, noAction},     			//EVENT_MOTOR_OVERLOAD
	{ST_BAG_INSERTED, Transition_bagInserted},	//EVENT_BAG_INSERTED
	{ST_IDLE, noAction},				//EVENT_BAG_REMOVED
	{ST_IDLE, noAction},				//EVENT_EOS_OPEN
	{ST_IDLE, noAction},				//EVENT_EOS_CLOSED
	{ST_IDLE, StateMachine_idle},		//EVENT_WDT
	{ST_IDLE, noAction},     			//EVENT_TIMER1
	{ST_IDLE, noAction},              	//EVENT_TIMER2
	{ST_IDLE, StateMachine_rxMsg},      //EVENT_UARTRX
#else
	{ST_IDLE, noAction},				//EVENT_MOTOR_ON
    {ST_IDLE, noAction},     			//EVENT_MOTOR_OVERLOAD
	{ST_IDLE, noAction},				//EVENT_BAG_INSERTED
	{ST_IDLE, noAction},				//EVENT_BAG_REMOVED
	{ST_IDLE, noAction},				//EVENT_EOS_OPEN
	{ST_IDLE, noAction},				//EVENT_EOS_CLOSED
	{ST_IDLE, StateMachine_rfidTest},	//EVENT_WDT
    {ST_IDLE, noAction},     			//EVENT_TIMER1
    {ST_IDLE, noAction},              	//EVENT_TIMER2
	{ST_IDLE, noAction},              	//EVENT_UARTRX
#endif		
    },

//ST_MOTOR_RUN
 {  {ST_MOTOR_RUN, noAction}, 					//EVENT_MOTOR_ON
    {ST_MOTOR_RUN, Transition_idleMotorOvld},   //EVENT_MOTOR_OVERLOAD
	{ST_MOTOR_RUN, noAction},					//EVENT_BAG_INSERTED - motor must be left to run first before handling this
	{ST_MOTOR_RUN, noAction},					//EVENT_BAG_REMOVED
	{ST_IDLE, Transition_idleNormal},			//EVENT_EOS_OPEN - signal that normally ends the motor run
	{ST_MOTOR_RUN, noAction},					//EVENT_EOS_CLOSED
	{ST_MOTOR_RUN, noAction},					//EVENT_WDT
    {ST_IDLE, Transition_idleMotorTimeout},		//EVENT_TIMER1 -> timeout for motor run
    {ST_MOTOR_RUN, noAction},					//EVENT_TIMER2
	{ST_MOTOR_RUN, noAction},					//EVENT_UARTRX
    },
	
 //ST_BAG_INSERTED
 {  {ST_BAG_INSERTED, noAction}, 					//EVENT_MOTOR_ON
	{ST_BAG_INSERTED, noAction},        			//EVENT_MOTOR_OVERLOAD
	{ST_BAG_INSERTED, StateMachine_insertedAgain},	//EVENT_BAG_INSERTED
	{ST_BAG_INSERTED, StateMachine_bagRemoved},		//EVENT_BAG_REMOVED
	{ST_BAG_INSERTED, noAction},					//EVENT_EOS_OPEN
	{ST_BAG_INSERTED, noAction},					//EVENT_EOS_CLOSED
	{ST_BAG_INSERTED, StateMachine_bagInsertedTick},//EVENT_WDT
	{ST_IDLE, Transition_biToIdle},					//EVENT_TIMER1 -> timeout for bag inserted (valve open)
	{ST_BAG_INSERTED, StateMachine_valveCycleDone},	//EVENT_TIMER2
	{ST_BAG_INSERTED, noAction},					//EVENT_UARTRX
	},
	
//ST_ERROR_FLASH
{   {ST_IDLE, Transition_idleNormal}, 				//EVENT_MOTOR_ON
	{ST_ERROR_FLASH, noAction},        				//EVENT_MOTOR_OVERLOAD
	{ST_IDLE, Transition_idleNormal},				//EVENT_BAG_INSERTED
	{ST_IDLE, Transition_idleNormal},				//EVENT_BAG_REMOVED
	{ST_ERROR_FLASH, noAction},						//EVENT_EOS_OPEN
	{ST_ERROR_FLASH, noAction},						//EVENT_EOS_CLOSED
	{ST_ERROR_FLASH, StateMachine_errorFlashTick},	//EVENT_WDT	-> ekey error flash
	{ST_IDLE, Transition_idleNormal},				//EVENT_TIMER1 -> timeout for bag inserted with error
	{ST_ERROR_FLASH, noAction},						//EVENT_TIMER2
	{ST_ERROR_FLASH, noAction},						//EVENT_UARTRX
	},
};

void l_checkIdReset(void) {
	uint16_t aVal;
	uint16_t offset,gain;// 19aug19
	//uint8_t cnt=0;// 22aug19
	TAG_DATA *tData;
	
	sei();
	
	System_refreshEepromData(&sysData.eepromData);					//read the EEPROM data
	
	aVal = Adc_getChannel(ADC_BATT_VOLTAGE_CHAN);
	_delay_ms(100);
	aVal = Adc_getChannel(ADC_BATT_VOLTAGE_CHAN);			//not sure why have to do this twice after reboot
	
	if( aVal >= BATT_RESET_STATS_COUNT ) {					//signals that should check ekey for reset code to reset stats
		if( Ekey_readAll()	) {								//read the ekey that is supposed to be used to reset the stats
			tData = Ekey_getTagData();
			if( tData->bagIdentifier == BAG_ID_RESET ) {	//make sure it is the reset key
				sysData.eepromData.distributorCode = 0xff;				//set the distributor key as 'unprogrammed'
				sysData.eepromData.numKdispenses = 0;
				sysData.eepromData.refillAmt = 0;
				sysFlags.eepromDataDirty = 1;
				System_refreshEepromData(&sysData.eepromData);				//write the data back
				
				YELLOW_LED_ON();
				RED_LED_ON();
				_delay_ms(1000);
				RED_LED_OFF();
				YELLOW_LED_OFF();
			}
		}
	}
	
	
	
	offset = eeprom_read_word(EEPROM_ADDR_OFFSET);
	gain = eeprom_read_word(EEPROM_ADDR_GAIN);
	
	memInc = eeprom_read_byte(EEPROM_ADDR_MEM_INC);
	memWearout =  eeprom_read_word(EEPROM_ADDR_WEAROUT+memInc);
	sysData.dispenseCounter = eeprom_read_word(EEPROM_ADDR_DISP_CNT+memInc);
	
	sysData.unitsReceivedCtr = eeprom_read_byte(EEPROM_ADDR_UNITS_RECEIVED_CTR); // cxr20jan20
	if(sysData.unitsReceivedCtr>=100) // cxr20jan20
	{
		sysData.unitsReceivedCtr=0;
		eeprom_write_byte(EEPROM_ADDR_UNITS_RECEIVED_CTR,0); // clear to zero
	}
	
	if((offset==0xFFFF) && (gain==0xFFFF))   // will FF as this is new address location so programmed via bootloader will land here for first time but not next time
	{		
		//offset = eeprom_read_word(0);// offset earlier addres------------------------------------needed if offset needs to be read fro previous address and only programmed through bootloader as it does not erase the eeprome contents
		eeprom_read_block( &sysData.eepromData, &l_eeprom, sizeof(EEPROM_DATA) ); // ensure the old registered code is saved
		sysData.eepromData.numKdispenses = 0xFFFF;  // reset to initial
		sysData.eepromData.refillAmt = 0xFFFFFFFF; // reset to initial
		sysData.dispenseCounter=0;   // reset to initial
		memInc=0; // reset to initial
		memWearout =0; // reset to initial
		
		eeprom_write_block( &sysData.eepromData, &l_eeprom, sizeof(EEPROM_DATA) );  // write the initial values in eeprome
		eeprom_write_word (EEPROM_ADDR_OFFSET, 0xA0);  // write initial value when eeprome is newly programmed
		//eeprom_write_word (EEPROM_ADDR_OFFSET, offset);  // write initial value when eeprome is newly programmed
		//eeprom_write_word (EEPROM_ADDR_GAIN, 0x258); // 1.2l bag // write initial value when eeprome is newly programmed  // //19aug19_2 
		eeprom_write_word (EEPROM_ADDR_GAIN, 0x320); // 2.2l bag //19aug19_2
		
		
		eeprom_write_byte (EEPROM_ADDR_MEM_INC, memInc); // 2.2l bag //19aug19_2
		eeprom_write_word (EEPROM_ADDR_DISP_CNT, sysData.dispenseCounter); // 2.2l bag //19aug19_2
		eeprom_write_word (EEPROM_ADDR_WEAROUT, memWearout); // 2.2l bag //19aug19_2
		
		
	}
	
	
	cli();
}

/**
 *
 * */
void StateMachine_init() { 
	 
#ifndef DEBUG_CRYPTO_DISABLE	
	uint8_t i;
	uint64_t state;
	uint32_t result, sum;

	sum =0;
	
	state = Crypto_init (rev64 (0x524B494D4E4FUL), rev32 (0x69574349), rev32 (0x72456E65));
	
	for (i = 0; i < 16; i++) {
		result = Crypto_byte (&state);
		sum += result;
	}
#endif

	sysFlags.allFlags = 0;		//clear all system status flags etc.
	sysFlags.restarted = 1;		//flag to indicate that the system has rebooted
	
	sysData.dispenseCounter = 0;	//dispenses up to 1000
	sysData.imprintCounter = 0;		//for imprinting distributor code
	
	sysData.battLowCounts = 0;
	sysData.battDeadCounts = 0;		//for determination of low battery lockout

	sysData.lowBattDispenseCtr = 0;	//counts number of dispenses under low battery condition
	
	//sysData.unitsReceivedCtr = 0;	//for collecting statistics on amount of soap added.  //cxr20jan20

	l_checkIdReset();				//if battery voltage is high enough, then reset all the EEPROM parameters
	
	OPAMP_PWR_ON();				//for initial soap level measurement in Transition_idle()
#if !DEBUG_SKIP_OPTO_CAL_DELAY
	l_lightShow(); 
#else
	_delay_ms(200);				//time to allow op-amp to settle
#endif
	
    Transition_idleNormal();
	
    EventStack_init();			//initialize the event stack
	
	BIT_SET(PCIFR, PCIF2);		//PCIFR – Pin Change Interrupt Flag Register. This write clears the interrupt flag
	BIT_SET(PCICR, PCIE2);		//PCICR – MOTOR, BAG, EOS switch
	
	Io_valve(IO_VALVE_CLOSE);

    StateMachine_currentState = ST_IDLE;
}

/* function run if no action is to take place*/
void noAction()
{    
	asm("nop");
}

/********************************************************************************
 * stateEval (event)
 * in Dependency of an triggered event, the action which is required by this
 * transition will be returned. The proper action is determined by the current state the
 * automat holds. The current state will then be transitioned to the requested next
 * state
 ********************************************************************************/
state StateMachine_stateEval(uint8_t event) {
	
    //determine the State-Matrix-Element in dependency of current state and triggered event
    stateElement stateEvaluation = stateMatrix[StateMachine_currentState][event];

    //do the transition to the next state (set requested next state to current state)...
    StateMachine_currentState = stateEvaluation.nextState;
    //... and fire the proper action to get to next state
    (*stateEvaluation.actionToDo)();

    return(StateMachine_currentState);
}

void StateMachine_idle(void) {
		
#ifndef DEBUG_ENABLE_SILLY_LIGHT_SHOW	//do not do the normal reporting if silly light show is being done
//	RED_LED_OFF();
	YELLOW_LED_OFF();					//LED in known state now
	
#if !DEBUG_EKEY_TESTER_MODE
	
	YELLOW_LED_ON();
	_delay_ms(20);
	YELLOW_LED_OFF();	
	
#endif	//DEBUG_EKEY_TESTER_MODE

	
#endif

}


void StateMachine_motorRun(void) {  
}

void StateMachine_bagRemoved(void) {
	
	sysFlags.coverOpen = 0;  // t100
	//sysFlags.ekeyPresent = 0; // t100

#if !DEBUG_BYPASS_MINIMUM_VALVE_CYCLE_TIME
	sysData.tickCounter = BAG_OUT_VALVE_CLOSE_DELAY;			
#else
	StateMachine_currentState = ST_IDLE;
	Transition_biToIdle();
#endif
}

void StateMachine_valveCycleDone(void) {
	StateMachine_currentState = ST_IDLE;
	Transition_biToIdle();
}

void StateMachine_insertedAgain(void) {
	sysData.tickCounter = BAG_INSERTED_INACTIVITY_TIMEOUT;
}

void StateMachine_rxMsg(void) {
	UartParse_rxMsg();
	if( sysFlags.dispensePending )	{		//probably set by SmartLink
		sysFlags.dispensePending = 0;
		EventStack_push(EVENT_MOTOR_ON);		//force a dispense event
	}
}

/************************************************************************/
/* This is triggered by the watchdog timer when bag is inserted         */
/* The dispenser does not know when the bag will be removed so usage	*/
/* decrement must occur here.											*/
/************************************************************************/
void StateMachine_bagInsertedTick(void) {
//	uint16_t aVal;
	static bool done;
	int8_t newWeightUnitsDifference, deltaWeight;
	unsigned char tries=0; // gk100
	//RED_LED_ON(); // t200
	//_delay_ms(50);// t200
	//RED_LED_OFF();// t200
	
#if !DEBUG_SKIP_WEIGHT_DOCK
	uint32_t dummy;
	uint8_t page;
	
#endif
//UartParse_rxMsg(); 
 //Uart_transmit(uartdebug);// t200
//Io_switchToADC();																	//temporarily switch to ADC mode for measuring soap
//sysData.currentSoapWeight = Adc_getWeightUnits();											//what is the weight now?
 	MLX_MODU_OFF();
	//if( --sysData.tickCounter == 0  ){			//If tick counter == 0 then bag has been left sitting there to long. Time to shut the valve.  V1.18 and below  gk100  commented
	  if(( --sysData.tickCounter == 0  )||(bag_empty==1)){ // gk100  added
		StateMachine_currentState = ST_IDLE;
		Transition_biToIdle();					//game over: took too long
		bagIdealTimeoutFlag=1;
				
	} else {
		if( !sysFlags.usingResetKey ) {
			//does the bag contain any more soap? And the number of bag usages good?
			if( (System_fillStats.weightUnitsReceived < System_fillStats.ekeyRemainingWeightUnits)
				&& (System_fillStats.ekeyRemainingFills > 0) ) {

				//op amp is powered on in transition
				Io_switchToADC();																	//temporarily switch to ADC mode for measuring soap
				sysData.currentSoapWeight = Adc_getWeightUnits();											//what is the weight now?
				//RED_LED_ON(); // t200
				//_delay_ms(20);// t200
				//RED_LED_OFF();// t200
				//Uart_transmit(uartdebug);// t200
				Io_switchToComparator();															//look for power removal
	
				deltaWeight = sysData.currentSoapWeight - System_fillStats.initialWeight;					//compare to the initial bag weight
		
				if( deltaWeight > (int8_t)System_fillStats.weightUnitsReceived) {							//have more units been added since last check?
					newWeightUnitsDifference = deltaWeight - (int8_t)System_fillStats.weightUnitsReceived;	//yes, have to dec those units from the ekey 
				
					//has to at least and limited to MAX_SOAP_UNITS_DECREMENT
					if(newWeightUnitsDifference >= (int8_t)MAX_SOAP_UNITS_DECREMENT) {
						newWeightUnitsDifference = MAX_SOAP_UNITS_DECREMENT;				
	#if !DEBUG_SKIP_WEIGHT_DOCK
						YELLOW_LED_OFF();
						
					
						sysData.unitsReceivedCtr += newWeightUnitsDifference;			//for RTLS monitoring of how much soap this dispenser has accepted.
					
						if( sysData.unitsReceivedCtr >= 100 ) {
							sysData.unitsReceivedCtr = 0;
							sysData.eepromData.refillAmt++;
							sysFlags.eepromDataDirty = 1;								//keep stats on amount of soap added
						}
						
						eeprom_write_byte (EEPROM_ADDR_UNITS_RECEIVED_CTR,(uint8_t)sysData.unitsReceivedCtr); // cxr20jan20
						_delay_ms(10); // cxr20jan20
						
				      while(tries<=EKEY_COMM_TRIES) // gk100
				     {	
						if( Ekey_uidRequest() ) 
						{ //communicate with ekey
							page = 0;
							_delay_ms(10);

							done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);   	//select the tag using the id obtained from TTF. This will automatically read from page 1 regardless of page value.
							if(done)break;
					
							if( !done ) {
								Ekey_getTagData()->page0 = Ekey_getAltUid();
								done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);
								if(done)break;
							}
							
							//---------------------------------j100----------------------------------------
							//----------------------------------------------------------------
							if( !done ) {
								Ekey_getTagData()->page0 = Ekey_getAltUid5();
								done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);
								if(done)break;
							}
							//----------------------------------------------------------------
							if( !done ) {
								Ekey_getTagData()->page0 = Ekey_getAltUid2();
								done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);
								if(done)break;
							}
							//----------------------------------------------------------------
							//----------------------------------------------------------------
							if( !done ) {
								Ekey_getTagData()->page0 = Ekey_getAltUid3();
								done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);
								if(done)break;
							}
							//----------------------------------------------------------------
							//----------------------------------------------------------------
							if( !done ) {
								Ekey_getTagData()->page0 = Ekey_getAltUid4();
								done = Ekey_readPage(CMD_SELECT_UID, &page, &dummy);
								if(done)break;
							}
							//----------------------------------------------------------------
						    
					    }
						tries++;
					 }  //-----------------------------check ID completes-----------------------------------
							//----------------------------------j100---------------------------------------
					
							if( done ) {  // check ID is successful  write to ekey
								if( Ekey_dockUsage(newWeightUnitsDifference, 0) ) {						//units have been added: dock them from bag ekey counter via RFID transceiver
									System_fillStats.weightUnitsReceived += newWeightUnitsDifference;	//all good: update local delivered count
									sysData.tickCounter = BAG_INSERTED_INACTIVITY_TIMEOUT;				//there has been movement of soap		
								} else {
									//what happens if usage cannot be docked? I say nothing!
									System_fillStats.ekeyDockageErrCtr++;	
									//--------------------------------------
									
									//if( --sysData.tickCounter == 0  ){			//If tick counter == 0 then bag has been left sitting there to long. Time to shut the valve.
										//StateMachine_currentState = ST_IDLE;
										//Transition_biToIdle();					//game over: took too long
										//bagIdealTimeoutFlag=1;
										
									//}
									//--------------------------------------
								}
							}
						//} //end communicate with ekey
					
						
						YELLOW_LED_ON();
						MLX_MODU_OFF();
						FMOD_OFF();
	#endif
					} else {	//weight difference is not enough to act upon
						System_fillStats.noSoapAddedCtr++;					
					}
				}	//end weight check
			
			} else {	//no more soap in this bag. It was emptied during this transaction as it did contain soap during validation
				System_fillStats.refillBagEmptied = 1;		//set boolean flag	
				
				//RED_LED_ON(); // t200
				//_delay_ms(50);// t200
				//RED_LED_OFF();// t200		
				//if( --sysData.tickCounter == 0  ){			//If tick counter == 0 then bag has been left sitting there to long. Time to shut the valve.
				StateMachine_currentState = ST_IDLE;
				Transition_biToIdle();					//game over: took too long
				//bagIdealTimeoutFlag=1;
				
				//}		
			}
		}	//reset key is inserted so no weight check etc is done
		bagIdealTimeoutFlag =0; //t100	
	}	//tick counter exit check
	
	//UartParse_rxMsg(); 
}

void StateMachine_errorFlashTick(void) {
	RED_LED_ON();
	_delay_ms(20);
	RED_LED_OFF();
}

#if DEBUG_RFID_TEST
//test for RFID functionality
void StateMachine_rfidTest(void) {
	uint8_t page, temp;	
	TAG_DATA *tData;
	static bit done;
	uint64_t firstUid;
	done = 0;
	
	tData = Ekey_getTagData();		//pointer to the tag data structure
	
	memset(tData, 0, sizeof(TAG_DATA));	//zero the ekey tag image
		
	if( Ekey_uidRequest() ) {						//read the tag UID
			
		page = 0;		
		done = Ekey_readPage(CMD_SELECT_UID, &page, &(tData->page1));			//This will automatically read from HITAG page 1 regardless of page value.
		
		if( !done ) {
			tData->page0 = Ekey_getAltUid();
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(tData->page1));
		}
		
		//----------------------------------------------------------------
		if( !done ) {
			l_tagData.page0 = Ekey_getAltUid5();
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
		}
		//----------------------------------------------------------------
		if( !done ) {
			l_tagData.page0 = Ekey_getAltUid2();
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
		}
		//----------------------------------------------------------------
		//----------------------------------------------------------------
		if( !done ) {
			l_tagData.page0 = Ekey_getAltUid3();
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
		}
		//----------------------------------------------------------------
		//----------------------------------------------------------------
		if( !done ) {
			l_tagData.page0 = Ekey_getAltUid4();
			done = Ekey_readPage(CMD_SELECT_UID, &page, &(l_tagData.page1));
		}
		//----------------------------------------------------------------
		
		//-----------------------------check ID completes-----------------------------------
		
		if(done) {			
			page = 2;
			done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page2));
					
			if(done) {
				page = 3;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page3));
			}
			if(done) {
				page = 4;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page4));
			}
			if(done) {
				page = 5;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page5));
			}
			if(done) {
				page = 6;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page6));
			}
			if(done) {
				page = 7;
				done = Ekey_readPage(CMD_RD_PAGE, &page, &(tData->page7));
			}
				
			if(done) {
				done = 0;
				if( ((int8_t)(tData->weightCounter) > (int8_t)0) && ((int8_t)(tData->useCounter) > (int8_t)0) ) {	//check the counters in page 5 data
					temp = tData->bagIdentifier;
					temp &= 0x3f;
					if(temp == GOJO_BAG_IDENTIFIER) {
						done = 1;												//ekey is validated and tag indicates that there is soap
					}
				}
			}
		}
	}
	
	MLX_MODU_OFF();			
	
	if(done) {
		RED_LED_OFF();
		YELLOW_LED_ON();		
	} else {
		RED_LED_ON();
		YELLOW_LED_OFF();
	}
	
//	TEST_HI();
	_delay_ms(20);
	RED_LED_OFF();
	YELLOW_LED_OFF();			
}

#endif


static void l_lightShow(void) {
	uint8_t i;
	
	RED_LED_ON();			//to see that both LEDS are working!
	
#if !DEBUG_EKEY_TESTER_MODE	
	for(i=0;i<15;i++) {
#else
	for(i=0;i<3;i++) {
#endif
		RED_LED_ON();
		YELLOW_LED_OFF();
		_delay_ms(500);
		RED_LED_OFF();
		YELLOW_LED_ON();
		_delay_ms(500);
	}
	
	YELLOW_LED_OFF();
	RED_LED_OFF();
}

/*	
	if(Ekey_doTTF()) {								//get UID information from HITAG via TTF of page 4
		
		edata = Ekey_getTagData();
		
		if( Ekey_fixUID(&(edata->page4and5)) ) {	//fix the TTF response to get the correct UID
			Ekey_uidRequest();						//put HITAG in 'init' state. The response for this command cannot be read as it is in anti-collision encoding
			
			page = 0;
			if(Ekey_readPage(CMD_SELECT_UID, &page, &(edata->page1)) ) {	//Select the HITAG via its UID. This will automatically respond with data from page 1 regardless of page value
				page = 5;
				if( Ekey_readPage(CMD_RD_PAGE, &page, &(edata->page5)) ) {	//read HITAG page 5 to get consumption values
					
					if( edata->bagIdentifier == GOJO_BAG_IDENTIFIER) {
						if( (edata->weightCounter == 0x7f) && (edata->useCounter == 0x7f) ) {
							allOK = 1;					//ekey is validated and tag indicates that there is soap
						}	
					}
				}
			}
		}
	}
	
	MLX_MODU_OFF();			
	
	if(allOK) {
		RED_LED_OFF();
		YELLOW_LED_ON();		
	} else {
		RED_LED_ON();
		YELLOW_LED_OFF();
	}
	
//	TEST_HI();
	_delay_ms(20);
	RED_LED_OFF();
	YELLOW_LED_OFF();			
}
*/