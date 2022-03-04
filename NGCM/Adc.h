/*
 * Adc.h
 *
 * Created: 8/5/2015 3:18:21 PM
 *  Author: mikev
 */ 


#ifndef ADC_H_
#define ADC_H_

#include "System.h"

uint16_t Adc_getChannel(uint8_t chan);
uint8_t Adc_getWeightUnits(void);

uint16_t Adc_getWeightOffset(void);
uint16_t Adc_getWeightGain(void);


#endif /* ADC_H_ */