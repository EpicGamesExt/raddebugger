#include <string.h>

//-------------------------------------------------
// UINTr = int the size of a register

#ifdef __RAD64REGS__

#define RAD_UINTr RAD_U64
#define RAD_SINTr RAD_S64

#define readR read64
#define writeR write64

#define rrClzBytesR rrClzBytes64
#define rrCtzBytesR rrCtzBytes64

#else

#define RAD_UINTr RAD_U32
#define RAD_SINTr RAD_S32

#define readR read32
#define writeR write32

#define rrClzBytesR rrClzBytes32
#define rrCtzBytesR rrCtzBytes32

#endif

typedef RAD_SINTr SINTr;
typedef RAD_UINTr UINTr;

#define OOINLINE	RADFORCEINLINE

#define if_unlikely(exp)	if ( RAD_UNLIKELY( exp ) )
#define if_likely(  exp)	if ( RAD_LIKELY( exp ) )

// Raw byte IO

#if defined(__RADARM__) && !defined(__RAD64__) && defined(__GNUC__)

// older GCCs don't turn the memcpy variant into loads/stores, but
// they do support this:
typedef union
{
	U16 u16;
	U32 u32; 
	U64 u64; 
} __attribute__((packed)) unaligned_type;

static inline U16 read16(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u16; }
static inline void write16(void *ptr, U16 x) 	{ ((unaligned_type *)ptr)->u16 = x; }

static inline U32 read32(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u32; }
static inline void write32(void *ptr, U32 x) 	{ ((unaligned_type *)ptr)->u32 = x; }

static inline U64 read64(const void *ptr) 		{ return ((const unaligned_type *)ptr)->u64; }
static inline void write64(void *ptr, U64 x) 	{ ((unaligned_type *)ptr)->u64 = x; }

#else

// most C compilers we target are smart enough to turn this into single loads/stores
static inline U16 read16(const void *ptr) 		{ U16 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write16(void *ptr, U16 x) 	{ memcpy(ptr, &x, sizeof(x)); }

static inline U32 read32(const void *ptr) 		{ U32 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write32(void *ptr, U32 x) 	{ memcpy(ptr, &x, sizeof(x)); }

static inline U64 read64(const void *ptr) 		{ U64 x; memcpy(&x, ptr, sizeof(x)); return x; }
static inline void write64(void *ptr, U64 x) 	{ memcpy(ptr, &x, sizeof(x)); }

#endif

#define RR_PUT16_LE_UNALIGNED(ptr,val)                 RR_PUT16_LE(ptr,val)
#define RR_PUT16_LE_UNALIGNED_OFFSET(ptr,val,offset)   RR_PUT16_LE_OFFSET(ptr,val,offset)

//===========================================================================

static RADINLINE SINTa rrPtrDiffV(void * end, void *start) { return (SINTa)( ((char *)(end)) - ((char *)(start)) ); }

// helper function to show I really am intending to put a pointer difference in an int :
static RADINLINE SINTa rrPtrDiff(SINTa val) { return val; }
static RADINLINE S32 rrPtrDiff32(SINTa val) { S32 ret = (S32) val; RR_ASSERT( (SINTa)ret == val ); return ret; }
static RADINLINE SINTr rrPtrDiffR(SINTa val) { SINTr ret = (SINTr) val; RR_ASSERT( (SINTa)ret == val ); return ret; }

//=================================================================

#define LZB_LRL_BITS	4
#define LZB_LRL_ESCAPE	15

#define LZB_ML_BITS		4
#define LZB_MLCONTROL_ESCAPE	15

#define LZB_SLIDING_WINDOW_POW2	16
#define LZB_SLIDING_WINDOW_SIZE	(1<<LZB_SLIDING_WINDOW_POW2)

#define LZB_MAX_OFFSET		0xFFFF

#define LZB_MML		4		// should be 3 if I had LO

#define LZB_MATCHLEN_ESCAPE		(LZB_MLCONTROL_ESCAPE+4)


#define LZB_END_WITH_LITERALS			1	// @@ ??
//#define LZB_END_WITH_LITERALS			0	// @@ ??
#define LZB_END_OF_BLOCK_NO_MATCH_ZONE	8

/**

NOTE ABOUT LZB_END_OF_BLOCK_NO_MATCH_ZONE

The limitation in LZB does not actually come from the 8-at-a-time match copier

it comes from the unconditional 8-byte LRL copy

that means the last 8 bytes of every block *must* be literals

(note that's *block* not quantum)

The constraint due to matches is actually weaker
(match len rounded up to next multiple of 8 must not go past block end)

**/

// decode speed on lzt99 :
// LZ4 :      1715.10235

#define LZB_FORCELASTLRL9	1

//=======================================

#define lz_copywordstep(d,s)			do { writeR(d, readR(s)); (s) += sizeof(UINTr); (d) += sizeof(UINTr); } while(0)
#define lz_copywordsteptoend(d,s,e)		do { lz_copywordstep(d,s); } while ((d)<(e))

// lz_copysteptoend_overrunok
// NOTE : unlike memcpy, adjusts dest pointer to end !
#define lz_copysteptoend_overrunok(d,s,l)	do { U8 * e=(d)+(l); lz_copywordsteptoend(d,s,e); d=e; } while(0)

//=======================================

#define LZB_PutExcessBW(cp,val)	do { \
if ( val < 192 ) *cp++ = (U8) val; \
else { val -= 192; *cp++ = 192 + (U8) ( val&0x3F); val >>= 6; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; \
if ( val < 128 ) *cp++ = (U8) val; \
else { val -= 128; *cp++ = 128 + (U8) ( val&0x7F); val >>= 7; *cp++ = (U8) val; } } } } \
} while(0)

