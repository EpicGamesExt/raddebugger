// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MARKUP_H
#define BASE_MARKUP_H

#define RADDBG_MARKUP_IMPLEMENTATION
#define RADDBG_MARKUP_VSNPRINTF raddbg_vsnprintf
#if OS_LINUX
# define RADDBG_MARKUP_STUBS
#endif
#include "lib_raddbg_markup/raddbg_markup.h"

#if !defined(LAYER_COLOR)
# define LAYER_COLOR 0x404040ff
#endif

internal void set_thread_name(String8 string);
internal void set_thread_namef(char *fmt, ...);
#define ThreadNameF(...) (set_thread_namef(__VA_ARGS__), raddbg_thread_color_u32(LAYER_COLOR))

#endif // BASE_MARKUP_H
