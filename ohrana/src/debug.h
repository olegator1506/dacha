#ifndef __DEBUG_H__
#define __DEBUG_H__
extern bool debugMode,debugSerial;
extern void DBG(const char *fmt, ... );
extern void DBGT(const char *fmt, ... );
#ifdef F 
extern void DBG(const __FlashStringHelper *format, ... );
#endif
void checkFreeRam(void);
#endif