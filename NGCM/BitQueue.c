#include "BitQueue.h"

/**
 *
 * @param BitQueue
 * @param size - size of bit array in bits
 */
void BitQueue_init(BIT_QUEUE *BitQueue, uint8_t size)
{
    BitQueue->limit = size;
    BitQueue->front = 0;
    BitQueue->array = 0;
}

void BitQueue_clear(BIT_QUEUE *BitQueue) {
    BitQueue->front = 0;
    BitQueue->array = 0;
}

uint8_t BitQueue_numBitsSet(BIT_QUEUE *bitQueue)
{
    uint8_t i;
    uint8_t count = 0;
    BQ_BUFFER_TYPE temp;

    temp = bitQueue->array;

    if(bitQueue->front > 0) {       //empty check
        for(i=0; i<bitQueue->front;i++) {
            if(temp & 0x01) {
                count++;
            }
            temp >>= 1;
        }
    }
    return count;
}

/**
 * Queue bit into buffer. If buffer is full, msbit is returned
 * @param BitQueue
 * @param bitVal
 * @return
 */
bit BitQueue_enqueue(BIT_QUEUE *bitQueue, uint8_t bitVal)
{
    BQ_BUFFER_TYPE temp;
    static bit retval;

    //if queue is full, get pushed out value
    if(bitQueue->front >= bitQueue->limit) {
        temp = ( 1 << (bitQueue->limit - 1) );
        temp &= bitQueue->array;
        retval = (temp != 0);
    } else {
        retval = 0;
        bitQueue->front++;
    }

    bitQueue->array <<= 1;
    temp = (BQ_BUFFER_TYPE)bitVal;
    bitQueue->array |= temp;

    return retval;
}

bit BitQueue_isFull(BIT_QUEUE *BitQueue)
{
    return(BitQueue->front >= BitQueue->limit);
}

uint8_t BitQueue_getLength(BIT_QUEUE *BitQueue) {
    return( BitQueue->front );
}

BQ_BUFFER_TYPE BitQueue_getArray(BIT_QUEUE *bitQueue) {
    return( bitQueue->array);
}

bit BitQueue_dequeue(BIT_QUEUE *BitQueue)
{
    BQ_BUFFER_TYPE temp = 0;
/*
    if(BitQueue->front > 0) {   //if not empty
        temp = BitQueue->array & (BQ_BUFFER_TYPE)0x01;
        BitQueue->array >>= 1;
        BitQueue->front--;
    }
*/
	if(BitQueue->front !=0) {
		temp = BitQueue->array & ((BQ_BUFFER_TYPE)1<<(BitQueue->front-1));
		BitQueue->front--;
	}
    return (temp != 0);
}

void BitQueue_test()
{
    uint8_t numSet;
    static bit bval;

    BIT_QUEUE bq;

    BitQueue_init(&bq, 5);

    BitQueue_enqueue(&bq,1);
    BitQueue_enqueue(&bq,1);
    BitQueue_enqueue(&bq,0);
    BitQueue_enqueue(&bq,0);
    numSet = BitQueue_isFull(&bq);
    BitQueue_enqueue(&bq,1);

    if(BitQueue_isFull(&bq)) {
        BitQueue_enqueue(&bq,1);
    } else {
        BitQueue_enqueue(&bq,0);
    }

    numSet = BitQueue_numBitsSet(&bq);
    bval = BitQueue_dequeue(&bq);

    numSet +=6;
}
