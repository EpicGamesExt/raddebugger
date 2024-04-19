// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MARKUP_H
#define BASE_MARKUP_H

internal void thread_name(String8 string);
internal void thread_namef(char *fmt, ...);
#define ThreadNameF(...) (ProfThreadName(__VA_ARGS__), thread_namef(__VA_ARGS__))
#define ThreadName(str) (ProfThreadName("%.*s", str8_varg(str)), thread_name(str))

#endif // BASE_MARKUP_H