// max bytes consumed: 5
#define LZB_AddExcessBW(cp,val)	do { U32 b = *cp++; \
if ( b < 192 ) val += b; \
else { val += 192; val += (b-192); b = *cp++; \
val += (b<<6); if ( b >= 128 ) { b = *cp++; \
val += (b<<13); if ( b >= 128 ) { b = *cp++; \
val += (b<<20); if ( b >= 128 ) { b = *cp++; \
val += (b<<27); } } } } \
} while(0)

#define LZB_PutExcessLRL(cp,val) LZB_PutExcessBW(cp,val)
#define LZB_PutExcessML(cp,val)  LZB_PutExcessBW(cp,val)

#define LZB_AddExcessLRL(cp,val) LZB_AddExcessBW(cp,val)
#define LZB_AddExcessML(cp,val)  LZB_AddExcessBW(cp,val)

//=============================================================================
// match copies :

// used for LRL :
static OOINLINE void copy_no_overlap_long(U8 * to, const U8 * from, SINTr length)
{
	for(int i=0;i<length;i+=8)
		write64(to+i, read64(from+i));
}

static OOINLINE void copy_no_overlap_nooverrun(U8 * to, const U8 * from, SINTr length)
{
	// used for final LRL of every block
	//  must not overrun
	memmove(to,from,(size_t)length);
}

RR_COMPILER_ASSERT( LZB_MLCONTROL_ESCAPE == 15 );
RR_COMPILER_ASSERT( LZB_MATCHLEN_ESCAPE == 19 );

static OOINLINE void copy_match_short_overlap(U8 * to, const U8 * from, SINTr ml)
{
	RR_ASSERT( ml >= LZB_MML && ml < LZB_MATCHLEN_ESCAPE );
  
	// overlap
	// @@ err not awesome
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
	to[3] = from[3];
	to[4] = from[4];
	to[5] = from[5];
	to[6] = from[6];
	to[7] = from[7];
	if ( ml > 8 )
	{
		to += 8; from += 8; ml -= 8;
		// max of 10 more
		while(ml--)
		{
			*to++ = *from++;
		}
	}
}

static OOINLINE void copy_match_memset(U8 * to, int c, SINTr ml)
{
	RR_ASSERT( ml >= 4 );
	U32 four = c * 0x01010101;
	U8 * end = to + ml;
	write32(to, four); to += 4;
	while(to<end)
	{
		write32(to, four); to += 4;
	}
}

//=============================================================================

