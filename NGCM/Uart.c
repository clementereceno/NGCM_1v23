/*
 * Uart.c
 *
 * Created: 4/5/2016 1:44:30 PM
 *  Author: mikev
 */
#include "System.h"
#include "Uart.h"
#include <avr/interrupt.h>
#include "Io.h"

static volatile char *l_txPtr;
extern SYSTEM_FLAGS sysFlags;//
extern char treceive,bagInsertedFlag1;


void Uart_init(void){
	
	UBRR0H = 0;
	UBRR0L = F_CPU/(16.0*UART_BAUD_RATE) - 1.0;		//make sure this is 8 bits or smaller
		
	// Set frame format: 8data, 1stop bit, no parity
	UCSR0C = (3<<UCSZ00);		
}

void Uart_rxStart(void) {
	uint8_t data;
	
	Uart_rxBufIdx = 0;
	Uart_rxBuf[0] = '\0';						//clear receive buffer
	
	data = UDR0;
	// Enable receiver and its interrupt
	UCSR0B |= ( (1<<RXEN0)|(1<<RXCIE0) );
}

void Uart_stop(void) {
	// Disable receiver and its interrupt
	UCSR0B &= ~( (1<<RXEN0)|(1<<RXCIE0) );
}


void Uart_transmit(char *cptr) {
	l_txPtr = cptr;
	
	if(l_txPtr != '\0') {
		UCSR0B |= (1<<TXEN0);					//turn on transmitter
		
//		while ( !( UCSR0A & (1<<UDRE0))) {};	// Wait for empty transmit buffer
		
//		UCSR0B |= (1<<TXCIE0);					//enable interrupt for the rest of the message
//		UDR0 = *l_txPtr++;						//write to transmit register
		UCSR0B |= (1<<UDRIE0);
	}
}

void Uart_rxReset(void) {
	Uart_rxBufIdx = 0;	
	Uart_rxBuf[0] = '\0';
}

//transmitter data register empty
ISR	(USART_UDRE_vect) {
	if(*l_txPtr != '\0') {
	
		/* Put data into buffer, sends the data */
		UDR0 = *l_txPtr++;	
	} else {
		UCSR0B &= ~(1<<UDRIE0);
//		UCSR0B &= ~(1<<TXCIE0);					//disable transmit interrupt
		UCSR0B &= ~(1<<TXEN0);					//turn off transmitter
//		UCSR0B &= ~( (1<<RXEN0)|(1<<RXCIE0) );	//turn off receiver and its interrupt
		if( sysFlags.unlockSerialModeFlag ) {
			sysFlags.lockSerialMode = 0;
			sysFlags.unlockSerialModeFlag = 0;
#ifndef DEBUG_OP_AMP_ON			
			OPAMP_PWR_OFF();						//finished with the weight calibration
#endif			
		}
	}
//	UCSR0A |= (1<<TXC0);
//	UCSR0A |= (1<<UDRE0);
}

ISR (USART_RX_vect) {
	uint8_t data;
	
	data = UDR0;
	
	if( Uart_rxBufIdx < (UART_RX_BUF_SIZE-1) ) {
	
		if(data == '\r') {
			Uart_rxBuf[Uart_rxBufIdx++] = '\0';
			BIT_SET(System_interruptFlags, SYSTEM_INTFLG_UARTRX);
			if(bagInsertedFlag1==1)treceive=1; // t100
		} else {
			if(data != '\n') {
				Uart_rxBuf[Uart_rxBufIdx++] = data;				
			}
		}
	
	} else {
		Uart_rxBufIdx = 0;}
	
		
}

