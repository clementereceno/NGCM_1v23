/*
 * IncFile1.h
 *
 * Created: 3/31/2016 4:45:58 PM
 *  Author: mikev
 */ 


#ifndef CRYPTO_H_
#define CRYPTO_H_

#include "System.h"

// Basic macros:

#define u8				uint8_t
#define u32				uint32_t
#define u64				uint64_t
#define rev8(x)			((((x)>>7)&1)+((((x)>>6)&1)<<1)+((((x)>>5)&1)<<2)+((((x)>>4)&1)<<3)+((((x)>>3)&1)<<4)+((((x)>>2)&1)<<5)+((((x)>>1)&1)<<6)+(((x)&1)<<7))
#define rev16(x)		(rev8 (x)+(rev8 (x>> 8)<< 8))
#define rev32(x)		(rev16(x)+(rev16(x>>16)<<16))
#define rev64(x)		(rev32(x)+(rev32(x>>32)<<32))
#define bit(x,n)		(((x)>>(n))&1)
#define bit32(x,n)		((((x)[(n)>>5])>>((n)))&1)
#define inv32(x,i,n)	((x)[(i)>>5]^=((u32)(n))<<((i)&31))
#define rotl64(x, n)	((((u64)(x))<<((n)&63))+(((u64)(x))>>((0-(n))&63)))

// Single bit Hitag2 functions:

#define i4(x,a,b,c,d)	((u32)((((x)>>(a))&1)+(((x)>>(b))&1)*2+(((x)>>(c))&1)*4+(((x)>>(d))&1)*8))

uint32_t Crypto_byte (uint64_t * x);

uint64_t Crypto_init (const uint64_t key, uint32_t serial, uint32_t IV);

#endif /* CRYPTO_H_ */