static SINTa rr_lzb_simple_decode_notexpanded(const void * comp, void * raw, SINTa rawLen)
{
	U8 * rp = (U8 *)raw;
	U8 * rpEnd = rp+rawLen;
  
	const U8 *	cp = (const U8 *)comp;
	
	for(;;)
	{
		RR_ASSERT( rp < rpEnd );
    
		// max bytes consumed (fast paths):
		// - 1 control
		// - lits:
		//   * 15 lits OR
		//   * 5 excess lrl + long lit run
		// - match:
		//   * 2 match offset (short match) OR
		//   * 1 excess code + 5 excess ML (overlap match) OR
		//   * 1 excess code + 5 excess ML (long match)
		//
		// need near-end checks mainly on long lit runs.
    
		UINTr control = *cp++;
    
		UINTr lrl = control & 0xF;
		UINTr ml_control = (control>>4);
    
		// copy 4 literals speculatively :
		write32( rp , read32(cp) );
    
		//RR_ASSERT( lrl >= 8 || ml_control >= 8 );
    
		if ( lrl > 4 )
		{
			// if lrl was <= 8 we did it, else need this :
			if_unlikely ( lrl > 8 )
			{
				if_unlikely ( lrl >= LZB_LRL_ESCAPE )
				{
					LZB_AddExcessLRL( cp, lrl );
          
					// hide the EOF check here ?
					// has to be after the GetExcess
					if_unlikely ( rp+lrl >= rpEnd )
					{	
						RR_ASSERT( rp+lrl == rpEnd );
            
						copy_no_overlap_nooverrun(rp,cp,lrl);
            
						rp += lrl;
						cp += lrl;
						break;
					}
					else
					{
						// total undo of the previous copy	
						copy_no_overlap_long(rp,cp,lrl);
					}
				}
				else // > 8 but not 0xF
				{
					// hide the EOF check here ?
					if_unlikely ( rp+lrl >= rpEnd )
					{	
						if ( lrl == 9 )
						{
							// may be a false 9
							lrl = rrPtrDiff32( rpEnd - rp );
						}
						RR_ASSERT( rp+lrl == rpEnd );
            
						copy_no_overlap_nooverrun(rp,cp,lrl);
            
						rp += lrl;
						cp += lrl;
						break;						
					}
					else
					{
						write32( rp+4 , read32(cp+4) );
						// put 8 more :
						write64( (rp+8) , read64((cp+8)) );
					}
				}
			}
			else
			{
				write32( rp+4 , read32(cp+4) );
			}
		}
    
		rp += lrl;
		cp += lrl;
    
		RR_ASSERT( rp+LZB_MML <= rpEnd );
    
		UINTr ml = ml_control + LZB_MML;
    
		// speculatively grab offset but don't advance cp yet
		UINTr off = RR_GET16_LE_UNALIGNED(cp);
    
		if ( ml_control <= 8 )
		{
			cp += 2; // consume offset
			const U8 * match = rp - off;
      
			RR_ASSERT( ml <= 12 );
      
			write64( rp , read64(match) );
			write32( rp+8 , read32(match+8) );
      
			rp += ml;
			continue;
		}
		else
		{
      
			if_likely( ml_control < LZB_MLCONTROL_ESCAPE ) // short match
			{
				cp += 2; // consume offset
				const U8 * match = rp - off;
        
				RR_ASSERT( off >= 8 || ml <= off );
        
				write64( rp , read64(match) );
				write64( rp+8 , read64(match+8) );
        
				if ( ml > 16 )
				{
					write16( rp+16, read16(match+16) );
				}
			}
			else
			{
				// get 1-byte excess code
				UINTr excesslow = off&127;
				cp++; // consume 1
        
				//if ( excess1 >= 128 )
				if ( off & 128 )
				{				
					ml_control = excesslow >> 3;
					ml = ml_control + LZB_MML;
					if ( ml_control == 0xF )
					{
						// get more ml
						LZB_AddExcessML( cp, ml );
					}	
          
					UINTr myoff = off & 7;
          
					// low offset, can't do 8-byte grabs
					if ( myoff == 1 )
					{
						int c = rp[-1];
						copy_match_memset(rp,c,ml);
					}
					else
					{
						// shit but whatever, very rare
						for(UINTr i=0;i<ml;i++)
						{
							rp[i] = rp[i-myoff];
						}
					}
				}
				else
				{
					UINTr myoff = RR_GET16_LE_UNALIGNED(cp); cp += 2;
					const U8 * match = rp - myoff;
          
					ml += excesslow;
          
					if ( excesslow == 127 )
					{
						// get more ml
						LZB_AddExcessML( cp, ml );
					}
          
					// 8-byte copier :
					copy_no_overlap_long(rp,match,ml);
				}
			}
      
			rp += ml;
		}
	}
  
	RR_ASSERT( rp == rpEnd );
  
	SINTa used = rrPtrDiff( cp - (const U8 *)comp );
	
	RR_ASSERT( used < rawLen );
	
	return used;
}

SINTa rr_lzb_simple_decode(const void * comp, SINTa compLen, void * raw, SINTa rawLen)
{
	RR_ASSERT_ALWAYS( compLen <= rawLen );
	if ( compLen == rawLen )
	{
		memcpy(raw,comp,rawLen);
		return compLen;
	}
	return rr_lzb_simple_decode_notexpanded(comp,raw,rawLen);
}

//=====================================================


static RADINLINE U32 hmf_hash4_32(U32 ptr32)
{
  U32 h = ( ptr32 * 2654435761u );
  h ^= (h>>13);
  return h;
}

#define HashMatchFinder_Hash32	hmf_hash4_32

//=================================================================================

#define LZB_Hash4	hmf_hash4_32

static RADINLINE U32 LZB_SecondHash4(U32 be4)
{
	const U32 m = 0x5bd1e995;
  
	U32 h = be4 * m;
	h += (h>>11);
	
	return h;
}

//=============================================    

static int RADFORCEINLINE GetNumBytesZeroNeverAllR(UINTr x)
{
	RR_ASSERT( x != 0 );
  
#if defined(__RADBIGENDIAN__)
	// big endian, so earlier bytes are at the top
	int nb = (int)rrClzBytesR(x);
#elif defined(__RADLITTLEENDIAN__)
	// little endian, so earlier bytes are at the bottom
	int nb = (int)rrCtzBytesR(x);
#else
#error wtf no endian set
#endif
  
	RR_ASSERT( nb >= 0 && nb < (int)sizeof(UINTr) );
	return nb;
}

//===============================

