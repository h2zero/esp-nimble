#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void ar_printf(const char *format, ...);
/*int _write(int file, const char * p_char, int len);*/
#define console_printf(_fmt, ...) ar_printf(_fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H__ */