#include "Util.h"

uint8_t ascHexBound(uint8_t chkbyte);
char nibbleToChar(uint8_t nibble);

uint32_t Util_hexStringToLong(char *c) {
    uint8_t	i;
    int32_t	retval = 0;
    uint8_t a;

    a = Util_strlen(c);

    for(i=0;i<a;i++) {
        retval = retval + ( ((int32_t) ascHexBound(*c++)) << (((a-1)-i)*4));
    }

    return(retval);
}

uint8_t ascHexBound(uint8_t chkbyte) {

  if( (chkbyte >= 48) && (chkbyte <=57) )
    return(chkbyte - 48);
  else
    if( (chkbyte >= 65) && (chkbyte <= 70) )
      return(chkbyte - 55);
    else
      return(0);
}

uint8_t Util_strlen(char *str) {
    uint8_t b = 0;

    while(*str != '\0') {
        b++; str++;
    }
    return(b);
}

void Util_strcpy(char *dest, const char *src) {
    while(*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void Util_strcat(char *dest, const char *src) {

    while(*dest != '\0') {
        dest++;
    }

    while(*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void Util_byteToString(char *ptr, uint8_t byt) {
    *ptr++ = nibbleToChar(byt >> 4);
    *ptr++ = nibbleToChar(byt & 0x0f);
    *ptr = '\0';
}

void Util_longToString(char *buf, uint32_t val) {
    uint8_t *ptr;

    ptr = (uint8_t *)&val;

    Util_byteToString(buf,*(ptr+3));
    Util_byteToString((buf+2),*(ptr+2));
    Util_byteToString((buf+4),*(ptr+1));
    Util_byteToString((buf+6),*(ptr));
}

void Util_bytesToString(char *buf, uint8_t *bytes, uint8_t numBytes) {
	uint8_t i;
	
	for(i=0;i<numBytes;i++) {
		Util_byteToString( (buf+2*i), *(bytes+i) );
	}
}

void Util_wordToString(char *buf, uint16_t val) {
    uint8_t *ptr;
    ptr = (uint8_t *)&val;

    Util_byteToString(buf,*(ptr+1));
    Util_byteToString((buf+2),*(ptr));
}

char nibbleToChar(uint8_t nibble) {
    char retval;

    if(nibble < 0x0a) {
        retval = nibble + '0';
    } else {
        retval = nibble - 0x0a + 'A';
    }
    return retval;
}

void Util_memcpy(char *dest, const char *src, uint8_t numBytes) {
    uint8_t i;

    for(i=0;i<numBytes;i++){
        *dest++ = *src++;
    }
}

uint8_t Util_hex2bcd (uint8_t x)
{
    uint8_t y;
    y = (x / 10) << 4;
    y = y | (x % 10);
    return (y);
}
