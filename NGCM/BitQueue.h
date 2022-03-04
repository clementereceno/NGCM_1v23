/* 
 * File:   CircularBitBuffer.h
 * Author: mikev
 *
 * Created on June 13, 2013, 10:43 AM
 */

#ifndef BitQueue_H
#define	BitQueue_H


#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "System.h"

//#define bit bool
   
#ifndef BQ_BUFFER_TYPE
#define BQ_BUFFER_TYPE uint32_t
#endif
    
    typedef struct {
        BQ_BUFFER_TYPE array;
        uint8_t front;
//        uint8_t back;
        uint8_t limit;
    } BIT_QUEUE;

    void BitQueue_init(BIT_QUEUE *BitQueue, uint8_t size);
    void BitQueue_clear(BIT_QUEUE *BitQueue);
    uint8_t BitQueue_numBitsSet(BIT_QUEUE *BitQueue);
    bit BitQueue_enqueue(BIT_QUEUE *BitQueue, uint8_t bitVal);
    bit BitQueue_dequeue(BIT_QUEUE *BitQueue);
    bit BitQueue_isFull(BIT_QUEUE *BitQueue);
    void BitQueue_test(void);
    uint8_t BitQueue_getLength(BIT_QUEUE *BitQueue);
    BQ_BUFFER_TYPE BitQueue_getArray(BIT_QUEUE *BitQueue);

#ifdef	__cplusplus
}
#endif

#endif	/* CIRCULARBITBUFFER_H */

