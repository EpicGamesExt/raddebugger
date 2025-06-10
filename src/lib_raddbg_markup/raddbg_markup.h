// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_MARKUP_H
#define RADDBG_MARKUP_H

////////////////////////////////
//~ Implementation Overrides

#if !defined(RADDBG_MARKUP_VSNPRINTF)
# define RADDBG_MARKUP_DEFAULT_VSNPRINTF 1
# define RADDBG_MARKUP_VSNPRINTF vsnprintf
#endif

#if !defined(RADDBG_MARKUP_STL_TYPE_VIEWS)
# define RADDBG_MARKUP_STL_TYPE_VIEWS 1
#endif

////////////////////////////////
//~ Usage Macros

#if !defined(RADDBG_MARKUP_STUBS)
# define raddbg_is_attached(...)                      raddbg_is_attached__impl()
# define raddbg_thread_id(...)                        raddbg_thread_id__impl()
# define raddbg_thread_name(fmt, ...)                 raddbg_thread_name__impl(raddbg_thread_id(), (fmt), __VA_ARGS__)
# define raddbg_thread_id_name(id, fmt, ...)          raddbg_thread_name__impl((id), (fmt), __VA_ARGS__)
# define raddbg_thread_color_u32(u32)                 raddbg_thread_color__impl(raddbg_thread_id(), (u32))
# define raddbg_thread_color_rgba(r, g, b, a)         raddbg_thread_color__impl(raddbg_thread_id(), ((unsigned int)((r)*255) << 24) | ((unsigned int)((g)*255) << 16) | ((unsigned int)((b)*255) << 8) | ((unsigned int)(a)*255))
# define raddbg_thread_id_color_u32(id, u32)          raddbg_thread_color__impl((id), (u32))
# define raddbg_thread_id_color_rgba(id, r, g, b, a)  raddbg_thread_color__impl((id), ((unsigned int)((r)*255) << 24) | ((unsigned int)((g)*255) << 16) | ((unsigned int)((b)*255) << 8) | ((unsigned int)(a)*255))
# define raddbg_break(...)                            raddbg_break__impl()
# define raddbg_break_if(expr, ...)                   ((expr) ? raddbg_break__impl() : (void)0)
# define raddbg_watch(fmt, ...)                       raddbg_watch__impl((fmt), __VA_ARGS__)
# define raddbg_pin(expr, ...)                        /* NOTE(rjf): inspected by debugger ui - does not change program execution */
# define raddbg_log(fmt, ...)                         raddbg_log__impl((fmt), __VA_ARGS__)
# define raddbg_entry_point(...)                      raddbg_exe_data char raddbg_gen_data_id()[] = ("entry_point: \"" #__VA_ARGS__ "\"")
# define raddbg_type_view(type, ...)                  raddbg_exe_data char raddbg_gen_data_id()[] = ("type_view: {type: ```" #type "```, expr: ```" #__VA_ARGS__ "```}")
# define raddbg_add_breakpoint(ptr, size, r, w, x)    raddbg_add_or_remove_breakpoint__impl((ptr), (1), (size), (r), (w), (x))
# define raddbg_remove_breakpoint(ptr, size, r, w, x) raddbg_add_or_remove_breakpoint__impl((ptr), (0), (size), (r), (w), (x))
# define raddbg_annotate_vaddr_range(ptr, size, ...)  raddbg_annotate_vaddr_range__impl((ptr), (size), __VA_ARGS__)
#else
# define raddbg_is_attached(...)                      (0)
# define raddbg_thread_id(...)                        ((void)0)
# define raddbg_thread_name(fmt, ...)                 ((void)0)
# define raddbg_thread_id_name(id, fmt, ...)          ((void)0)
# define raddbg_thread_color_u32(u32)                 ((void)0)
# define raddbg_thread_color_rgba(r, g, b, a)         ((void)0)
# define raddbg_thread_id_color_u32(id, u32)          ((void)0)
# define raddbg_thread_id_color_rgba(id, r, g, b, a)  ((void)0)
# define raddbg_break(...)                            ((void)0)
# define raddbg_break_if(expr, ...)                   ((void)expr)
# define raddbg_watch(fmt, ...)                       ((void)0)
# define raddbg_pin(expr, ...)
# define raddbg_log(fmt, ...)                         ((void)0)
# define raddbg_entry_point(...)                      struct raddbg_gen_data_id(){int __unused__;}
# define raddbg_type_view(type, ...)                  struct raddbg_gen_data_id(){int __unused__;}
# define raddbg_add_breakpoint(ptr, size, r, w, x)    ((void)0)
# define raddbg_remove_breakpoint(ptr, size, r, w, x) ((void)0)
# define raddbg_annotate_vaddr_range(ptr, size, ...)  ((void)0)
#endif

