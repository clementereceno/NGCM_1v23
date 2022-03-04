// Software optimized 48-bit Philips/NXP Mifare Hitag2 PCF7936/46/47/52 stream cipher algorithm by I.C. Wiener 2006-2007.
// For educational purposes only.
// No warranties or guarantees of any kind.
// This code is released into the public domain by its author.

#include "Crypto.h"
#include <stdio.h>


static const u32 ht2_f4a = 0x2C79;		// 0010 1100 0111 1001
static const u32 ht2_f4b = 0x6671;		// 0110 0110 0111 0001
static const u32 ht2_f5c = 0x7907287B;	// 0111 1001 0000 0111 0010 1000 0111 1011

static u32 f20 (const uint64_t x)
{
	u32					i5;
	
	i5 = ((ht2_f4a >> i4 (x, 1, 2, 4, 5)) & 1)* 1
	+ ((ht2_f4b >> i4 (x, 7,11,13,14)) & 1)* 2
	+ ((ht2_f4b >> i4 (x,16,20,22,25)) & 1)* 4
	+ ((ht2_f4b >> i4 (x,27,28,30,32)) & 1)* 8
	+ ((ht2_f4a >> i4 (x,33,42,43,45)) & 1)*16;
	
	return (ht2_f5c >> i5) & 1;
}

uint64_t Crypto_init (const uint64_t key, uint32_t serial, uint32_t IV)
{
	u32					i;
	uint64_t					x = ((key & 0xFFFF) << 32) + serial;
	
	for (i = 0; i < 32; i++)
	{
		x >>= 1;
		x += (u64) (f20 (x) ^ (((IV >> i) ^ (key >> (i+16))) & 1)) << 47;
	}
	return x;
}

static uint64_t hitag2_round (uint64_t *state)
{
	uint64_t					x = *state;
	
	x = (x >>  1) +
	((((x >>  0) ^ (x >>  2) ^ (x >>  3) ^ (x >>  6)
	^ (x >>  7) ^ (x >>  8) ^ (x >> 16) ^ (x >> 22)
	^ (x >> 23) ^ (x >> 26) ^ (x >> 30) ^ (x >> 41)
	^ (x >> 42) ^ (x >> 43) ^ (x >> 46) ^ (x >> 47)) & 1) << 47);
	
	*state = x;
	return f20 (x);
}

// Bitslice Hitag2 functions:

#define ht2bs_4a(a,b,c,d)	(~(((a|b)&c)^(a|d)^b))
#define ht2bs_4b(a,b,c,d)	(~(((d|c)&(a^b))^(d|a|b)))
#define ht2bs_5c(a,b,c,d,e)	(~((((((c^e)|d)&a)^b)&(c^b))^(((d^e)|a)&((d^b)|c))))

#define uf20bs				u32		// choose your own type/width

static uf20bs f20bs (const uf20bs *x)
{
	return ht2bs_5c (
	ht2bs_4a(x[ 1],x[ 2],x[ 4],x[ 5]),
	ht2bs_4b(x[ 7],x[11],x[13],x[14]),
	ht2bs_4b(x[16],x[20],x[22],x[25]),
	ht2bs_4b(x[27],x[28],x[30],x[32]),
	ht2bs_4a(x[33],x[42],x[43],x[45]));
}

static void hitag2bs_init (uf20bs *x, const uf20bs *key, const uf20bs *serial, const uf20bs *IV)
{
	u32					i, r;
	
	for (i = 0; i < 32; i++) x[i] = serial[i];
	for (i = 0; i < 16; i++) x[32+i] = key[i];
	
	for (r = 0; r < 32; r++)
	{
		for (i = 0; i < 47; i++) x[i] = x[i+1];
		x[47] = f20bs (x) ^ IV[i] ^ key[16+i];
	}
}

static uf20bs hitag2bs_round (uf20bs *x)
{
	uf20bs				y;
	u32					i;
	
	y = x[ 0] ^ x[ 2] ^ x[ 3] ^ x[ 6] ^ x[ 7] ^ x[ 8] ^ x[16] ^ x[22]
	^ x[23] ^ x[26] ^ x[30] ^ x[41] ^ x[42] ^ x[43] ^ x[46] ^ x[47];
	
	for (i = 0; i < 47; i++) x[i] = x[i+1];
	x[47] = y;
	
	return f20bs (x);
}

// "MIKRON"		=  O  N  M  I  K  R
// Key			= 4F 4E 4D 49 4B 52		- Secret 48-bit key
// Serial		= 49 43 57 69			- Serial number of the tag, transmitted in clear
// Random		= 65 6E 45 72			- Random IV, transmitted in clear
//~28~DC~80~31	= D7 23 7F CE			- Authenticator value = inverted first 4 bytes of the keystream

// The code below must print out "D7 23 7F CE 8C D0 37 A9 57 49 C1 E6 48 00 8A B6".
// The inverse of the first 4 bytes is sent to the tag to authenticate.
// The rest is encrypted by XORing it with the subsequent keystream.

uint32_t Crypto_byte (uint64_t * x)
{
	u32					i, c;
	
	for (i = 0, c = 0; i < 8; i++) c += (u32) hitag2_round (x) << (i^7);
	return c;
}

int test (void)
{
	u32					i;
	u64					state;
								 //key="RKIMNO"           S/N of tag          RANDOM 4 bytes
	state = Crypto_init (rev64 (0x524B494D4E4FUL), rev32 (0x69574349), rev32 (0x72456E65));
	for (i = 0; i < 16; i++) printf ("%02X ", (uint8_t)Crypto_byte (&state));
	printf ("\n");
	return 0;
}