static RADFORCEINLINE U8 * LZB_Output(U8 * cp, S32 lrl, const U8 * literals,  S32 matchlen ,  S32 mo )
{
	RR_ASSERT( lrl >= 0 );
	RR_ASSERT( matchlen >= LZB_MML );
	RR_ASSERT( mo > 0 && mo <= LZB_MAX_OFFSET );
	
	//rrprintf("[%3d][%3d][%7d]\n",lrl,ml,mo);
  
	S32 sendml = matchlen - LZB_MML;
	
	U32 ml_in_control  = RR_MIN(sendml,LZB_MLCONTROL_ESCAPE);
	
	if ( mo >= 8 ) // no overlap	
	{
		if ( lrl < LZB_LRL_ESCAPE )
		{
			U32 control = lrl | (ml_in_control<<4);
      
			*cp++ = (U8) control;
			
			write64(cp, read64(literals));
			if ( lrl > 8 )
			{
				write64(cp+8, read64(literals+8));
			}
			cp += lrl;
		}
		else
		{
			U32 control = LZB_LRL_ESCAPE | (ml_in_control<<4);
      
			*cp++ = (U8) control;
			
			U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
			LZB_PutExcessLRL(cp,lrl_excess);
      
			// @@ ? is this okay for overrun ?
			lz_copysteptoend_overrunok(cp,literals,lrl);
		}
		
		if ( ml_in_control < LZB_MLCONTROL_ESCAPE )
		{
			RR_ASSERT( (U16)(mo) == mo );
			RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
			cp += 2;
		}
		else
		{
			U32 ml_excess = sendml - LZB_MLCONTROL_ESCAPE;
			
			// put special first byte, then offset, then remainder
			if ( ml_excess < 127 )
			{
				*cp++ = (U8)ml_excess;
        
				RR_ASSERT( (U16)(mo) == mo );
				RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
				cp += 2;
			}
			else
			{
				*cp++ = (U8)127;
        
				RR_ASSERT( (U16)(mo) == mo );
				RR_PUT16_LE_UNALIGNED(cp,(U16)(mo));
				cp += 2;
        
				ml_excess -= 127;
				LZB_PutExcessML(cp,ml_excess);
			}
		}
	}
	else
	{
		U32 lrl_in_control = RR_MIN(lrl,LZB_LRL_ESCAPE);
    
    // overlap case
		U32 control = (lrl_in_control) | (LZB_MLCONTROL_ESCAPE<<4);
		
		*cp++ = (U8) control;
		
		if ( lrl_in_control == LZB_LRL_ESCAPE )
		{
			U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
			LZB_PutExcessLRL(cp,lrl_excess);
		}
		
		lz_copysteptoend_overrunok(cp,literals,lrl);
		//cp += lrl;
		
		// special excess1 :
		UINTr excess1 = 128 + (ml_in_control<<3) + mo;
		RR_ASSERT( excess1 < 256 );
		
		*cp++ = (U8)excess1;
		
		if ( ml_in_control == LZB_MLCONTROL_ESCAPE )
		{
			U32 ml_excess = sendml - LZB_MLCONTROL_ESCAPE;
			LZB_PutExcessML(cp,ml_excess);
		}		
	}
	
	return cp;
}

#if LZB_FORCELASTLRL9

static RADINLINE U8 * LZB_OutputLast(U8 * cp, S32 lrl, const U8 * literals )
{
	RR_ASSERT( lrl >= 0 );
	
	//U32 ml = 0;
	//U32 mo = 0;
  
	U32 lrl_in_control = RR_MIN(lrl,LZB_LRL_ESCAPE);
	
#if LZB_END_WITH_LITERALS
	// lrl_in_control must be at least 9
	lrl_in_control = RR_MAX(lrl_in_control,9);
#endif
	
	U32 control = lrl_in_control;
  
	*cp++ = (U8) control;
	
	if ( lrl_in_control == LZB_LRL_ESCAPE )
	{
		U32 lrl_excess = lrl - LZB_LRL_ESCAPE;
		LZB_PutExcessLRL(cp,lrl_excess);
	}
	
	memmove(cp,literals,lrl);
	cp += lrl;
	
	return cp;
}

#else

static RADINLINE U8 * LZB_OutputLast(U8 * cp, S32 lrl, const U8 * literals )
{
	cp = LZB_Output(cp,lrl,literals,LZB_MML,1);
	
	// remove the offset we put :
	cp -= 2;
	
	return cp;
}

#endif

//===============================================================

static void rr_lzb_simple_context_init(rr_lzb_simple_context * ctx) //, const void * base)
{
	RR_ASSERT( ctx->m_tableSizeBits >= 12 && ctx->m_tableSizeBits <= 24 );
	memset(ctx->m_hashTable,0,sizeof(U16)*((SINTa)1<<ctx->m_tableSizeBits));
}

//===============================================================

/*     
#define FAST_HASH_DEPTH_SHIFT   (1) // more depth = more & more compression,
#define DO_FAST_2ND_HASH    //  rate= 30.69 mb/s , 15451369 <- turning this off is the best way to get more speed and less compression
/*/
#define FAST_HASH_DEPTH_SHIFT   (0)
#define DO_FAST_2ND_HASH
/**/

//     lzt99,  24700820,  15475520,  16677179
//encode only      : 0.880 seconds, 1.62 b/hc, rate= 28.08 mb/s

//#define FAST_HASH_DEPTH_SHIFT   (1) // more depth = more & more compression, but slower

#define DO_FAST_UPDATE_MATCH_HASHES 1 // helps compression a lot , like 0.30
//#define DO_FAST_UPDATE_MATCH_HASHES 2 // helps compression a lot , like 0.30
#define DO_FAST_LAZY_MATCH  // also helps a lot , like 0.15
#define DO_FAST_HASH_DWORD		1

#define FAST_MULTISTEP_LITERALS_SHIFT	(5)


//-----------------------
// derived :

/*
#define FAST_HASH_BITS          (FAST_HASH_TOTAL_BITS-FAST_HASH_DEPTH_SHIFT)
#define FAST_HASH_SIZE          (1<<FAST_HASH_BITS)
#define FAST_HASH_MASK          (FAST_HASH_SIZE-1)
*/

#undef FAST_HASH_DEPTH
#define FAST_HASH_DEPTH         (1<<FAST_HASH_DEPTH_SHIFT)

/*
#if FAST_HASH_DEPTH == 1
#error nope
#endif
*/

#undef FAST_HASH_CYCLE_MASK
#define FAST_HASH_CYCLE_MASK    (FAST_HASH_DEPTH-1)

