// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_MARKUP_H
#define RADDBGI_MARKUP_H

////////////////////////////////
//~ Usage Macros

#define raddbg_is_attached(...)               raddbg_is_attached__impl()
#define raddbg_thread_name(fmt, ...)          raddbg_thread_name__impl((fmt), __VA_ARGS__)
#define raddbg_thread_color_hex(hexcode)      raddbg_thread_color__impl((hexcode))
#define raddbg_thread_color_rgba(r, g, b, a)  raddbg_thread_color__impl((unsigned int)(((r)*255) << 24) | (unsigned int)(((g)*255) << 16) | (unsigned int)(((b)*255) << 8) | (unsigned int)((a)*255))
#define raddbg_break(...)                     raddbg_break__impl()
#define raddbg_break_if(expr, ...)            ((expr) ? raddbg_break__impl() : (void)0)
#define raddbg_watch(fmt, ...)                raddbg_watch__impl((fmt), __VA_ARGS__)
#define raddbg_pin(expr, ...)                 /* NOTE(rjf): inspected by debugger ui - does not change program execution */
#define raddbg_log(fmt, ...)                  raddbg_log__impl((fmt), __VA_ARGS__)

////////////////////////////////
//~ Win32 Implementations

#if defined(_WIN32)

//- types

typedef unsigned long DWORD;
typedef char const *LPCSTR;

#pragma pack(push, 8)
typedef struct THREADNAME_INFO THREADNAME_INFO;
struct THREADNAME_INFO
{
  DWORD dwType;
  LPCSTR szName;
  DWORD dwThreadID;
  DWORD dwFlags;
};
#pragma pack(pop)

//- implementations

static inline int
raddbg_is_attached__impl(void)
{
  // TODO(rjf)
  return 0;
}

static inline void
raddbg_thread_name__impl(char *fmt, ...)
{
  // TODO(rjf)
}

static inline void
raddbg_thread_color__impl(unsigned int hexcode)
{
  // TODO(rjf)
}

#define raddbg_break__impl() (__debugbreak())

static inline void
raddbg_watch__impl(char *fmt, ...)
{
  // TODO(rjf)
}

static inline void
raddbg_log__impl(char *fmt, ...)
{
  // TODO(rjf)
}

#endif // defined(_WIN32)

#endif // RADDBGI_MARKUP_H
