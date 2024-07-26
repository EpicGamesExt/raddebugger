// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_MARKUP_H
#define RADDBG_MARKUP_H

////////////////////////////////
//~ Implementation Overrides

#if !defined(raddbg_markup_vsnprintf)
# define raddbg_markup_vsnprintf vsnprintf
#endif

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

#endif // RADDBG_MARKUP_H
