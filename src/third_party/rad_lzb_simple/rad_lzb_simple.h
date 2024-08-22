#ifndef _RAD_LZB_SIMPLE_H_
#define _RAD_LZB_SIMPLE_H_

/*======================================================

To encode :

	Set up an rr_lzb_simple_context
	
	fill out m_tableSizeBits (14-16 is typical)
	
	allocate m_hashTable

	rr_lzb_simple_context c;
	c.m_tableSizeBits = 14;
	c.m_hashTable = OODLE_MALLOC_ARRAY(U16,RR_ONE_SA<<c.m_tableSizeBits);
	
	then call _encode

NOTE :
	compressed & raw size are not included in the encoded bytes.  You must send
	them separately.
	
NOTE :
	lzb will never expand.  comp_len is <= raw_len strictly.
	if comp_len = raw_len it indicates that the compressed bytes are just a memcpy
	of the raw bytes.  In that case you do not need to decode.
	
To decode :

	if comp_len is == raw_len, then the compressed bytes are just a copy of the 
	raw bytes and you could use them directly without calling decode.
	
	if you call rr_lzb_simple_decode in that case, then the compressed buffer will
	be memcpy'd to the raw buffer

===============================================================*/

//~ TODO(rjf): temporary glue for building this without the shared rad code:

#define __RAD64REGS__

#include <stdint.h>
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t   S8;
typedef int16_t  S16;
typedef int32_t  S32;
typedef int64_t  S64;

typedef S64 SINTa;
typedef U64 RAD_U64;
typedef S64 RAD_S64;
typedef U32 RAD_U32;
typedef S32 RAD_S32;

#define RADINLINE __inline

#if defined(_MSC_VER)
# define RADFORCEINLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
# define RADFORCEINLINE __attribute__((always_inline))
#else
# error need force inline for this compiler
#endif

#if _MSC_VER
# define RADLZB_TRAP() __debugbreak()
#elif __clang__ || __GNUC__
# define RADLZB_TRAP() __builtin_trap()
#else
# error Unknown trap intrinsic for this compiler.
#endif

#define RR_STRING_JOIN(arg1, arg2)              RR_STRING_JOIN_DELAY(arg1, arg2)
#define RR_STRING_JOIN_DELAY(arg1, arg2)        RR_STRING_JOIN_IMMEDIATE(arg1, arg2)
#define RR_STRING_JOIN_IMMEDIATE(arg1, arg2)    arg1 ## arg2

#ifdef _MSC_VER
#define RR_NUMBERNAME(name) RR_STRING_JOIN(name,__COUNTER__)
#else
#define RR_NUMBERNAME(name) RR_STRING_JOIN(name,__LINE__)
#endif

#define RR_COMPILER_ASSERT(exp)   typedef char RR_NUMBERNAME(_dummy_array) [ (exp) ? 1 : -1 ]

#if defined(__clang__)
# define Expect(expr, val) __builtin_expect((expr), (val))
#else
# define Expect(expr, val) (expr)
#endif

#define RAD_LIKELY(expr)            Expect(expr,1)
#define RAD_UNLIKELY(expr)          Expect(expr,0)

#define __RADLITTLEENDIAN__ 1
#define RAD_PTRBYTES 8
#define RR_MIN(a,b)    ( (a) < (b) ? (a) : (b) )
#define RR_MAX(a,b)    ( (a) > (b) ? (a) : (b) )
#define RR_ASSERT_ALWAYS(c) do{if(!(c)) {RADLZB_TRAP();}}while(0)
#define RR_ASSERT(c) RR_ASSERT_ALWAYS(c)

#define RR_PUT16_LE(ptr,val)       *((U16 *)(ptr)) = (U16)(val)
#define RR_GET16_LE_UNALIGNED(ptr) *((const U16 *)(ptr))

static RADINLINE U32
rrCtzBytes32(U32 val)
{
  // Don't get fancy here. Assumes val != 0.
  if (val & 0x000000ffu) return 0;
  if (val & 0x0000ff00u) return 1;
  if (val & 0x00ff0000u) return 2;
  return 3;
}

static RADINLINE U32
rrCtzBytes64(U64 val)
{
  U32 lo = (U32) val;
  return lo ? rrCtzBytes32(lo) : 4 + rrCtzBytes32((U32) (val >> 32));
}

//~

//---------------------

typedef struct rr_lzb_simple_context rr_lzb_simple_context;
struct rr_lzb_simple_context
{
	U16	*	m_hashTable;	// must be allocated to sizeof(U16)*(1<<m_tableSizeBits)
	S32		m_tableSizeBits;
};

SINTa rr_lzb_simple_encode_fast(rr_lzb_simple_context * ctx,
                                const void * raw, SINTa rawLen, void * comp);

SINTa rr_lzb_simple_encode_veryfast(rr_lzb_simple_context * ctx,
                                    const void * raw, SINTa rawLen, void * comp);

//---------------------

// rr_lzb_simple_decode returns the number of compressed bytes consumed ( == compLen)
SINTa rr_lzb_simple_decode(const void * comp, SINTa compLen, void * raw, SINTa rawLen);

//---------------------

#endif // _RAD_LZB_SIMPLE_H_
