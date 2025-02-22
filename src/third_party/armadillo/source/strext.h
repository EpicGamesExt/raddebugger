#ifndef _STREXT_H_
#define _STREXT_H_

#include <stdarg.h>

S32 vconcat(char **, const char *, va_list);
S32 concat(char **, const char *, ...);

#endif