////////////////////////////////
//~ Helpers

#define raddbg_glue_(a, b) a##b
#define raddbg_glue(a, b) raddbg_glue_(a, b)
#define raddbg_gen_data_id() raddbg_glue(raddbg_data__, __COUNTER__)

////////////////////////////////
//~ Global Symbols

#if !defined(RADDBG_MARKUP_STUBS)
int raddbg_is_attached__impl(void);
int raddbg_thread_id__impl(void);
void raddbg_thread_name__impl(int id, char *fmt, ...);
void raddbg_thread_color__impl(int id, unsigned int hexcode);
void raddbg_watch__impl(char *fmt, ...);
void raddbg_log__impl(char *fmt, ...);
void raddbg_add_or_remove_breakpoint__impl(void *ptr, int set, int size, int r, int w, int x);
void raddbg_annotate_vaddr_range__impl(void *ptr, unsigned __int64 size, char *fmt, ...);
#endif

////////////////////////////////
//~ Win32 Implementations

#if defined(RADDBG_MARKUP_IMPLEMENTATION) && !defined(RADDBG_MARKUP_STUBS)
#if defined(_WIN32)

//- default includes
#if RADDBG_MARKUP_DEFAULT_VSNPRINTF
#include <stdio.h>
#endif

//- section allocating
#pragma section(".raddbg", read, write)
#define raddbg_exe_data __declspec(allocate(".raddbg"))

//- first byte of exe data section -> is attached
static raddbg_exe_data unsigned char raddbg_is_attached_byte_marker[1];

//- types

typedef int BOOL;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
typedef unsigned long DWORD;
typedef wchar_t WCHAR;
typedef char const *LPCSTR;
typedef const WCHAR *LPCWSTR, *PCWSTR;
typedef LONG HRESULT;
typedef void *HANDLE;
struct HINSTANCE__;
typedef struct HINSTANCE__ *HMODULE;
typedef __int64 INT_PTR;
typedef INT_PTR (*FARPROC)();

//- prototypes

#include <stdarg.h>

#if defined(__cplusplus)
extern "C"
{
#endif
  __declspec(dllimport) HMODULE LoadLibraryA(LPCSTR name);
  __declspec(dllimport) FARPROC GetProcAddress(HMODULE module, LPCSTR name);
  __declspec(dllimport) BOOL FreeLibrary(HMODULE mod);
  __declspec(dllimport) HANDLE GetCurrentThread(void);
  __declspec(dllimport) DWORD GetCurrentThreadId(void);
  __declspec(dllimport) void RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, const ULONG_PTR *lpArguments);
  __declspec(dllimport) void OutputDebugStringA(LPCSTR buffer);
  long long _InterlockedCompareExchange64(long long volatile*, long long, long long);
  long long _InterlockedExchangeAdd64(long long volatile*, long long);
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#if RADDBG_MARKUP_DEFAULT_VSNPRINTF
  int RADDBG_MARKUP_VSNPRINTF(char * const, unsigned long long const, const char * const, va_list);
#endif
#if defined(__cplusplus)
}
#endif

//- helpers

typedef struct RADDBG_MARKUP_UnicodeDecode RADDBG_MARKUP_UnicodeDecode;
struct RADDBG_MARKUP_UnicodeDecode
{
  unsigned __int32 inc;
  unsigned __int32 codepoint;
};
static __int8 raddbg_utf8_class[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5};