#undef FAST_HASH_INDEX
#if FAST_HASH_DEPTH > 1
#define FAST_HASH_INDEX(h,d)    ( ((h)<<FAST_HASH_DEPTH_SHIFT) + (d) )
#else
#define FAST_HASH_INDEX(h,d)    (h)
#endif

#undef FAST_HASH_FUNC
#define FAST_HASH_FUNC(ptr,dword)	( LZB_Hash4(dword) & hash_table_mask )



static SINTa rr_lzb_simple_encode_fast_sub(rr_lzb_simple_context * fh,
                                           const void * raw, SINTa rawLen, void * comp)
{
	//SIMPLEPROFILE_SCOPE_N(lzbfast_sub,rawLen);
	//THREADPROFILEFUNC();
	
	U8 * cp = (U8 *)comp;
	U8 * compExpandedPtr = cp + rawLen - 8;
  
	const U8 * rp = (const U8 *)raw;
	const U8 * rpEnd = rp+rawLen;
  
	const U8 * rpMatchEnd = rpEnd - LZB_END_OF_BLOCK_NO_MATCH_ZONE;
	
	const U8 * rpEndSafe = rpMatchEnd - LZB_MML;
	
	if ( rpEndSafe <= (U8 *)raw )
	{
		// can't compress
		return rawLen+1;
	}
	
	const U8 * literals_start = rp;
  
#if FAST_HASH_DEPTH > 1
	int hashCycle = 0;
#endif
  
	U16 * hashTable16 = fh->m_hashTable;
	
	int hashTableSizeBits = fh->m_tableSizeBits;
	U32 hash_table_mask = (U32)((1UL<<(hashTableSizeBits - FAST_HASH_DEPTH_SHIFT)) - 1);
	
	const U8 * zeroPosPtr = (const U8 *)raw;
  
	// first byte is always a literal
	rp++;
	
	for(;;)
	{	
		S32 matchOff;
    
		UINTr failedMatches = (1<<FAST_MULTISTEP_LITERALS_SHIFT) + 3;
		
		U32 rp32 = read32(rp);
		U32 hash = FAST_HASH_FUNC(rp, rp32 );
		SINTa curpos;
		const U8 * hashrp;
    
#ifdef DO_FAST_2ND_HASH
		U32 hash2;
#endif
    
		// literals :
		for(;;)		
		{    				
			curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
#ifdef DO_FAST_2ND_HASH
			hash2 = ( LZB_SecondHash4(rp32) ) & hash_table_mask;
#endif
      
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(hash,d) ];
				
				matchOff = (U16)(curpos - hashpos16);
				RR_ASSERT( matchOff >= 0 );
				
				hashrp = rp - matchOff;
        
				//if ( matchOff <= LZB_MAX_OFFSET )
				RR_ASSERT( matchOff <= LZB_MAX_OFFSET );
				{							
					const U32 hashrp32 = read32(hashrp);
          
					if ( rp32 == hashrp32 && matchOff != 0 )
					{
						goto found_match;
					}
				}
			}
      
#ifdef DO_FAST_2ND_HASH
      
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(hash2,d) ];
				
				matchOff = (U16)(curpos - hashpos16);
				RR_ASSERT( matchOff >= 0 );
				
				hashrp = rp - matchOff;
        
				RR_ASSERT( matchOff <= LZB_MAX_OFFSET );
				{							
					const U32 hashrp32 = read32(hashrp);
          
					if ( rp32 == hashrp32 && matchOff != 0 )
					{
						goto found_match;
					}
				}
			} 
			
#endif
      
			//---------------------------
			// update hash :
      
			hashTable16[ FAST_HASH_INDEX(hash,hashCycle) ] = (U16) curpos;
      
#ifdef DO_FAST_2ND_HASH
			// do NOT step hashCycle !
			//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
			hashTable16[ FAST_HASH_INDEX(hash2,hashCycle) ] = (U16) curpos;
#endif
			
#if FAST_HASH_DEPTH > 1
			hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
      
			UINTr stepLiterals = (failedMatches>>FAST_MULTISTEP_LITERALS_SHIFT);
			RR_ASSERT( stepLiterals >= 1 );
      
			++failedMatches;
      
			rp += stepLiterals;
      
			if ( rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
      
		}
		
		//-------------------------------
		found_match:
    
		// found something
    
    //-------------------------
    // update hash now so lazy can see it :
    
#if 1 // pretty important to compression
		hashTable16[ FAST_HASH_INDEX(hash,hashCycle) ] = (U16) curpos;
    
#ifdef DO_FAST_2ND_HASH
		// do NOT step hashCycle !
		//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
		hashTable16[ FAST_HASH_INDEX(hash2,hashCycle) ] = (U16) curpos;
#endif
		
#if FAST_HASH_DEPTH > 1
		hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
#endif
		
		//-----------------------------------
		
		const U8 * match_start = rp;
		rp += 4;
    
		while( rp < rpEndSafe )
		{
			UINTr big1 = readR(rp);
			UINTr big2 = readR(rp-matchOff);
	    
			if ( big1 == big2 )
			{
				rp += RAD_PTRBYTES;
				continue;
			}
			else
			{
				rp += GetNumBytesZeroNeverAllR(big1^big2);  
				break;
			}
		}
		rp = RR_MIN(rp,rpMatchEnd);
    
		//-------------------------------
    // rp is now at the *end* of the match
    
		//-------------------------------
		
		// check lazy match too
