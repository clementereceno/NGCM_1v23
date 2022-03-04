/*
 * Adc.c
 *
 * Created: 8/5/2015 3:17:53 PM
 *  Author: mikev
 */ 
#include "System.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "Adc.h"

uint16_t Adc_getChannel(uint8_t chan) {
	
	uint16_t rawVal;

	PRR &= ~(1<<PRADC);		//start ADC clocks
	
	_delay_us(100);

	ADMUX = (1<<REFS0) | chan;	//select Vcc as ADC reference, select channel ADC6 for battery measurement
	
	set_sleep_mode(SLEEP_MODE_ADC);
	
	ADCSRA = (1<<ADEN);		//enable A/D
	ADCSRA |= (1<<ADIE);	//enable A/D interrupt
	ADCSRA |= 3;			//select clock divider of 8 for 125Khz
	ADCSRA |= (1<<ADSC);	//start A/D conversion
	
	cli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	sei();
	
	rawVal = (uint16_t)ADCL;
	rawVal |= ((uint16_t)ADCH << 8);
	
	ADCSRA = 0;		//disable ADC
	
	PRR |= (1<<PRADC);		//stop ADC clocks
	
	return(rawVal);	
}

uint8_t Adc_getWeightUnits(void) {
	uint16_t offset;
	uint16_t gain;
	uint16_t adcVal;
	
	offset = Adc_getWeightOffset();
	gain = Adc_getWeightGain();
		
	adcVal = Adc_getChannel(ADC_WEIGHT_CHAN);
	
	if(adcVal > offset) {
		adcVal -= offset;			//offset correction
		adcVal = (float)adcVal		//gain correction
				/(float)gain*DISPENSER_FULL_SCALE_SOAP_UNITS;
		if( adcVal > DISPENSER_FULL_SCALE_SOAP_UNITS ) {	//saturate at maximum
			adcVal = DISPENSER_FULL_SCALE_SOAP_UNITS;
		}
	} else {
		adcVal = 0;
	}

	return(adcVal);
}

uint16_t Adc_getWeightGain(void) {
	uint16_t gain;
	
	gain = eeprom_read_word(EEPROM_ADDR_GAIN);
	if( (gain > WEIGHT_GAIN_CHECK_MAX) || (gain < WEIGHT_GAIN_CHECK_MIN) ) {
		gain = DEFAULT_WEIGHT_GAIN;
	}
	return gain;	
}

uint16_t Adc_getWeightOffset(void) {
	uint16_t offset;
	
	offset = eeprom_read_word(EEPROM_ADDR_OFFSET);
	if( (offset > WEIGHT_OFFSET_CHECK_MAX) || (offset < WEIGHT_OFFSET_CHECK_MIN) ) {
		offset = DEFAULT_WEIGHT_OFFSET;
	}	
	
	return offset;
}

ISR (ADC_vect) {
	asm("nop");
}
