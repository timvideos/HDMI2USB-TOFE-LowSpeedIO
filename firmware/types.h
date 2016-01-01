#ifndef PIC_TYPE_H
#define	PIC_TYPE_H

typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short int uint16_t;
typedef short int int16_t;

typedef unsigned char bool;
#define true 1
#define false 0

#define MSN(x)	((((uint8_t)(x)) & 0xf0) >> 4)
#define LSN(x)	(((uint8_t)(x)) & 0x0f)

#define LSB(x)	((uint8_t)(((uint16_t)(x)) & 0x00ff))
#define MSB(x)	((uint8_t)((((uint16_t)(end)) & 0xff00) >> 8))

#endif
