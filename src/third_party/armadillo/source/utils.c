#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

/* Thanks https://github.com/xerub/macho/blob/master/patchfinder64.c */
static U64 RORZeroExtendOnes(U32 M, U32 N,
        U32 R){
    U64 val = Ones(M, N);

    if(R == 0)
        return val;

    return ((val >> R) & (((U64)1 << (N - R)) - 1)) |
        ((val & (((U64)1 << R) - 1)) << (N - R));
}

S32 HighestSetBit(U32 number, U32 n){
    S32 ret = -1;

    for(S32 i = n-1; i>=0; i--){
        if(number & (1 << i))
            return i;
    }

    return ret;
}

S32 LowestSetBit(U32 number, U32 n){
    S32 ret = n;

    for(S32 i=0; i<n; i++){
        if(number & (1 << i))
            return i;
    }

    return ret;
}

S32 BitCount(U32 X, U32 N){
    S32 result = 0;

    for(S32 i=0; i<N; i++){
        if(((X >> i) & 1) == 1)
            result++;
    }

    return result;
}

U64 Ones(S32 len, S32 N){
    (void)N;
    U64 ret = 0;

    for(S32 i=len-1; i>=0; i--)
        ret |= ((U64)1 << i);

    return ret;
}

S32 DecodeBitMasks(U32 N, U32 imms, U32 immr,
        S32 immediate, U64 *out){
    U32 num = (N << 6) | (~imms & 0x3f);
    U32 len = HighestSetBit(num, 7);

    if(len < 1)
        return -1;

    U32 levels = Ones(len, 0);

    if(immediate && ((imms & levels) == levels))
        return -1;

    U32 S = imms & levels;
    U32 R = immr & levels;
    U32 esize = 1 << len;

    *out = replicate(RORZeroExtendOnes(S + 1, esize, R), sizeof(U64) * CHAR_BIT, esize);

    return 0;
}

/*
 * num: the number to replicate
 * nbits: how many bits make up this number
 * cnt: how many times to replicate
 */
U64 replicate(U64 num, S32 nbits, S32 cnt){
    U64 result = 0;

    for(S32 i=0; i<cnt; i++){
        result <<= nbits;
        result |= num;
    }

    return result;
}

S32 MoveWidePreferred(U32 sf, U32 immN, U32 immr,
        U32 imms){
    S32 width = sf == 1 ? 64 : 32;
    U32 combined = (immN << 6) | imms;

    if(sf == 1 && (combined >> 6) != 1)
        return 0;

    if(sf == 0 && (combined >> 5) != 0)
        return 0;

    if(imms < 16)
        return (-immr % 16) <= (15 - imms);

    if(imms >= (width - 15))
        return (immr % 16) <= (imms - (width - 15));

    return 0;
}

S32 IsZero(U64 x){
    return x == 0;
}

S32 IsOnes(U64 x, S32 n){
    return x == Ones(n, 0);
}

S32 BFXPreferred(U32 sf, U32 uns,
        U32 imms, U32 immr){
    if(imms < immr)
        return 0;

    if(imms == ((sf << 6) | 0x3f))
        return 0;

    if(immr == 0){
        if(sf == 0 && (imms == 0x7 || imms == 0xf))
            return 0;
        else if(((sf << 1) | uns) == 0x2 && (imms == 0x7 || imms == 0xf || imms == 0x1f))
            return 0;
    }

    return 1;
}

char *decode_reg_extend(U32 op){
    switch(op){
        case 0x0:
            return "uxtb";
        case 0x1:
            return "uxth";
        case 0x2:
            return "uxtw";
        case 0x3:
            return "uxtx";
        case 0x4:
            return "sxtb";
        case 0x5:
            return "sxth";
        case 0x6:
            return "sxtw";
        case 0x7:
            return "sxtx";
        default:
            return NULL;
    };
}

const char *decode_cond(U32 cond){
    switch(cond){
        case 0: return "eq";
        case 1: return "ne";
        case 2: return "cs";
        case 3: return "cc";
        case 4: return "mi";
        case 5: return "pl";
        case 6: return "vs";
        case 7: return "vc";
        case 8: return "hi";
        case 9: return "ls";
        case 10: return "ge";
        case 11: return "lt";
        case 12: return "gt";
        case 13: return "le";
        case 14: return "al";
        case 15: return "nv";
        default: return NULL;
    };
}

const char *get_arrangement(U32 size, U32 Q){
    if(size == 0)
        return Q == 0 ? "8b" : "16b";
    if(size == 1)
        return Q == 0 ? "4h" : "8h";
    if(size == 2)
        return Q == 0 ? "2s" : "4s";
    if(size == 3)
        return Q == 0 ? "1d" : "2d";

    return NULL;
}