static RADDBG_MARKUP_UnicodeDecode
raddbg_decode_utf8(char *str, unsigned __int64 max)
{
  RADDBG_MARKUP_UnicodeDecode result = {1, 0xffffffff};
  unsigned __int8 byte = str[0];
  unsigned __int8 byte_class = raddbg_utf8_class[byte >> 3];
  switch(byte_class)
  {
    case 1:
    {
      result.codepoint = byte;
    }break;
    case 2:
    if(2 < max)
    {
      char cont_byte = str[1];
      if(raddbg_utf8_class[cont_byte >> 3] == 0)
      {
        result.codepoint = (byte & 0x0000001f) << 6;
        result.codepoint |= (cont_byte & 0x0000003f);
        result.inc = 2;
      }
    }break;
    case 3:
    if(2 < max)
    {
      char cont_byte[2] = {str[1], str[2]};
      if(raddbg_utf8_class[cont_byte[0] >> 3] == 0 &&
         raddbg_utf8_class[cont_byte[1] >> 3] == 0)
      {
        result.codepoint = (byte & 0x0000000f) << 12;
        result.codepoint |= ((cont_byte[0] & 0x0000003f) << 6);
        result.codepoint |=  (cont_byte[1] & 0x0000003f);
        result.inc = 3;
      }
    }break;
    case 4:
    if(3 < max)
    {
      char cont_byte[3] = {str[1], str[2], str[3]};
      if(raddbg_utf8_class[cont_byte[0] >> 3] == 0 &&
         raddbg_utf8_class[cont_byte[1] >> 3] == 0 &&
         raddbg_utf8_class[cont_byte[2] >> 3] == 0)
      {
        result.codepoint = (byte & 0x00000007) << 18;
        result.codepoint |= ((cont_byte[0] & 0x0000003f) << 12);
        result.codepoint |= ((cont_byte[1] & 0x0000003f) <<  6);
        result.codepoint |=  (cont_byte[2] & 0x0000003f);
        result.inc = 4;
      }
    }
  }
  return result;
}

static unsigned __int32
raddbg_encode_utf16(wchar_t *str, unsigned __int32 codepoint)
{
  unsigned __int32 inc = 1;
  if(codepoint == 0xffffffff)
  {
    str[0] = (wchar_t)'?';
  }
  else if(codepoint < 0x10000)
  {
    str[0] = (wchar_t)codepoint;
  }
  else
  {
    unsigned __int32 v = codepoint - 0x10000;
    str[0] = (wchar_t)(0xD800 + (v >> 10));
    str[1] = (wchar_t)(0xDC00 + (v & 0x000003ff));
    inc = 2;
  }
  return inc;
}

//- implementations

int
raddbg_is_attached__impl(void)
{
  return !!raddbg_is_attached_byte_marker[0];
}

int
raddbg_thread_id__impl(void)
{
  DWORD result = GetCurrentThreadId();
  return result;
}

void
raddbg_thread_name__impl(int id, char *fmt, ...)
{
  // rjf: resolve variadic arguments
  char buffer[512] = {0};
  char *name = buffer;
  {
    va_list args;
    va_start(args, fmt);
    RADDBG_MARKUP_VSNPRINTF(buffer, sizeof(buffer), fmt, args);
    va_end(args);
  }
  
  // rjf: get windows 10 style procedure
  HRESULT (*SetThreadDescription_function)(HANDLE hThread, PCWSTR lpThreadDescription) = 0;
  {
    static HRESULT (*global_SetThreadDescription_function)(HANDLE hThread, PCWSTR lpThreadDescription);
    static volatile __int64 global_SetThreadDescription_init_started;
    static volatile __int64 global_SetThreadDescription_init_done;
    __int64 do_init = !_InterlockedCompareExchange64(&global_SetThreadDescription_init_started, 1, 0);
    if(do_init)
    {
      HMODULE module = LoadLibraryA("kernel32.dll");
      global_SetThreadDescription_function = (HRESULT (*)(HANDLE, PCWSTR))GetProcAddress(module, "SetThreadDescription");
      FreeLibrary(module);
      _InterlockedExchangeAdd64(&global_SetThreadDescription_init_done, 1);
    }
    for(;_InterlockedExchangeAdd64(&global_SetThreadDescription_init_done, 0) == 0;)
    {
      // NOTE(rjf): busy-loop, until init is done
    }
    SetThreadDescription_function = global_SetThreadDescription_function;
  }
  
  // rjf: set thread name, windows 10 style
  if(SetThreadDescription_function && id == GetCurrentThreadId())
  {
    WCHAR buffer16[1024] = {0};
    int name_length = 0;
    for(;name[name_length]; name_length += 1);
    int write_offset = 0;
    for(int idx = 0; idx < name_length;)
    {
      RADDBG_MARKUP_UnicodeDecode decode = raddbg_decode_utf8(name+idx, name_length-idx);
      write_offset += raddbg_encode_utf16(buffer16 + write_offset, decode.codepoint);
      idx += decode.inc;
    }
    SetThreadDescription_function(GetCurrentThread(), buffer16);
  }
  
  // rjf: set thread name, raise-exception style
  {
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
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = id;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
      RaiseException(0x406D1388, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
    }
    __except(1)
    {
    }
#pragma warning(pop)
  }
}

