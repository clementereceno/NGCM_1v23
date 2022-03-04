/* 
 * File:   Util.h
 * Author: mikev
 *
 * Created on August 28, 2013, 1:59 PM
 */

#ifndef UTIL_H
#define	UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <stdint.h>
    
uint32_t Util_hexStringToLong(char *c);
uint8_t Util_strlen(char *str);
void Util_strcpy(char *dest, const char *src);
void Util_byteToString(char *ptr, uint8_t byt);
void Util_strcat(char *dest, const char *src);
void Util_memcpy(char *dest, const char *src, uint8_t numBytes);
uint8_t Util_hex2bcd (uint8_t x);
void Util_longToString(char *buf, uint32_t val);
void Util_wordToString(char *buf, uint16_t val);
void Util_bytesToString(char *buf, uint8_t *bytes, uint8_t numBytes);

#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

