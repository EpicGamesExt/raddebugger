#ifndef _UTILS_H_
#define _UTILS_H_

S32 HighestSetBit(U32, U32);
S32 LowestSetBit(U32 number, U32 n);
S32 BitCount(U32, U32);
U64 Ones(S32 len, S32 N);
S32 DecodeBitMasks(U32 N, U32 imms, U32 immr, S32 immediate, U64 *out);
U64 replicate(U64, S32, S32);
S32 MoveWidePreferred(U32 sf, U32 immN, U32 immr, U32 imms);
S32 IsZero(U64 x);
S32 IsOnes(U64 x, S32 n);
S32 BFXPreferred(U32 sf, U32 uns, U32 imms, U32 immr);
char *decode_reg_extend(U32 op);
const char *decode_cond(U32 cond);
const char *get_arrangement(U32 size, U32 Q);

#endif
