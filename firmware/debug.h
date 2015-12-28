#ifndef DEBUG_H
#define	DEBUG_H

#include "types.h"

#if false //def NDEBUG

#define debug_init()
#define debug_service()
#define debug_can_send() true
#define debug_send_data(c)

#else

void debug_init(void);
void debug_service(void);
bool debug_can_send(void);
void debug_send_data(char* data);
void debug_send_cdata(const const char *data);

#endif	/* DEBUG_H */
