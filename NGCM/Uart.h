/*
 * Uart.h
 *
 * Created: 4/5/2016 1:45:10 PM
 *  Author: mikev
 */ 


#ifndef UART_H_
#define UART_H_
#include "System.h"

//
// RS232 Globals
//

#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE            ((uint8_t)10)
#endif

#ifndef UART_TX_BUF_SIZE
#define UART_TX_BUF_SIZE            ((uint8_t)10)
#endif

char Uart_txBuf[UART_TX_BUF_SIZE];
char Uart_rxBuf[UART_RX_BUF_SIZE];


volatile uint8_t Uart_rxBufIdx;

void Uart_init(void);
void Uart_transmit(char *cptr);
void Uart_rxReset(void);
void Uart_rxStart(void);
void Uart_stop(void);

#endif /* UART_H_ */