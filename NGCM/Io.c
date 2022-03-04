/*
 * io.c
 *
 *  Created on: Jul 22, 2015
 *      Author: mikev
 */

#include <avr/interrupt.h>

#include "Io.h"
#include "System.h"
#include "EventStack.h"

extern SYSTEM_DATA sysData;

void Io_init() {
	//writing to PORT bit when the bit is configured as an input turns on the pull-up resistor
	//writing to PORT bit when bit is configured as an output drives the pin level
	//writing '1' to the PIN bit toggles the port pin
//	DDRA = 0b00001111;		// '1' means output. PA1 (NC)
	
	//DDRB = 0b11111010;   // f100  v1.13 and less
	DDRB = 0b11011010;   //   f100   // bit 5 cleared  for ovld
	BIT_CLR(PORTB, PB2);	//Melexis DATA input. Pull-up is external.
	BIT_CLR(PORTB, PB0);	//Melexis CLK input. Pull-up is external.
	BIT_CLR(PORTB, PB5);	// OVLD input. Pull-up is external.  // f100
	VALVE_OPEN_OFF();
	VALVE_CLOSE_OFF();
	MLX_MODU_OFF();
	TEST_HI();				//port b pin 5 used for debug scope trigger etc.
	
	DDRC = 0b10111110;
	BIT_CLR(PORTC, PINC0);	//motor current signal pull-up off
	BIT_SET(PORTC, PINC6);	//reset signal pull-up on
	PUMP_MOTOR_OFF();
	PUMP_MOTOR_BRAKE_OFF();
	SET_MLX_MODE_MANCH();
	SET_MLX_DATA_SPEED_4K();
	FMOD_OFF();
	//PC7 does not exist
	
	TXD_OUT_HIGH();			//RS233 Tx pin
	
	DDRD = 0b11100010;	
	BIT_SET(PORTD, PIND0);	//RxD pull-up on
	BIT_SET(PORTD, PIND2);	//pull-up on for EOS switch
	BIT_SET(PORTD, PIND3);	//pull-up on for cover
//	BIT_SET(PORTD, PIND4);	//pull-up on for MOTORON in case upper deck board is disconnected
	TXD_OUT_HIGH();	 
	OPAMP_PWR_OFF();
	YELLOW_LED_OFF();
	RED_LED_OFF();
	
	DIDR0 = (1<<ADC0D);		//motor current monitor
//	DIDR1 = (1<<AIN1D) | (1<<AIN0D);	//disable digital inputs for comparator inputs
	
//	PW18_DONE_ON();

#ifdef DEBUG_OP_AMP_ON
	OPAMP_PWR_ON();
#else
	OPAMP_PWR_OFF();				//op amp used for soap weight
#endif
		
//set up of motor over-current detection
/*
	Io_setOvercurrentThreshold(0);	//set this to 0 to save energy. Analog comparator threshold only needed when motor is running
	
	BIT_SET(ACSR, ACD);				//disable analog comparator for now. When turned on, time must be given to allow bandgap reference to stabilize.
//	BIT_SET(ACSR, ACBG);			//use internal bandgap reference (1.1V) as AIN0(+). If cleared, uses external pin voltage as AIN0(+)
	BIT_CLR(ACSR, ACIS0);			//
	BIT_SET(ACSR, ACIS1);			//comparator interrupt on falling edge
	BIT_CLR(ACSR, ACI);				//clear comparator interrupt flag
	BIT_CLR(ACSR, ACIE);			//clear comparator interrupt enable
*/

//MOTOR signal from SW014 detection	
	BIT_SET(PCMSK2, PCINT20);		//PCMSK2 – Pin Change Mask Register 2. Enable only PCINT20 - PD4(MOTOR) signal from SW014
	BIT_SET(PCMSK2, PCINT19);		//bag inserted switch (PD3)
	BIT_SET(PCMSK2, PCINT18);		//End Of Stroke (EOS) cam switch (PD2)
	
	BIT_CLR(PCMSK0, PCINT0);		//disable PCINT0 interrupt pin (data clock from Melexis chip) PB0
	
	BIT_CLR(PCMSK0, PCINT5);		//disable PCINT5 interrupt pin (OVLD) PB5  // f100
	
//power down peripherals to save the battery
	PRR = (1<<PRTWI) | (1<<PRTIM2) | (1<<PRTIM0) | (1<<PRTIM1)| (1<<PRUSART0) | (1<<PRADC) | (1<<PRSPI);
}


void Io_setOvercurrentThreshold(int val) {
	val &= 0x03;

	switch(val) {
		case 1:
			BIT_SET(PORTD, PIND5);
			BIT_CLR(PORTC, PINC2);
			break;

		case 2:
		case 3:
			BIT_SET(PORTD, PIND5);
			BIT_SET(PORTC, PINC2);
			break;
		default:
			BIT_CLR(PORTD, PIND5);
			BIT_CLR(PORTC, PINC2);
			break;
	}
}