#ifdef DO_FAST_LAZY_MATCH
		if (rp< rpEndSafe)
		{
			const U8 * lazyrp = match_start + 1;
			//SINTa lazypos = rrPtrDiff(lazyrp - zeroPosPtr);
			SINTa lazypos = curpos + 1;
			RR_ASSERT( lazypos == rrPtrDiff(lazyrp - zeroPosPtr) );
      
			U32 lazyrp32 = read32(lazyrp);
      
			const U8 * lazyhashrp;	
			SINTa lazymatchOff;					
			
			U32 lazyHash = FAST_HASH_FUNC(lazyrp, lazyrp32 );
			
#ifdef DO_FAST_2ND_HASH
			U32 lazyhash2 = LZB_SecondHash4(lazyrp32) & hash_table_mask;
#endif
			
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{			
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(lazyHash,d) ];
				
				lazymatchOff = (U16)(lazypos - hashpos16);
				RR_ASSERT( lazymatchOff >= 0 );
				
				RR_ASSERT( lazymatchOff <= LZB_MAX_OFFSET );
				{
					lazyhashrp = lazyrp - lazymatchOff;
          
					const U32 hashrp32 = read32(lazyhashrp);
          
					if ( lazyrp32 == hashrp32 && lazymatchOff != 0 )
					{
						goto lazy_found_match;
					}
				}
			}
      
#ifdef DO_FAST_2ND_HASH
#if FAST_HASH_DEPTH > 1
			for(int d=0;d<FAST_HASH_DEPTH;d++)
#endif
			{
				U16 hashpos16 = hashTable16[ FAST_HASH_INDEX(lazyhash2,d) ];
				
				lazymatchOff = (U16)(lazypos - hashpos16);
				RR_ASSERT( lazymatchOff >= 0 );
				
				RR_ASSERT( lazymatchOff <= LZB_MAX_OFFSET );
				{
					lazyhashrp = lazyrp - lazymatchOff;
          
					const U32 hashrp32 = read32(lazyhashrp);
          
					if ( lazyrp32 == hashrp32 && lazymatchOff != 0 )
					{
						goto lazy_found_match;
					}
				}
			}  
#endif
			
			if ( 0 )
			{
				lazy_found_match:
        
				lazyrp += 4;
        
				while( lazyrp < rpEndSafe )
				{
					UINTr big1 = readR(lazyrp);
					UINTr big2 = readR(lazyrp-lazymatchOff);
			    
					if ( big1 == big2 )
					{
						lazyrp += RAD_PTRBYTES;
						continue;
					}
					else
					{
						lazyrp += GetNumBytesZeroNeverAllR(big1^big2);  
						break;
					}
				}
				lazyrp = RR_MIN(lazyrp,rpMatchEnd);
				
				//S32 lazymatchLen = rrPtrDiff32( lazyrp - (match_start+1) );
				//RR_ASSERT( lazymatchLen >= 4 );
        
				if ( lazyrp >= rp+3 )
				{
					// yes take the lazy match
					
					// put a literal :
					match_start++;
          
					// I had a bug where lazypos was set wrong for the hash fill
					// it set it to the *end* of the normal match
					// and for some reason that helped compression WTF WTF						              
					//SINTa lazypos = rrPtrDiff(rp - zeroPosPtr); // 233647528
					// with correct lazypos : 233651228	
					
					// really this shouldn't be necessary at all
					// because I do an update of hash at all positions in the match including first!
#if 1	 // with update disabled - 233690274			    
          
					hashTable16[ FAST_HASH_INDEX(lazyHash,hashCycle) ] = (U16) lazypos;
          
#ifdef DO_FAST_2ND_HASH
					// do NOT step hashCycle !
					hashTable16[ FAST_HASH_INDEX(lazyhash2,hashCycle) ] = (U16) lazypos;
#endif
					
#if FAST_HASH_DEPTH > 1
					hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
#endif
					
#endif
					
					// and then drop out and do the lazy match :
					//matchLen = lazymatchLen;
					matchOff = (S32)lazymatchOff;
					rp = lazyrp;
					hashrp = lazyhashrp;
				}	
			}
		}
#endif			  
		
		//---------------------------------------------------
    
		// back up start of match that we missed due to stepLiterals !
		// make sure we don't read off the start of the array
		
		// this costs a little speed and gains a little compression
		// 15662162 at 121.58 mb/s
		// 15776473 at 127.92 mb/s
#if 1
		/*
		lzbf : 24,700,820 ->15,963,503 =  5.170 bpb =  1.547 to 1
		encode           : 0.171 seconds, 83.60 b/kc, rate= 144.54 M/s
		decode           : 0.014 seconds, 1002.64 b/kc, rate= 1733.57 M/s
		*/
		{
			// 144 M/s
			// back up start of match that we missed
			// make sure we don't read off the start of the array
			
			const U8 * rpm1 = match_start-1;
			if ( rpm1 >= literals_start && hashrp > zeroPosPtr && rpm1[0] == hashrp[-1] )
			{
				rpm1--; hashrp-= 2;
				
				while ( rpm1 >= literals_start && hashrp >= zeroPosPtr && rpm1[0] == *hashrp )
				{
					rpm1--;
					hashrp--;
				}
				
				match_start = rpm1+1;
				//rp = RR_MAX(rp,literals_start);
				RR_ASSERT( match_start >= literals_start );
			}
		}
