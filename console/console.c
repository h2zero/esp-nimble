#ifndef ESP_PLATFORM
/*
#include <stdarg.h>
#include "console.h"
#include <Arduino.h>

void ar_printf(const char *format, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
    {
        if(*p == '\n')
            Serial.print('\r');
        Serial.print(*p);
    }
    va_end(ap);
}
*/
#include <stdio.h>
#include "nrf.h"
#include "console.h"

int _write(int file, const char * p_char, int len)
{
    (void)file;
    int i;
    
    for (i = 0; i < len; i++)
    {
        NRF_UART0->TXD = *p_char++;
        while(!NRF_UART0->EVENTS_TXDRDY);
        NRF_UART0->EVENTS_TXDRDY = 0x0UL;
    }

    return len;
}
#endif