int8_t Io_debouncedPinLevelHigh(const uint8_t ioPortAddress, const uint8_t pinNumber) {
	int8_t this, last;
	uint8_t count;
	
	count = 0;
	
	last = BIT_IS_SET(_SFR_IO8(ioPortAddress), pinNumber);
	_delay_ms(1);
	this = BIT_IS_SET(_SFR_IO8(ioPortAddress), pinNumber);
	
	while(count < 4) {
		if(this == last) {
			count++;
		} else {
			count = 0;
		}
		last = this;
		_delay_ms(1);
		this = BIT_IS_SET(_SFR_IO8(ioPortAddress), pinNumber);	
	}
	
	return(this);
}

int8_t Io_pinLevelHigh(const uint8_t ioPortAddress, const uint8_t pinNumber) {
	uint8_t temp;
	uint8_t this, last;
	temp = 0;
	
	last = BIT_IS_SET(_SFR_IO8(ioPortAddress), pinNumber);
	
	while(temp < 3) {
		_delay_ms(1);
		this = BIT_IS_SET(_SFR_IO8(ioPortAddress), pinNumber);
		if(this == last) {
			temp++;
		} else {
			temp = 0;
		}
		last = this;
	}
	return(this);
}

void Io_valve(uint8_t openClose) {
	
	VALVE_OPEN_OFF();
	VALVE_CLOSE_OFF();
	
	if(openClose == IO_VALVE_OPEN) {
		VALVE_OPEN_ON();			
	} else {
		VALVE_CLOSE_ON();
	}
	
	//_delay_ms(200); // for <= V 1.10 
	//_delay_ms(20); // V1.11
	_delay_ms(25); // V1.16 // h100
	VALVE_OPEN_OFF();
	VALVE_CLOSE_OFF();
}

void Io_switchToComparator(void) {
			
	PRR &= ~(1<<PRADC);		//start ADC clocks or it won't get its registers updated
	BIT_CLR(ADCSRA, ADEN);	//disable AD converter

	BIT_SET(ADCSRB, ACME);	//enable the mux for use with the comparator
	
	ADMUX = (1<<REFS0) | 7;	//select channel 7 (dispenser voltage)
	
	BIT_SET(ACSR, ACIS0);
	BIT_SET(ACSR, ACIS1);	//comparator interrupt on rising edge. When battery voltage drops below threshold.
	
	BIT_SET(ACSR, ACBG);	//use and turn on bandgap
	_delay_us(100);			//delay minimum 70uS for bandgap to stabilize.
		
	BIT_CLR(ACSR, ACD);		//power up comparator
	
	BIT_SET(ACSR, ACI);		//clear comparator interrupt flag
	BIT_SET(ACSR, ACIE);	//enable comparator interrupt
	
	BIT_CLR(ADCSRA, ADEN);	//disable AD converter
	PRR |= (1<<PRADC);		//stop ADC clocks
}

void Io_switchToADC(void) {
	
	BIT_CLR(ACSR, ACIE);	//disable comparator interrupt
	BIT_CLR(ADCSRB, ACME);	//disable mux for comparator
	BIT_SET(ACSR, ACD);		//disable comparator
}

void Io_idlePark(void) {
		
}

// the following interrupt will wake up the micro indicating that a dispense should be made
ISR (PCINT2_vect) {

	BIT_SET(System_interruptFlags, SYSTEM_INTFLG_PCINT2);	//MOTOR_ON, EOS or bag inserted signal changed

//	I think the following line is done automatically
//	BIT_CLR(PCIFR, 3);				//PCIFR – Pin Change Interrupt Flag Register. clear the interrupt flag
}

ISR (ANALOG_COMP_vect) {
	YELLOW_LED_OFF();
	RED_LED_OFF();
	MLX_MODU_OFF();
	FMOD_OFF();
	OPAMP_PWR_OFF();
	
	Io_switchToADC();
//	Io_valve(IO_VALVE_CLOSE);		//power may die when executing this
	
	EventStack_push(EVENT_TIMER1);	//forces exit from bag inserted mode to idle mode

}


/*
INT0_vect
INT1_vect
Pin Change Interrupt Request 0	PCINT0_vect
Pin Change Interrupt Request 1	PCINT1_vect
Pin Change Interrupt Request 2	PCINT2_vect
Watchdog Time-out Interrupt	WDT_vect
Timer/Counter2 Compare Match A	TIMER2_COMPA_vect
Timer/Counter2 Compare Match B	TIMER2_COMPB_vect
Timer/Counter2 Overflow	TIMER2_OVF_vect
Timer/Counter1 Capture Event	TIMER1_CAPT_vect
Timer/Counter1 Compare Match A	TIMER1_COMPA_vect
Timer/Counter1 Compare Match B	TIMER1_COMPB_vect
Timer/Counter1 Overflow	TIMER1_OVF_vect
Timer/Counter0 Compare Match A	TIMER0_COMPA_vect
Timer/Counter0 Compare Match B	TIMER0_COMPB_vect
Timer/Counter0 Overflow	TIMER0_OVF_vect
SPI Serial Transfer Complete	SPI_STC_vect
USART Rx Complete	USART_RX_vect
USART Data Register Empty	USART_UDRE_vect
USART Tx Complete	USART_TX_vect
ADC Conversion Complete	ADC_vect
EEPROM Ready	EE_READY_vect
Analog Comparator	ANALOG_COMP_vect
Two-wire Serial Interface	TWI_vect
Store Program Memory Read	SPM_READY_vect
*/
