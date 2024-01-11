// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_INS_H
#define BASE_INS_H

////////////////////////////////
// NOTE(allen): Implementations of Intrinsics

#if OS_WINDOWS

# include <windows.h>
# include <tmmintrin.h>
# include <wmmintrin.h>
# include <intrin.h>

# if ARCH_X64
#  define ins_atomic_u64_eval(x) InterlockedAdd((volatile LONG *)(x), 0)
#  define ins_atomic_u64_inc_eval(x) InterlockedIncrement64((volatile __int64 *)(x))
#  define ins_atomic_u64_dec_eval(x) InterlockedDecrement64((volatile __int64 *)(x))
#  define ins_atomic_u64_eval_assign(x,c) InterlockedExchange64((volatile __int64 *)(x),(c))
#  define ins_atomic_u64_add_eval(x,c) InterlockedAdd((volatile LONG *)(x), c)
#  define ins_atomic_u32_eval_assign(x,c) InterlockedExchange((volatile LONG *)(x),(c))
#  define ins_atomic_u32_eval_cond_assign(x,k,c) InterlockedCompareExchange((volatile LONG *)(x),(k),(c))
#  define ins_atomic_ptr_eval_assign(x,c) (void*)ins_atomic_u64_eval_assign((volatile __int64 *)(x), (__int64)(c))
# endif

#elif OS_LINUX

# if ARCH_X64
#  define ins_atomic_u64_inc_eval(x) __sync_fetch_and_add((volatile U64 *)(x), 1)
# endif

#else
// TODO(allen): 
#endif

////////////////////////////////
// NOTE(allen): Intrinsic Checks

#if ARCH_X64

# if !defined(ins_atomic_u64_inc_eval)
# error missing: ins_atomic_u64_inc_eval
# endif

#else
# error the intrinsic set for this arch is not developed
#endif


#endif //BASE_INS_H
