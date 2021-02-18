#ifndef ESP_PLATFORM

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

#endif