void
raddbg_thread_color__impl(int id, unsigned int hexcode)
{
  if(raddbg_is_attached())
  {
#pragma pack(push, 8)
    typedef struct RADDBG_ThreadColorInfo RADDBG_ThreadColorInfo;
    struct RADDBG_ThreadColorInfo
    {
      DWORD dwThreadID;
      DWORD _pad_0;
      DWORD rgba;
      DWORD _pad_1;
    };
#pragma pack(pop)
    RADDBG_ThreadColorInfo info;
    info.dwThreadID = id;
    info.rgba = hexcode;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
      RaiseException(0x00524144u, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
    }
    __except(1)
    {
    }
#pragma warning(pop)
  }
}

#define raddbg_break__impl() (__debugbreak())

void
raddbg_watch__impl(char *fmt, ...)
{
  // TODO(rjf)
}

void
raddbg_log__impl(char *fmt, ...)
{
  // rjf: resolve variadic arguments
  char buffer[4096];
  {
    va_list args;
    va_start(args, fmt);
    RADDBG_MARKUP_VSNPRINTF(buffer, sizeof(buffer), fmt, args);
    va_end(args);
  }
  
  // rjf: output debug string
  OutputDebugStringA(buffer);
}

void
raddbg_add_or_remove_breakpoint__impl(void *ptr, int set, int size, int r, int w, int x)
{
  if(raddbg_is_attached())
  {
#pragma pack(push, 8)
    typedef struct RADDBG_AddBreakpointInfo RADDBG_AddBreakpointInfo;
    struct RADDBG_AddBreakpointInfo
    {
      unsigned __int64 vaddr;
      unsigned __int64 size;
      unsigned __int64 r;
      unsigned __int64 w;
      unsigned __int64 x;
      unsigned __int64 add;
    };
#pragma pack(pop)
    RADDBG_AddBreakpointInfo info;
    info.vaddr = (unsigned __int64)ptr;
    info.size  = size;
    info.r     = r;
    info.w     = w;
    info.x     = x;
    info.add   = set;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
      RaiseException(0x00524145u, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
    }
    __except(1)
    {
    }
#pragma warning(pop)
  }
}

void
raddbg_annotate_vaddr_range__impl(void *ptr, unsigned __int64 size, char *fmt, ...)
{
  if(raddbg_is_attached())
  {
    // rjf: resolve variadic arguments
    char buffer[4096];
    int buffer_size = 0;
    {
      va_list args;
      va_start(args, fmt);
      buffer_size = RADDBG_MARKUP_VSNPRINTF(buffer, sizeof(buffer), fmt, args);
      va_end(args);
    }
    
    // rjf: send annotation info via exception
#pragma pack(push, 8)
    typedef struct RADDBG_VaddrRangeAnnotationInfo RADDBG_VaddrRangeAnnotationInfo;
    struct RADDBG_VaddrRangeAnnotationInfo
    {
      unsigned __int64 vaddr;
      unsigned __int64 size;
      void *name;
      unsigned __int64 name_size;
    };
#pragma pack(pop)
    RADDBG_VaddrRangeAnnotationInfo info;
    info.vaddr     = (unsigned __int64)ptr;
    info.size      = size;
    info.name      = buffer;
    info.name_size = buffer_size;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
      RaiseException(0x00524156u, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
    }
    __except(1)
    {
    }
#pragma warning(pop)
  }
}

#endif // defined(_WIN32)
#endif // defined(RADDBG_MARKUP_IMPLEMENTATION)

////////////////////////////////
//~ Win32 STL Type Views

#if defined(_WIN32) && defined(RADDBG_MARKUP_IMPLEMENTATION) && RADDBG_MARKUP_STL_TYPE_VIEWS
# if defined(_VECTOR_)
raddbg_type_view(std::vector<?>, slice(_Mypair._Myval2));
# endif
# if defined(_MEMORY_)
raddbg_type_view(std::unique_ptr<?>, _Mypair._Myval2);
# endif
# if defined(_STRING_)
raddbg_type_view(std::basic_string<?>, _Mypair._Myval2._Myres <= 15 ? _Mypair._Myval2._Bx._Buf : array(_Mypair._Myval2._Bx._Ptr, _Mypair._Myval2._Mysize));
# endif
# if defined(_STRING_VIEW_)
raddbg_type_view(std::basic_string_view<?>, array(_Mydata, _Mysize));
# endif
#endif // defined(_WIN32) && defined(RADDBG_MARKUP_IMPLEMENTATION) && RADDBG_MARKUP_STL_TYPE_VIEWS

#endif // RADDBG_MARKUP_H