#endif
		
		S32 matchLen = rrPtrDiff32( rp - match_start );
		RR_ASSERT( matchLen >= 4 );
    
		//===============================================
		// chose a match
		//	output LRL (if any) and match
		
		S32 cur_lrl = rrPtrDiff32(match_start - literals_start);
    
		// catch expansion while writing :
		if_unlikely ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
    
		cp = LZB_Output(cp,cur_lrl,literals_start,matchLen,matchOff);
    
		// skip the match :
		literals_start = rp;		
		
		if ( rp >= rpEndSafe )
			break;
		
		// step & update hashes :
		//  (I already did cur pos)
#ifdef DO_FAST_UPDATE_MATCH_HASHES
		// don't bother if it takes us to the end :      
		//	(this check is not for speed it's to avoid the access violation)          
		const U8 * ptr = match_start+1;
		U16 pos16 = (U16) rrPtrDiff( ptr - zeroPosPtr );
		for(;ptr<rp;ptr++)
		{
			U32 hash_result = FAST_HASH_FUNC( ptr, read32(ptr) );
			hashTable16[ FAST_HASH_INDEX(hash_result,hashCycle) ] = pos16; pos16++;
			//hashCycle = (hashCycle+1)&FAST_HASH_CYCLE_MASK;
			// helps a bit to NOT step cycle here
			//  the hash entries that come inside a match are of much lower quality
		}
#endif
	}
  
	done:;
	
	int cur_lrl = rrPtrDiff32(rpEnd - literals_start);
#if LZB_END_WITH_LITERALS
	RR_ASSERT_ALWAYS(cur_lrl > 0 );
#endif
  
	if ( cur_lrl > 0 )
	{
		// catch expansion while writing :
		if ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		cp = LZB_OutputLast(cp,cur_lrl,literals_start);
	}
  
	SINTa compLen = rrPtrDiff( cp - (U8 *)comp );
  
	return compLen;
}

SINTa rr_lzb_simple_encode_fast(rr_lzb_simple_context * fh,
                                const void * raw, SINTa rawLen, void * comp)
{
	rr_lzb_simple_context_init(fh); //,raw);
  
	SINTa comp_len = rr_lzb_simple_encode_fast_sub(fh,raw,rawLen,comp);
	if ( comp_len >= rawLen )
	{
		memcpy(comp,raw,rawLen);
		return rawLen;
	}
	return comp_len;
}

#undef FAST_HASH_DEPTH_SHIFT

#undef DO_FAST_UPDATE_MATCH_HASHES
#undef DO_FAST_LAZY_MATCH
#undef DO_FAST_2ND_HASH  

//=====================================================

#define FAST_HASH_DEPTH_SHIFT	(0)

#undef FAST_MULTISTEP_LITERALS_SHIFT
#define FAST_MULTISTEP_LITERALS_SHIFT	(4)



//-----------------------
// derived :

RR_COMPILER_ASSERT( FAST_HASH_DEPTH_SHIFT == 0 );

#undef FAST_HASH_FUNC
//#define FAST_HASH_FUNC(ptr,dword)	( LZB_Hash4(dword) & hash_table_mask )
#define FAST_HASH_FUNC(ptr,dword)	( (((dword)*2654435761U)>>16) & hash_table_mask )


// @@@@ ????
#define LZBVF_DO_BACKUP	0
//#define LZBVF_DO_BACKUP	1


