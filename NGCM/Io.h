/*
 * io.h
 *
 *  Created on: Jul 22, 2015
 *      Author: mikev
 */

#ifndef IO_H_
#define IO_H_

#include "System.h"

#define PUMP_MOTOR_OFF()		BIT_CLR(PORTC, PINC1)
#define PUMP_MOTOR_ON()			BIT_SET(PORTC, PINC1)

#define PUMP_MOTOR_BRAKE_OFF() 	BIT_CLR(PORTC, PINC2)
#define PUMP_MOTOR_BRAKE_ON() 	BIT_SET(PORTC, PINC2)

#define VALVE_OPEN_ON()			BIT_SET(PORTB, PINB6)
#define VALVE_OPEN_OFF()		BIT_CLR(PORTB, PINB6)
	
#define VALVE_CLOSE_ON()		BIT_SET(PORTB, PINB7)
#define VALVE_CLOSE_OFF()		BIT_CLR(PORTB, PINB7)

#define YELLOW_LED_OFF()		BIT_SET(PORTD, PIND6)
#define YELLOW_LED_ON()			BIT_CLR(PORTD, PIND6)
#define YELLOW_LED_IS_ON()		!BIT_IS_SET(PORTD, PIND6)

#define RED_LED_OFF()			BIT_SET(PORTD, PIND7)
#define RED_LED_ON()			BIT_CLR(PORTD, PIND7)
#define RED_LED_IS_ON()			!BIT_IS_SET(PORTD, PIND7)

#define FMOD_OFF()				BIT_SET(PORTC, PINC5)
#define FMOD_ON()				BIT_CLR(PORTC, PINC5)

#define SET_MLX_DATA_SPEED_4K()	BIT_CLR(PORTC, PINC4)
#define SET_MLX_DATA_SPEED_2K()	BIT_SET(PORTC, PINC4)

#define SET_MLX_MODE_BIPHASE()	BIT_CLR(PORTC, PINC3)
#define SET_MLX_MODE_MANCH()	BIT_SET(PORTC, PINC3)

#define MLX_MODU_ON()			BIT_CLR(PORTB, PINB1)
#define MLX_MODU_OFF()			BIT_SET(PORTB, PINB1)

#define PW18_DONE_ON()			BIT_SET(PORTD, PIND2)
#define PW18_DONE_OFF()			BIT_CLR(PORTD, PIND2)

#define OPAMP_PWR_ON()			BIT_CLR(PORTD, PIND5)
#define OPAMP_PWR_OFF()			BIT_SET(PORTD, PIND5)

#define BAG_INSERTED()			!BIT_IS_SET(PORTD, PIND3)
#define BAG_NOT_INSERTED()		BIT_IS_SET(PORTD, PIND3)

#define TEST_HI()				BIT_SET(PORTB, PINB5)
#define TEST_LO()				BIT_CLR(PORTB, PINB5)

#define PIN_SW14_MOTOR			0x09, PIND4
#define PIN_BAG_SWITCH			0x09, PIND3
#define PIN_EOS_SWITCH			0x09, PIND2

#define DISABLE_EOS_INT()		(PCMSK2 &= ~(1<<PCINT18))
#define ENABLE_EOS_INT()		(PCMSK2 |= (1<<PCINT18))
#define DISABLE_MOTOR_INT()		(PCMSK2 &= ~(1<<PCINT20))
#define ENABLE_MOTOR_INT()		(PCMSK2 |= (1<<PCINT20))
#define DISABLE_BAG_INT()		(PCMSK2 &= ~(1<<PCINT19))
#define ENABLE_BAG_INT()		(PCMSK2 |= (1<<PCINT19))

#define ENABLE_RX_PIN_INT()		(PCMSK2 |= (1<<PCINT16))
#define DISABLE_RX_PIN_INT()	(PCMSK2 &= ~(1<<PCINT16))

#define PIN_DATA				0x03, PINB2
#define PIN_CLOCK				0x03, PINB0
#define PIN_OVLD				0x06, PINC6

#define PIN_RXD					0x09, PIND0
#define TXD_OUT_HIGH()			BIT_SET(PORTD, PIND1)
					
#define IO_NO_DEBOUNCE			(0x00)
#define IO_DEBOUNCE				(0xff)

#define IO_VALVE_OPEN			(0x00)
#define IO_VALVE_CLOSE			(0xff)

void Io_init(void);
void Io_setOvercurrentThreshold(int val);
void Io_valve(uint8_t openClose);
void Io_switchToComparator(void);
void Io_switchToADC(void);

int8_t Io_debouncedPinLevelHigh(const uint8_t ioPortAddress, const uint8_t pinNumber);
int8_t Io_pinLevelHigh(const uint8_t ioPortAddress, const uint8_t pinNumber);

#endif /* IO_H_ */