static SINTa rr_lzb_simple_encode_veryfast_sub(rr_lzb_simple_context * fh,
                                               const void * raw, SINTa rawLen, void * comp)
{
	//SIMPLEPROFILE_SCOPE_N(lzbfast_sub,rawLen);
	//THREADPROFILEFUNC();
	
	U8 * cp = (U8 *)comp;
	U8 * compExpandedPtr = cp + rawLen - 8;
  
	const U8 * rp = (const U8 *)raw;
	const U8 * rpEnd = rp+rawLen;
  
	// we can match up to rpEnd
	//	but matches can't start past rpEndSafe
	const U8 * rpMatchEnd = rpEnd - LZB_END_OF_BLOCK_NO_MATCH_ZONE;
	
	const U8 * rpEndSafe = rpMatchEnd - LZB_MML;
	
	if ( rpEndSafe <= (U8 *)raw )
	{
		// can't compress
		return rawLen+1;
	}
	
	const U8 * literals_start = rp;
  
	U16 * hashTable16 = fh->m_hashTable;
	int hashTableSizeBits = fh->m_tableSizeBits;
	U32 hash_table_mask = (U32)((1UL<<(hashTableSizeBits)) - 1);
  
	const U8 * zeroPosPtr = (const U8 *)raw;
  
	// first byte is always a literal
	rp++;
	
	for(;;)
	{   		
		U32 rp32 = read32(rp);
		U32 hash = FAST_HASH_FUNC(rp, rp32 );
		const U8 * hashrp;
		S32 matchOff;
		UINTr failedMatches;
    
		// loop while no match found :
		
		// first loop with step = 1
		// @@
		//int step1count = (1<<FAST_MULTISTEP_LITERALS_SHIFT); // full count
		int step1count = (1<<(FAST_MULTISTEP_LITERALS_SHIFT-1)); // half count
		while(step1count--)
		{			    					
			SINTa curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
			U16 hashpos16 = hashTable16[hash];
			hashTable16[ hash ] = (U16) curpos;
			
			matchOff = (U16)(curpos - hashpos16);
			RR_ASSERT( matchOff >= 0 && matchOff <= LZB_MAX_OFFSET );
			hashrp = rp - matchOff;
      
			const U32 hashrp32 = read32(hashrp);
			if ( rp32 == hashrp32 && matchOff != 0 )
			{
				goto found_match;
			}
      
			if ( ++rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
		}
		
		// step starts at 2 :
		failedMatches = (2<<FAST_MULTISTEP_LITERALS_SHIFT);
    
		for(;;)		
		{			    		
			SINTa curpos = rrPtrDiff(rp - zeroPosPtr);	
			RR_ASSERT( curpos >= 0 );
			
			U16 hashpos16 = hashTable16[hash];
			hashTable16[ hash ] = (U16) curpos;
      
			matchOff = (U16)(curpos - hashpos16);
			RR_ASSERT( matchOff >= 0 && matchOff <= LZB_MAX_OFFSET );
			hashrp = rp - matchOff;
      
			const U32 hashrp32 = read32(hashrp);
      
			if ( rp32 == hashrp32 && matchOff != 0 )
			{
				goto found_match;
			}
      
			UINTr stepLiterals = (failedMatches>>FAST_MULTISTEP_LITERALS_SHIFT);
			RR_ASSERT( stepLiterals >= 1 );
      
			++failedMatches;
      
			rp += stepLiterals;
      
			if ( rp >= rpEndSafe )
				goto done;
      
			rp32 = read32(rp);
			hash = FAST_HASH_FUNC(rp, rp32 );
		}
		
		//-------------------------------
		found_match:;
    
		// found something
    
#if LZBVF_DO_BACKUP
		
		// alternative backup using counter :
		S32 cur_lrl = rrPtrDiff32(rp - literals_start);
		int neg_max_backup = - RR_MIN(cur_lrl , rrPtrDiff32(hashrp - zeroPosPtr) );
		int neg_backup = -1;
		if( neg_backup >= neg_max_backup && rp[neg_backup] == hashrp[neg_backup] )
		{
			neg_backup--;
			while( neg_backup >= neg_max_backup && rp[neg_backup] == hashrp[neg_backup] )
			{
				neg_backup--;
			}
			neg_backup++;
			rp += neg_backup;
			cur_lrl += neg_backup;
			RR_ASSERT( cur_lrl >= 0 );
			RR_ASSERT( cur_lrl == rrPtrDiff32(rp - literals_start) );
		}
		
#else
		
		S32 cur_lrl = rrPtrDiff32(rp - literals_start);
		
#endif
    
		// catch expansion while writing :
		if_unlikely ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		RR_ASSERT( matchOff >= 1 );
    
		//---------------------------------------
		// find rest of match len
		// save pointer to start of match
		// walk rp ahead to end of match
		const U8 * match_start = rp;
		rp += 4;
    
		while( rp < rpEndSafe )
		{
			UINTr big1 = readR(rp);
			UINTr big2 = readR(rp-matchOff);
	    
			if ( big1 == big2 )
			{
				rp += RAD_PTRBYTES;
				continue;
			}
			else
			{
				rp += GetNumBytesZeroNeverAllR(big1^big2);  
				break;
			}
		}
		rp = RR_MIN(rp,rpMatchEnd);
		S32 matchLen = rrPtrDiff32( rp - match_start );
		
		//===============================================
		// chose a match
		//	output LRL (if any) and match
		
		cp = LZB_Output(cp,cur_lrl,literals_start,matchLen,matchOff);
    
		// skip the match :
		literals_start = rp;
		
		if ( rp >= rpEndSafe )
			goto done;	
	}
	
	done:;
	
	int cur_lrl = rrPtrDiff32(rpEnd - literals_start);
#if LZB_END_WITH_LITERALS
	RR_ASSERT_ALWAYS(cur_lrl > 0 );
#endif
  
	if ( cur_lrl > 0 )
	{
		// catch expansion while writing :
		if ( cp+cur_lrl >= compExpandedPtr )
		{
			return rawLen+1;
		}
		
		cp = LZB_OutputLast(cp,cur_lrl,literals_start);
	}
  
	SINTa compLen = rrPtrDiff( cp - (U8 *)comp );
  
	return compLen;
}

SINTa rr_lzb_simple_encode_veryfast(rr_lzb_simple_context * fh,
                                    const void * raw, SINTa rawLen, void * comp)
{
	rr_lzb_simple_context_init(fh); //,raw);
	
	SINTa comp_len = rr_lzb_simple_encode_veryfast_sub(fh,raw,rawLen,comp);
	if ( comp_len >= rawLen )
	{
		memcpy(comp,raw,rawLen);
		return rawLen;
	}
	return comp_len;
}

#undef FAST_HASH_DEPTH_SHIFT

#undef DO_FAST_UPDATE_MATCH_HASHES
#undef DO_FAST_LAZY_MATCH
#undef DO_FAST_2ND_HASH  

//=====================================================
// vim:noet:sw=4:ts=4
