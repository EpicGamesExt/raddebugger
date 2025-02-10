#include <stdio.h>
#include <stdlib.h>

#include "adefs.h"
#include "bits.h"
#include "common.h"
#include "instruction.h"
#include "utils.h"
#include "strext.h"

#define NO_ALLOCATE 0
#define POST_INDEXED 1
#define OFFSET 2
#define PRE_INDEXED 3

#define UNSIGNED_IMMEDIATE -1

#define UNSCALED_IMMEDIATE 0
#define IMMEDIATE_POST_INDEXED 1
#define UNPRIVILEGED 2
#define IMMEDIATE_PRE_INDEXED 3

#define AD_NONE_C2099_MSVC -1
static struct itab unscaled_instr_tbl[] = {
    { "sturb", AD_INSTR_STURB },
    { "ldurb", AD_INSTR_LDURB },
    { "ldursb", AD_INSTR_LDURSB },
    { "ldursb", AD_INSTR_LDURSB },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { "sturh", AD_INSTR_STURH },
    { "ldurh", AD_INSTR_LDURH },
    { "ldursh", AD_INSTR_LDURSH },
    { "ldursh", AD_INSTR_LDURSH },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { "ldursw", AD_INSTR_LDURSW },
    { NULL, AD_NONE_C2099_MSVC },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR },
    { "prfum", AD_INSTR_PRFUM },
    { NULL, AD_NONE_C2099_MSVC },
    { "stur", AD_INSTR_STUR },
    { "ldur", AD_INSTR_LDUR }
};

static struct itab pre_post_unsigned_register_idx_instr_tbl[] = {
    { "strb", AD_INSTR_STRB },
    { "ldrb", AD_INSTR_LDRB },
    { "ldrsb", AD_INSTR_LDRSB },
    { "ldrsb", AD_INSTR_LDRSB },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { "strh", AD_INSTR_STRH },
    { "ldrh", AD_INSTR_LDRH },
    { "ldrsh", AD_INSTR_LDRSH },
    { "ldrsh", AD_INSTR_LDRSH },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { "ldrsw", AD_INSTR_LDRSW },
    { NULL, AD_NONE_C2099_MSVC },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR },
    { "prfm", AD_INSTR_PRFM },
    { NULL, AD_NONE_C2099_MSVC },
    { "str", AD_INSTR_STR },
    { "ldr", AD_INSTR_LDR }
};

static struct itab unprivileged_instr_tbl[] = {
    { "sttrb", AD_INSTR_STTRB },
    { "ldtrb", AD_INSTR_LDTRB },
    { "ldtrsb", AD_INSTR_LDTRSB },
    { "ldtrsb", AD_INSTR_LDTRSB },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "sttrh", AD_INSTR_STTRH },
    { "ldtrh", AD_INSTR_LDTRH },
    { "ldtrsh", AD_INSTR_LDTRSH },
    { "ldtrsh", AD_INSTR_LDTRSH },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "sttr", AD_INSTR_STTR },
    { "ldtr", AD_INSTR_LDTR },
    { "ldtrsw", AD_INSTR_LDTRSW },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { NULL, AD_NONE_C2099_MSVC },
    { "sttr", AD_INSTR_STTR },
    { "ldtr", AD_INSTR_LDTR },
    { NULL, AD_NONE_C2099_MSVC }
};

static S32 get_post_idx_immediate_offset(S32 regamount, U32 Q){
    if(regamount == 1)
        return Q == 0 ? 8 : 16;
    if(regamount == 2)
        return Q == 0 ? 16 : 32;
    if(regamount == 3)
        return Q == 0 ? 24 : 48;
    if(regamount == 4)
        return Q == 0 ? 32 : 64;

    return -1;
}

static S32 DisassembleLoadStoreMultStructuresInstr(struct instruction *i,
        struct ad_insn *out, S32 postidxed){
    U32 Q = bits(i->opcode, 30, 30);
    U32 L = bits(i->opcode, 22, 22);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 12, 15);
    U32 size = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    const char *T = get_arrangement(size, Q);

    if(!T)
        return 1;

    ADD_FIELD(out, Q);
    ADD_FIELD(out, L);

    if(postidxed)
        ADD_FIELD(out, Rm);

    ADD_FIELD(out, opcode);
    ADD_FIELD(out, size);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(L == 0)
        instr_s = "st";
    else
        instr_s = "ld";

    U32 regcnt, selem;

    switch(opcode){
        case 0: regcnt = 4; selem = 4; break;
        case 0x2: regcnt = 4; selem = 1; break;
        case 0x4: regcnt = 3; selem = 3; break;
        case 0x6: regcnt = 3; selem = 1; break;
        case 0x7: regcnt = 1; selem = 1; break;
        case 0x8: regcnt = 2; selem = 2; break;
        case 0xa: regcnt = 2; selem = 1; break;
        default: return 1;
    };

    if(L == 0)
        instr_id = (AD_INSTR_ST1 - 1) + selem;
    else
        /* the way the AD_INSTR_* enum is set up makes this more complicated */
        instr_id = (AD_INSTR_LD1 - 1) + ((selem * 2) - 1);

    concat(&DECODE_STR(out), "%s%d { ", instr_s, selem);

    for(S32 i=Rt; i<(Rt+regcnt)-1; i++){
        ADD_REG_OPERAND(out, i, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
        const char *Ri_s = GET_FP_REG(AD_RTBL_FP_V_128, i);

        concat(&DECODE_STR(out), "%s.%s, ", Ri_s, T);
    }

    ADD_REG_OPERAND(out, (Rt+regcnt)-1, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
            _RTBL(AD_RTBL_FP_V_128));
    const char *last_Rt_s = GET_FP_REG(AD_RTBL_FP_V_128, (Rt+regcnt)-1);

    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    concat(&DECODE_STR(out), "%s.%s }, [%s]", last_Rt_s, T, Rn_s);

    if(postidxed){
        if(Rm != 0x1f){
            ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_64));
            const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, NO_PREFER_ZR);

            concat(&DECODE_STR(out), ", %s", Rm_s);
        }
        else{
            S32 imm = get_post_idx_immediate_offset(regcnt, Q);

            if(imm == -1)
                return 1;

            /* imm is unsigned, that fxn returns -1 for error checking */
            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm);

            concat(&DECODE_STR(out), ", #%#x", (U32)imm);
        }
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadStoreSingleStructuresInstr(struct instruction *i,
        struct ad_insn *out, S32 postidxed){
    U32 Q = bits(i->opcode, 30, 30);
    U32 L = bits(i->opcode, 22, 22);
    U32 R = bits(i->opcode, 21, 21);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 13, 15);
    U32 S = bits(i->opcode, 12, 12);
    U32 size = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Q);
    ADD_FIELD(out, L);
    ADD_FIELD(out, R);

    if(postidxed)
        ADD_FIELD(out, Rm);

    ADD_FIELD(out, opcode);
    ADD_FIELD(out, S);
    ADD_FIELD(out, size);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(L == 0)
        instr_s = "st";
    else
        instr_s = "ld";

    const char *suffix = NULL;

    U32 scale = opcode >> 1;
    U32 selem = (((opcode & 1) << 1) | R) + 1;
    U32 index = 0;

    S32 replicate = 0;

    switch(scale){
        case 3: replicate = 1; break;
        case 0:
            {
                index = (Q << 3) | (S << 2) | size;
                suffix = "b";
                break;
            }
        case 1:
            {
                index = (Q << 2) | (S << 1) | (size >> 1);
                suffix = "h";
                break;
            }
        case 2:
            {
                if((size & 1) == 0){
                    index = (Q << 1) | S;
                    suffix = "s";
                }
                else{
                    index = Q;
                    suffix = "d";
                }

                break;
            }
        default: return 1;
    };

    if(replicate)
        instr_id = (AD_INSTR_LD1R - 1) + ((selem * 2) - 1);
    else if(L == 0)
        instr_id = (AD_INSTR_ST1 - 1) + selem;
    else if(L == 1)
        instr_id = (AD_INSTR_LD1 - 1) + ((selem * 2) - 1);

    concat(&DECODE_STR(out), "%s%d%s { ", instr_s, selem, replicate ? "r" : "");

    for(S32 i=Rt; i<(Rt+selem)-1; i++){
        ADD_REG_OPERAND(out, i, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_FP_V_128));
        const char *Ri_s = GET_FP_REG(AD_RTBL_FP_V_128, i);

        concat(&DECODE_STR(out), "%s", Ri_s);

        if(replicate){
            const char *T = get_arrangement(size, Q);

            if(!T)
                return 1;

            concat(&DECODE_STR(out), ".%s", T);
        }
        else{
            concat(&DECODE_STR(out), ".%s", suffix);
        }

        concat(&DECODE_STR(out), ", ");
    }

    ADD_REG_OPERAND(out, (Rt+selem)-1, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
            _RTBL(AD_RTBL_FP_V_128));
    const char *last_Rt_s = GET_FP_REG(AD_RTBL_FP_V_128, (Rt+selem)-1);

    concat(&DECODE_STR(out), "%s", last_Rt_s);

    if(replicate){
        const char *T = get_arrangement(size, Q);

        if(!T)
            return 1;

        concat(&DECODE_STR(out), ".%s", T);
    }
    else{
        concat(&DECODE_STR(out), ".%s", suffix);
    }

    concat(&DECODE_STR(out), " }");

    if(!replicate){
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&index);
        concat(&DECODE_STR(out), "[%d]", index);
    }

    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    concat(&DECODE_STR(out), ", [%s]", Rn_s);

    S32 rimms[] = { 1, 2, 4, 8 };

    if(postidxed){
        if(replicate){
            if(Rm != 0x1f){
                ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                        _RTBL(AD_RTBL_GEN_64));
                const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, NO_PREFER_ZR);

                concat(&DECODE_STR(out), ", %s", Rm_s);
            }
            else{
                U32 imm = rimms[size] * selem;
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm);

                concat(&DECODE_STR(out), ", #%#x", imm);
            }
        }
        else{
            if(Rm != 0x1f){
                ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                        _RTBL(AD_RTBL_GEN_64));
                const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, NO_PREFER_ZR);

                concat(&DECODE_STR(out), ", %s", Rm_s);
            }
            else{
                S32 idx = 0;

                if(*suffix == 'h')
                    idx = 1;
                else if(*suffix == 's')
                    idx = 2;
                else if(*suffix == 'd')
                    idx = 3;

                U32 imm = rimms[idx] * selem;
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm);

                concat(&DECODE_STR(out), ", #%#x", imm);
            }
        }
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadStoreMemoryTagsInstr(struct instruction *i,
        struct ad_insn *out){
    U32 opc = bits(i->opcode, 22, 23);
    U32 imm9 = bits(i->opcode, 12, 20);
    U32 op2 = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    if((opc == 2 || opc == 3) && imm9 != 0 && op2 == 0)
        return 1;

    ADD_FIELD(out, opc);
    ADD_FIELD(out, imm9);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);
    const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

    if((opc == 0 || opc == 2 || opc == 3) && imm9 == 0 && op2 == 0){
        const char *instr_s = NULL;

        if(opc == 0){
            instr_s = "stzgm";
            instr_id = AD_INSTR_STZGM;
        }
        else if(opc == 2){
            instr_s = "stgm";
            instr_id = AD_INSTR_STGM;
        }
        else{
            instr_s = "ldgm";
            instr_id = AD_INSTR_LDGM;
        }

        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "%s %s, [%s]", instr_s, Rt_s, Rn_s);
    }
    else if(opc == 1 && op2 == 0){
        instr_id = AD_INSTR_LDG;

        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "ldg %s, [%s", Rt_s, Rn_s);

        if(imm9 != 0){
            signed simm = sign_extend(imm9, 9) << 4;

            ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&simm);

            concat(&DECODE_STR(out), ", #"S_X"", S_A(simm));
        }

        concat(&DECODE_STR(out), "]");
    }
    else if(op2 > 0){
        enum {
            post = 1, signed_ = 2, pre
        };

        const char *instr_s = NULL;

        if(opc == 0){
            instr_s = "stg";
            instr_id = AD_INSTR_STG;
        }
        else if(opc == 1){
            instr_s = "stzg";
            instr_id = AD_INSTR_STZG;
        }
        else if(opc == 2){
            instr_s = "st2g";
            instr_id = AD_INSTR_ST2G;
        }
        else if(opc == 3){
            instr_s = "stz2g";
            instr_id = AD_INSTR_STZ2G;
        }

        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "%s %s, [%s", instr_s, Rt_s, Rn_s);

        if(imm9 == 0)
            concat(&DECODE_STR(out), "]");
        else{
            signed simm = sign_extend(imm9, 9) << 4;

            ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&simm);

            if(op2 == post)
                concat(&DECODE_STR(out), "], #"S_X"", S_A(simm));
            else{
                concat(&DECODE_STR(out), ", #"S_X"]", S_A(simm));

                if(op2 == pre)
                    concat(&DECODE_STR(out), "!");
            }
        }
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadAndStoreExclusiveInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 30, 31);
    U32 o2 = bits(i->opcode, 23, 23);
    U32 L = bits(i->opcode, 22, 22);
    U32 o1 = bits(i->opcode, 21, 21);
    U32 Rs = bits(i->opcode, 16, 20);
    U32 o0 = bits(i->opcode, 15, 15);
    U32 Rt2 = bits(i->opcode, 10, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    ADD_FIELD(out, size);
    ADD_FIELD(out, o2);
    ADD_FIELD(out, L);
    ADD_FIELD(out, o1);
    ADD_FIELD(out, Rs);
    ADD_FIELD(out, o0);
    ADD_FIELD(out, Rt2);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    U32 encoding = (o2 << 3) | (L << 2) | (o1 << 1) | o0;
    S32 instr_id = AD_NONE;

    if((size == 0 || size == 1) && (encoding == 2 || encoding == 3 ||
                encoding == 6 || encoding == 7)){
        U32 sz = size & 1;
        const char **registers = AD_RTBL_GEN_32;

        if(sz == 1)
            registers = AD_RTBL_GEN_64;

        S32 rsz = (registers == AD_RTBL_GEN_64 ? _64_BIT : _32_BIT);

        const char *Rs_s = GET_GEN_REG(registers, Rs, PREFER_ZR);
        const char *Rs1_s = GET_GEN_REG(registers, Rs + 1, PREFER_ZR);
        const char *Rt_s = GET_GEN_REG(registers, Rt, PREFER_ZR);
        const char *Rt1_s = GET_GEN_REG(registers, Rt + 1, PREFER_ZR);

        /* always 64 bit */
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        ADD_REG_OPERAND(out, Rs, rsz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rs + 1, rsz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rt, rsz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rt + 1, rsz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        const char *instr_s = NULL;

        if(encoding == 2){
            instr_s = "casp";
            instr_id = AD_INSTR_CASP;
        }
        else if(encoding == 3){
            instr_s = "caspl";
            instr_id = AD_INSTR_CASPL;
        }
        else if(encoding == 6){
            instr_s = "caspa";
            instr_id = AD_INSTR_CASPA;
        }
        else{
            instr_s = "caspal";
            instr_id = AD_INSTR_CASPAL;
        }

        concat(&DECODE_STR(out), "%s %s, %s, %s, %s, [%s]", instr_s,
                Rs_s, Rs1_s, Rt_s, Rt1_s, Rn_s);
    }
    else if((size == 0 || size == 1 || size == 2 || size == 3) &&
            (encoding == 10 || encoding == 11 || encoding == 14 || encoding == 15)){
        const char **Rs_Rt_Rtbl = AD_RTBL_GEN_32;
        U32 Rs_Rt_Sz = _32_BIT;

        if(size == 3){
            Rs_Rt_Rtbl = AD_RTBL_GEN_64;
            Rs_Rt_Sz = _64_BIT;
        }

        const char *Rs_s = GET_GEN_REG(Rs_Rt_Rtbl, Rs, PREFER_ZR);
        const char *Rt_s = GET_GEN_REG(Rs_Rt_Rtbl, Rt, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        ADD_REG_OPERAND(out, Rs, Rs_Rt_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rs_Rt_Rtbl);
        ADD_REG_OPERAND(out, Rt, Rs_Rt_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rs_Rt_Rtbl);
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        const char *instr_s = NULL;

        /* we'll figure out the suffixes and adjust insn ID later */
        if(encoding == 10){
            instr_s = "cas";
            instr_id = AD_INSTR_CASB;
        }
        else if(encoding == 11){
            instr_s = "casl";
            instr_id = AD_INSTR_CASLB;
        }
        else if(encoding == 14){
            instr_s = "casa";
            instr_id = AD_INSTR_CASAB;
        }
        else{
            instr_s = "casal";
            instr_id = AD_INSTR_CASALB;
        }

        const char *suffix = NULL; 

        if(size == 0)
            suffix = "b";
        else if(size == 1){
            instr_id += 4;
            suffix = "h";
        }
        else{
            if(encoding == 10)
                instr_id += 10;
            else if(encoding == 11)
                instr_id += 12;
            else
                instr_id += 13;

            suffix = "";
        }

        concat(&DECODE_STR(out), "%s%s %s, %s, [%s]", instr_s, suffix,
                Rs_s, Rt_s, Rn_s);
    }
    else if(size == 0 || size == 1){
        /* We'll figure out if this deals with bytes or halfwords later.
         * For now, set the instruction id to the instruction which deals with
         * bytes, and if we find out this instruction actually deals
         * with halfwords, we increment the instruction ID. Additionally,
         * we'll add the last character to the instruction string later on.
         */
        struct itab tab[] = {
            { "stxr", AD_INSTR_STXRB },
            { "stlxr", AD_INSTR_STLXRB },
            { NULL, AD_NONE },
            { NULL, AD_NONE },
            { "ldxr", AD_INSTR_LDXRB },
            { "ldaxr", AD_INSTR_LDAXRB },
            { NULL, AD_NONE },
            { NULL, AD_NONE },
            { "stllr", AD_INSTR_STLLRB },
            { "stlr", AD_INSTR_STLRB },
            { NULL, AD_NONE },
            { NULL, AD_NONE },
            { "ldlar", AD_INSTR_LDLARB },
            { "ldar", AD_INSTR_LDARB },
        };

        const char *instr_s = tab[encoding].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[encoding].instr_id;

        /* insn deals with bytes */
        if(size == 0)
            concat(&DECODE_STR(out), "%sb ", instr_s);
        else{
            concat(&DECODE_STR(out), "%sh ", instr_s);

            instr_id++;
        }

        if(instr_id == AD_INSTR_STXRB || instr_id == AD_INSTR_STLXRB ||
                instr_id == AD_INSTR_STXRH || instr_id == AD_INSTR_STLXRH){
            const char *Rs_s = GET_GEN_REG(AD_RTBL_GEN_32, Rs, PREFER_ZR);

            ADD_REG_OPERAND(out, Rs, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_32));

            concat(&DECODE_STR(out), "%s, ", Rs_s);
        }

        const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_32, Rt, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        ADD_REG_OPERAND(out, Rt, _SZ(_32_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_32));
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "%s, [%s]", Rt_s, Rn_s);
    }
    else if(size == 2 || size == 3){
        struct itab tab[] = {
            { "stxr", AD_INSTR_STXR },
            { "stlxr", AD_INSTR_STLXR },
            { "stxp", AD_INSTR_STXP },
            { "stlxp", AD_INSTR_STLXP },
            { "ldxr", AD_INSTR_LDXR },
            { "ldaxr", AD_INSTR_LDAXR },
            { "ldxp", AD_INSTR_LDXP },
            { "ldaxp", AD_INSTR_LDAXP },
            { "stllr", AD_INSTR_STLLR },
            { "stlr", AD_INSTR_STLR },
            { NULL, AD_NONE },
            { NULL, AD_NONE },
            { "ldlar", AD_INSTR_LDLAR },
            { "ldar", AD_INSTR_LDAR },
        };

        const char *instr_s = tab[encoding].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[encoding].instr_id;

        const char *Rs_s = GET_GEN_REG(AD_RTBL_GEN_32, Rs, PREFER_ZR);
        
        const char **Rt_Rtbl = AD_RTBL_GEN_32;
        U32 Rt_Sz = _32_BIT;

        U32 Rt1 = Rt;
        const char **Rt1_Rtbl = AD_RTBL_GEN_32;
        U32 Rt1_Sz = _32_BIT;

        const char **Rt2_Rtbl = AD_RTBL_GEN_32;
        U32 Rt2_Sz = _32_BIT;

        if(size == 3){
            Rt_Rtbl = AD_RTBL_GEN_64;
            Rt_Sz = _64_BIT;

            Rt1_Rtbl = AD_RTBL_GEN_64;
            Rt1_Sz = _64_BIT;

            Rt2_Rtbl = AD_RTBL_GEN_64;
            Rt2_Sz = _64_BIT;
        }

        const char *Rt_s = GET_GEN_REG(Rt_Rtbl, Rt, PREFER_ZR);
        const char *Rt1_s = Rt_s;
        const char *Rt2_s = GET_GEN_REG(Rt2_Rtbl, Rt2, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        concat(&DECODE_STR(out), "%s ", instr_s);

        if(instr_id == AD_INSTR_STXR || instr_id == AD_INSTR_STLXR){
            ADD_REG_OPERAND(out, Rs, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_32));
            ADD_REG_OPERAND(out, Rt, Rt_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt_Rtbl);

            concat(&DECODE_STR(out), "%s, %s", Rs_s, Rt_s);
        }
        else if(instr_id == AD_INSTR_STXP || instr_id == AD_INSTR_STLXP){
            ADD_REG_OPERAND(out, Rs, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_32));
            ADD_REG_OPERAND(out, Rt1, Rt1_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt1_Rtbl);
            ADD_REG_OPERAND(out, Rt2, Rt2_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt2_Rtbl);

            concat(&DECODE_STR(out), "%s, %s, %s", Rs_s, Rt1_s, Rt2_s);
        }
        else if(instr_id == AD_INSTR_LDXP || instr_id == AD_INSTR_LDAXP){
            ADD_REG_OPERAND(out, Rt1, Rt1_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt1_Rtbl);
            ADD_REG_OPERAND(out, Rt2, Rt2_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt2_Rtbl);

            concat(&DECODE_STR(out), "%s, %s", Rt1_s, Rt2_s);
        }
        else{
            ADD_REG_OPERAND(out, Rt, Rt_Sz, PREFER_ZR, _SYSREG(AD_NONE), Rt_Rtbl);

            concat(&DECODE_STR(out), "%s", Rt_s);
        }

        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), ", [%s]", Rn_s);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLDAPR_STLRInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 30, 31);
    U32 opc = bits(i->opcode, 22, 23);
    U32 imm9 = bits(i->opcode, 12, 20);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    if(size == 2 && opc == 3)
        return 1;

    if(size == 3 && (opc == 2 || opc == 3))
        return 1;

    ADD_FIELD(out, size);
    ADD_FIELD(out, opc);
    ADD_FIELD(out, imm9);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    imm9 = sign_extend(imm9, 9);

    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    if(size == 0){
        if(opc == 0){
            instr_s = "stlurb";
            instr_id = AD_INSTR_STLURB;
        }
        else if(opc == 1){
            instr_s = "ldapurb";
            instr_id = AD_INSTR_LDAPURB;
        }
        else{
            instr_s = "ldapursb";
            instr_id = AD_INSTR_LDAPURSB;
        }

        const char *Rt_s = opc == 2 ?
            GET_GEN_REG(AD_RTBL_GEN_64, Rt, PREFER_ZR) :
            GET_GEN_REG(AD_RTBL_GEN_32, Rt, PREFER_ZR);

        ADD_REG_OPERAND(out, Rt, _SZ(opc == 2 ? _64_BIT : _32_BIT),
                PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(opc == 2 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32));

        concat(&DECODE_STR(out), "%s %s", instr_s, Rt_s);
    }
    else if(size == 1){
        if(opc == 0){
            instr_s = "stlurh";
            instr_id = AD_INSTR_STLURH;
        }
        else if(opc == 1){
            instr_s = "ldapurh";
            instr_id = AD_INSTR_LDAPURH;
        }
        else{
            instr_s = "ldapursh";
            instr_id = AD_INSTR_LDAPURSH;
        }

        const char *Rt_s = opc == 2 ?
            GET_GEN_REG(AD_RTBL_GEN_64, Rt, PREFER_ZR) :
            GET_GEN_REG(AD_RTBL_GEN_32, Rt, PREFER_ZR);

        ADD_REG_OPERAND(out, Rt, _SZ(opc == 2 ? _64_BIT : _32_BIT),
                PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(opc == 2 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32));

        concat(&DECODE_STR(out), "%s %s", instr_s, Rt_s);
    }
    else if(size == 2){
        struct itab tab[] = {
            { "stlur", AD_INSTR_STLUR },
            { "ldapur", AD_INSTR_LDAPUR },
            { "ldapursw", AD_INSTR_LDAPURSW },
        };

        instr_s = tab[opc].instr_s;
        instr_id = tab[opc].instr_id;

        const char *Rt_s = opc > 1 ?
            GET_GEN_REG(AD_RTBL_GEN_64, Rt, PREFER_ZR) :
            GET_GEN_REG(AD_RTBL_GEN_32, Rt, PREFER_ZR);

        ADD_REG_OPERAND(out, Rt, _SZ(opc > 1 ? _64_BIT : _32_BIT),
                PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(opc > 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32));

        concat(&DECODE_STR(out), "%s %s", instr_s, Rt_s);
    }
    else{
        struct itab tab[] = {
            { "stlur", AD_INSTR_STLUR },
            { "ldapur", AD_INSTR_LDAPUR },
        };

        instr_s = tab[opc].instr_s;
        instr_id = tab[opc].instr_id;

        const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, PREFER_ZR);

        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "%s %s", instr_s, Rt_s);
    }
    
    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
            _RTBL(AD_RTBL_GEN_64));

    concat(&DECODE_STR(out), ", [%s", Rn_s);

    if(imm9 == 0)
        concat(&DECODE_STR(out), "]");
    else{
        ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&imm9);

        concat(&DECODE_STR(out), ", #"S_X"]", S_A(imm9));
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadAndStoreLiteralInstr(struct instruction *i,
        struct ad_insn *out){
    U32 opc = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 imm19 = bits(i->opcode, 5, 23);
    U32 Rt = bits(i->opcode, 0, 4);

    if(opc == 3 && V == 1)
        return 1;

    ADD_FIELD(out, opc);
    ADD_FIELD(out, V);
    ADD_FIELD(out, imm19);
    ADD_FIELD(out, Rt);

    const char *instr_s = "ldr";
    S32 instr_id = AD_INSTR_LDR;

    imm19 = sign_extend((imm19 << 2), 21);

    S64 imm = (signed)imm19 + i->PC;

    if(opc == 2 && V == 0){
        instr_s = "ldrsw";
        instr_id = AD_INSTR_LDRSW;

        const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);
        ADD_REG_OPERAND(out, Rt, _64_BIT, NO_PREFER_ZR, _SYSREG(AD_NONE), AD_RTBL_GEN_64);

        concat(&DECODE_STR(out), "%s %s, #"S_LX"", instr_s, Rt_s, S_LA(imm));
    }
    else if(opc == 3 && V == 0){
        instr_s = "prfm";
        instr_id = AD_INSTR_PRFM;

        U32 type = bits(Rt, 3, 4);
        U32 target = bits(Rt, 1, 2);
        U32 policy = Rt & 1;

        const char *types[] = { "PLD", "PLI", "PST" };
        const char *targets[] = { "L1", "L2", "L3" };
        const char *policies[] = { "KEEP", "STRM" };

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&Rt);

        if(OOB(type, types) || OOB(target, targets) || OOB(policy, policies))
            concat(&DECODE_STR(out), "%s #%#x, #"S_LX"", instr_s, Rt, S_LA(imm));
        else{
            concat(&DECODE_STR(out), "%s %s%s%s, #"S_LX"", instr_s, types[type],
                    targets[target], policies[policy], S_LA(imm));
        }
    }
    else{
        const char *Rt_s = NULL;

        if(V == 0){
            const char **registers = AD_RTBL_GEN_32;
            U32 sz = _32_BIT;

            if(opc > 0){
                registers = AD_RTBL_GEN_64;
                sz = _64_BIT;
            }

            Rt_s = GET_GEN_REG(registers, Rt, NO_PREFER_ZR);
            ADD_REG_OPERAND(out, Rt, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), registers);
        }
        else{
            const char **registers_a[] = {
                AD_RTBL_FP_32, AD_RTBL_FP_64, AD_RTBL_FP_128
            };
            U32 szs[] = { _32_BIT, _64_BIT, _128_BIT };

            const char **registers = registers_a[opc];
            U32 sz = szs[opc];

            Rt_s = GET_FP_REG(registers, Rt);
            ADD_REG_OPERAND(out, Rt, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), registers);
        }

        concat(&DECODE_STR(out), "%s %s, #"S_LX"", instr_s, Rt_s, S_LA(imm));
    }

    ADD_IMM_OPERAND(out, AD_IMM_LONG, *(S64 *)&imm);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadAndStoreRegisterPairInstr(struct instruction *i,
        struct ad_insn *out, S32 kind){
    U32 opc = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 L = bits(i->opcode, 22, 22);
    U32 imm7 = bits(i->opcode, 15, 21);
    U32 Rt2 = bits(i->opcode, 10, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    if(opc == 3)
        return 1;

    ADD_FIELD(out, opc);
    ADD_FIELD(out, V);
    ADD_FIELD(out, L);
    ADD_FIELD(out, imm7);
    ADD_FIELD(out, Rt2);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    const char *Rt2_s = NULL, *Rn_s = NULL, *Rt_s = NULL;

    const char **registers = NULL;
    U32 sz = 0;

    if(opc == 0){
        registers = V == 0 ? AD_RTBL_GEN_32 : AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(opc == 1){
        registers = V == 0 ? AD_RTBL_GEN_64 : AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        registers = V == 0 ? AD_RTBL_GEN_64 : AD_RTBL_FP_128;
        sz = V == 0 ? _64_BIT : _128_BIT;
    }

    U32 scale;

    if(V == 0){
        Rt2_s = GET_FP_REG(registers, Rt2);
        Rt_s = GET_FP_REG(registers, Rt);

        scale = 2 + (opc >> 1);
    }
    else{
        Rt2_s = GET_GEN_REG(registers, Rt2, NO_PREFER_ZR);
        Rt_s = GET_GEN_REG(registers, Rt, NO_PREFER_ZR);

        scale = 2 + opc;
    }

    Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    ADD_REG_OPERAND(out, Rt, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), registers);
    ADD_REG_OPERAND(out, Rt2, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), registers);
    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
            AD_RTBL_GEN_64);

    U32 opco = (opc << 2) | (V << 1) | L;

    /* start at AD_INSTR_(STP|LDP) and adjust as needed */

    if(L == 0){
        instr_id = AD_INSTR_STP;
        
        concat(&DECODE_STR(out), "st");
    }
    else{
        instr_id = AD_INSTR_LDP;

        concat(&DECODE_STR(out), "ld");
    }

    if(kind == NO_ALLOCATE){
        instr_id--;
        concat(&DECODE_STR(out), "n");
    }
    else{
        if(opco == 4){
            instr_id -= 15;

            concat(&DECODE_STR(out), "g");

            scale = 4;
        }
    }

    concat(&DECODE_STR(out), "p");

    if(kind != NO_ALLOCATE && opco == 5){
        instr_id++;

        concat(&DECODE_STR(out), "sw");
    }

    concat(&DECODE_STR(out), " %s, %s, [%s", Rt_s, Rt2_s, Rn_s);

    imm7 = sign_extend(imm7, 7) << scale;

    if(imm7 == 0)
        concat(&DECODE_STR(out), "]");
    else{
        if(kind == POST_INDEXED)
            concat(&DECODE_STR(out), "], #"S_X"", S_A(imm7));
        else if(kind == OFFSET || kind == NO_ALLOCATE)
            concat(&DECODE_STR(out), ", #"S_X"]", S_A(imm7));
        else
            concat(&DECODE_STR(out), ", #"S_X"]!", S_A(imm7));

        ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&imm7);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadAndStoreRegisterInstr(struct instruction *i,
        struct ad_insn *out, S32 kind){
    U32 size = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 opc = bits(i->opcode, 22, 23);
    U32 imm9 = bits(i->opcode, 12, 20);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    ADD_FIELD(out, size);
    ADD_FIELD(out, V);
    ADD_FIELD(out, opc);
    ADD_FIELD(out, imm9);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    const char **registers = AD_RTBL_GEN_32;
    U32 sz = _32_BIT;

    const char *Rt_s = GET_GEN_REG(registers, Rt, PREFER_ZR);

    if(V == 0 && (opc == 2 || size == 3)){
        registers = AD_RTBL_GEN_64;
        sz = _64_BIT;

        Rt_s = GET_GEN_REG(registers, Rt, PREFER_ZR);
    }
    else if(V == 1){
        if(size == 0 && (opc == 0 || opc == 1)){
            registers = AD_RTBL_FP_8;
            sz = _8_BIT;
        }
        else if(size == 0 && (opc == 2 || opc == 3)){
            registers = AD_RTBL_FP_128;
            sz = _128_BIT;
        }
        else if(size == 1 && (opc == 0 || opc == 1)){
            registers = AD_RTBL_FP_16;
            sz = _16_BIT;
        }
        else if(size == 2 && (opc == 0 || opc == 1)){
            registers = AD_RTBL_FP_32;
            sz = _32_BIT;
        }
        else if(size == 3 && (opc == 0 || opc == 1)){
            registers = AD_RTBL_FP_64;
            sz = _64_BIT;
        }

        Rt_s = GET_FP_REG(registers, Rt);
    }

    ADD_REG_OPERAND(out, Rt, sz, PREFER_ZR, _SYSREG(AD_NONE), registers);

    U32 instr_idx = (size << 3) | (V << 2) | opc;
    struct itab *instr_tab = NULL;

    if(kind == UNSIGNED_IMMEDIATE || kind == IMMEDIATE_POST_INDEXED ||
            kind == IMMEDIATE_PRE_INDEXED){
        if(OOB(instr_idx, pre_post_unsigned_register_idx_instr_tbl))
            return 1;

        instr_tab = pre_post_unsigned_register_idx_instr_tbl;
    }
    else if(kind == UNPRIVILEGED){
        if(OOB(instr_idx, unprivileged_instr_tbl))
            return 1;

        instr_tab = unprivileged_instr_tbl;
    }
    else if(kind == UNSCALED_IMMEDIATE){
        if(OOB(instr_idx, unscaled_instr_tbl))
            return 1;

        instr_tab = unscaled_instr_tbl;
    }

    if(!instr_tab)
        return 1;

    const char *instr_s = instr_tab[instr_idx].instr_s;

    if(!instr_s)
        return 1;

    instr_id = instr_tab[instr_idx].instr_id;

    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);
    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
            AD_RTBL_GEN_64);

    concat(&DECODE_STR(out), "%s ", instr_s);

    imm9 = sign_extend(imm9, 9);

    if(instr_id == AD_INSTR_PRFUM){
        U32 type = bits(Rt, 3, 4);
        U32 target = bits(Rt, 1, 2);
        U32 policy = Rt & 1;

        const char *types[] = { "PLD", "PLI", "PST" };
        const char *targets[] = { "L1", "L2", "L3" };
        const char *policies[] = { "KEEP", "STRM" };

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm9);

        if(OOB(type, types) || OOB(target, targets) || OOB(policy, policies))
            concat(&DECODE_STR(out), "#%#x, ", Rt);
        else{
            concat(&DECODE_STR(out), "%s%s%s, ", types[type], targets[target],
                    policies[policy]);
        }
    }
    else{
        concat(&DECODE_STR(out), "%s, [%s", Rt_s, Rn_s);

        if(kind == UNSCALED_IMMEDIATE || kind == UNPRIVILEGED){
            if(imm9 == 0)
                concat(&DECODE_STR(out), "]");
            else
                concat(&DECODE_STR(out), ", #"S_X"]", S_A(imm9));
        }
        else if(kind == UNSIGNED_IMMEDIATE){
            U32 imm12 = bits(i->opcode, 10, 21);

            S32 fp = V == 1;
            S32 shift = 0;

            if(fp)
                shift = ((opc >> 1) << 2) | size;
            else if(instr_id == AD_INSTR_LDRH || instr_id == AD_INSTR_STRH ||
                    instr_id == AD_INSTR_LDRSH){
                shift = 1;
            }
            else if(instr_id == AD_INSTR_LDRSW){
                shift = 2;
            }
            else if(instr_id == AD_INSTR_STR || instr_id == AD_INSTR_LDR){
                shift = size;
            }

            U64 pimm = imm12 << shift;

            if(pimm != 0){
                ADD_IMM_OPERAND(out, AD_IMM_ULONG, *(U64 *)&pimm);

                concat(&DECODE_STR(out), ", #"S_LX"", S_LA(pimm));
            }

            concat(&DECODE_STR(out), "]");
        }
        else if(kind == IMMEDIATE_POST_INDEXED){
            concat(&DECODE_STR(out), "], #"S_X"", S_A(imm9));
        }
        else if(kind == IMMEDIATE_PRE_INDEXED){
            concat(&DECODE_STR(out), ", #"S_X"]!", S_A(imm9));
        }
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

/* returns 1 if the alias is meant to be used */
static S32 get_atomic_memory_op(U32 size, U32 V, U32 A,
        U32 R, U32 o3, U32 opc, U32 Rt, struct itab *instr,
        struct itab *alias){
    U32 encoding = size << 7;
    encoding |= V << 6;
    encoding |= A << 5;
    encoding |= R << 4;
    encoding |= o3 << 3;
    encoding |= opc;

    S32 use_alias = (A == 0 && Rt == 0x1f);

    switch(encoding){
        case 0x184:
            {
                instr->instr_s = "ldsmax";
                instr->instr_id = AD_INSTR_LDSMAX;
                alias->instr_s = "stsmax";
                alias->instr_id = AD_INSTR_STSMAX;
                break;
            }
        case 0x110:
            {
                instr->instr_s = "ldaddl";
                instr->instr_id = AD_INSTR_LDADDL;
                alias->instr_s = "staddl";
                alias->instr_id = AD_INSTR_STADDL;
                break;
            }
        case 0x1b4:
            {
                instr->instr_s = "ldsmaxal";
                instr->instr_id = AD_INSTR_LDSMAXAL;
                break;
            }
        case 0x120:
            {
                instr->instr_s = "ldadda";
                instr->instr_id = AD_INSTR_LDADDA;
                break;
            }
        case 0xac:
            {
                instr->instr_s = "ldaprh";
                instr->instr_id = AD_INSTR_LDAPRH;
                break;
            }
        case 0x38:
            {
                instr->instr_s = "swpalb";
                instr->instr_id = AD_INSTR_SWPALB;
                break;
            }
        case 0x8:
            {
                instr->instr_s = "swpb";
                instr->instr_id = AD_INSTR_SWPB;
                break;
            }
        case 0xa1:
            {
                instr->instr_s = "ldclrah";
                instr->instr_id = AD_INSTR_LDCLRAH;
                break;
            }
        case 0x35:
            {
                instr->instr_s = "ldsminalb";
                instr->instr_id = AD_INSTR_LDSMINALB;
                break;
            }
        case 0x91:
            {
                instr->instr_s = "ldclrlh";
                instr->instr_id = AD_INSTR_LDCLRLH;
                alias->instr_s = "stclrlh";
                alias->instr_id = AD_INSTR_STCLRLH;
                break;
            }
        case 0x5:
            {
                instr->instr_s = "ldsminb";
                instr->instr_id = AD_INSTR_LDSMINB;
                alias->instr_s = "stsminb";
                alias->instr_id = AD_INSTR_STSMINB;
                break;
            }
        case 0x97:
            {
                instr->instr_s = "lduminlh";
                instr->instr_id = AD_INSTR_LDUMINLH;
                alias->instr_s = "stuminlh";
                alias->instr_id = AD_INSTR_STUMINLH;
                break;
            }
        case 0x3:
            {
                instr->instr_s = "ldsetb";
                instr->instr_id = AD_INSTR_LDSETB;
                alias->instr_s = "stsetb";
                alias->instr_id = AD_INSTR_STSETB;
                break;
            }
        case 0xa7:
            {
                instr->instr_s = "lduminah";
                instr->instr_id = AD_INSTR_LDUMINAH;
                break;
            }
        case 0x33:
            {
                instr->instr_s = "ldsetalb";
                instr->instr_id = AD_INSTR_LDSETALB;
                break;
            }
        case 0x1b2:
            {
                instr->instr_s = "ldeoral";
                instr->instr_id = AD_INSTR_LDEORAL;
                break;
            }
        case 0x126:
            {
                instr->instr_s = "ldumaxa";
                instr->instr_id = AD_INSTR_LDUMAXA;
                break;
            }
        case 0x182:
            {
                instr->instr_s = "ldeor";
                instr->instr_id = AD_INSTR_LDEOR;
                alias->instr_s = "steor";
                alias->instr_id = AD_INSTR_STEOR;
                break;
            }
        case 0x116:
            {
                instr->instr_s = "ldumaxl";
                instr->instr_id = AD_INSTR_LDUMAXL;
                alias->instr_s = "stumaxl";
                alias->instr_id = AD_INSTR_STUMAXL;
                break;
            }
        case 0x105:
            {
                instr->instr_s = "ldsmin";
                instr->instr_id = AD_INSTR_LDSMIN;
                alias->instr_s = "stsmin";
                alias->instr_id = AD_INSTR_STSMIN;
                break;
            }
        case 0x191:
            {
                instr->instr_s = "ldclrl";
                instr->instr_id = AD_INSTR_LDCLRL;
                alias->instr_s = "stclrl";
                alias->instr_id = AD_INSTR_STCLRL;
                break;
            }
        case 0x135:
            {
                instr->instr_s = "ldsminal";
                instr->instr_id = AD_INSTR_LDSMINAL;
                break;
            }
        case 0x1a1:
            {
                instr->instr_s = "ldclra";
                instr->instr_id = AD_INSTR_LDCLRA;
                break;
            }
        case 0x108:
            {
                instr->instr_s = "swp";
                instr->instr_id = AD_INSTR_SWP;
                break;
            }
        case 0x138:
            {
                instr->instr_s = "swpal";
                instr->instr_id = AD_INSTR_SWPAL;
                break;
            }
        case 0x1ac:
            {
                instr->instr_s = "ldaddal";
                instr->instr_id = AD_INSTR_LDADDAL;
                break;
            }
        case 0x20:
            {
                instr->instr_s = "ldaddab";
                instr->instr_id = AD_INSTR_LDADDAB;
                break;
            }
        case 0xb4:
            {
                instr->instr_s = "ldsmaxalh";
                instr->instr_id = AD_INSTR_LDSMAXALH;
                break;
            }
        case 0x10:
            {
                instr->instr_s = "ldaddlb";
                instr->instr_id = AD_INSTR_LDADDLB;
                alias->instr_s = "staddlb";
                alias->instr_id = AD_INSTR_STADDLB;
                break;
            }
        case 0x84:
            {
                instr->instr_s = "ldsmaxh";
                instr->instr_id = AD_INSTR_LDSMAXH;
                alias->instr_s = "stsmaxh";
                alias->instr_id = AD_INSTR_STSMAXH;
                break;
            }
        case 0x16:
            {
                instr->instr_s = "ldumaxlb";
                instr->instr_id = AD_INSTR_LDUMAXLB;
                alias->instr_s = "stumaxlb";
                alias->instr_id = AD_INSTR_STUMAXLB;
                break;
            }
        case 0x82:
            {
                instr->instr_s = "ldeorh";
                instr->instr_id = AD_INSTR_LDEORH;
                alias->instr_s = "steorh";
                alias->instr_id = AD_INSTR_STEORH;
                break;
            }
        case 0x26:
            {
                instr->instr_s = "ldumaxab";
                instr->instr_id = AD_INSTR_LDUMAXAB;
                break;
            }
        case 0xb2:
            {
                instr->instr_s = "ldeoralh";
                instr->instr_id = AD_INSTR_LDEORALH;
                break;
            }
        case 0x133:
            {
                instr->instr_s = "ldsetal";
                instr->instr_id = AD_INSTR_LDSETAL;
                break;
            }
        case 0x1a7:
            {
                instr->instr_s = "ldumina";
                instr->instr_id = AD_INSTR_LDUMINA;
                break;
            }
        case 0x103:
            {
                instr->instr_s = "ldset";
                instr->instr_id = AD_INSTR_LDSET;
                alias->instr_s = "stset";
                alias->instr_id = AD_INSTR_STSET;
                break;
            }
        case 0x197:
            {
                instr->instr_s = "lduminl";
                instr->instr_id = AD_INSTR_LDUMINL;
                alias->instr_s = "stuminl";
                alias->instr_id = AD_INSTR_STUMINL;
                break;
            }
        case 0x180:
            {
                instr->instr_s = "ldadd";
                instr->instr_id = AD_INSTR_LDADD;
                alias->instr_s = "stadd";
                alias->instr_id = AD_INSTR_STADD;
                break;
            }
        case 0x114:
            {
                instr->instr_s = "ldsmaxl";
                instr->instr_id = AD_INSTR_LDSMAXL;
                alias->instr_s = "stsmaxl";
                alias->instr_id = AD_INSTR_STSMAXL;
                break;
            }
        case 0x124:
            {
                instr->instr_s = "ldsmaxa";
                instr->instr_id = AD_INSTR_LDSMAXA;
                break;
            }
        case 0xa8:
            {
                instr->instr_s = "swpah";
                instr->instr_id = AD_INSTR_SWPAH;
                break;
            }
        case 0x98:
            {
                instr->instr_s = "swplh";
                instr->instr_id = AD_INSTR_SWPLH;
                break;
            }
        case 0xa5:
            {
                instr->instr_s = "ldsminah";
                instr->instr_id = AD_INSTR_LDSMINAH;
                break;
            }
        case 0x31:
            {
                instr->instr_s = "ldclralb";
                instr->instr_id = AD_INSTR_LDCLRALB;
                break;
            }
        case 0x95:
            {
                instr->instr_s = "ldsminlh";
                instr->instr_id = AD_INSTR_LDSMINLH;
                alias->instr_s = "stsminlh";
                alias->instr_id = AD_INSTR_STSMINLH;
                break;
            }
        case 0x1:
            {
                instr->instr_s = "ldclrb";
                instr->instr_id = AD_INSTR_LDCLRB;
                alias->instr_s = "stclrb";
                alias->instr_id = AD_INSTR_STCLRB;
                break;
            }
        case 0x93:
            {
                instr->instr_s = "ldsetlh";
                instr->instr_id = AD_INSTR_LDSETLH;
                alias->instr_s = "stsetlh";
                alias->instr_id = AD_INSTR_STSETLH;
                break;
            }
        case 0x7:
            {
                instr->instr_s = "lduminb";
                instr->instr_id = AD_INSTR_LDUMINB;
                alias->instr_s = "stuminb";
                alias->instr_id = AD_INSTR_STUMINB;
                break;
            }
        case 0xa3:
            {
                instr->instr_s = "ldsetah";
                instr->instr_id = AD_INSTR_LDSETAH;
                break;
            }
        case 0x37:
            {
                instr->instr_s = "lduminalb";
                instr->instr_id = AD_INSTR_LDUMINALB;
                break;
            }
        case 0x1b6:
            {
                instr->instr_s = "ldumaxal";
                instr->instr_id = AD_INSTR_LDUMAXAL;
                break;
            }
        case 0x122:
            {
                instr->instr_s = "ldeora";
                instr->instr_id = AD_INSTR_LDEORA;
                break;
            }
        case 0x186:
            {
                instr->instr_s = "ldumax";
                instr->instr_id = AD_INSTR_LDUMAX;
                alias->instr_s = "stumax";
                alias->instr_id = AD_INSTR_STUMAX;
                break;
            }
        case 0x112:
            {
                instr->instr_s = "ldeorl";
                instr->instr_id = AD_INSTR_LDEORL;
                alias->instr_s = "steorl";
                alias->instr_id = AD_INSTR_STEORL;
                break;
            }
        case 0x101:
            {
                instr->instr_s = "ldclr";
                instr->instr_id = AD_INSTR_LDCLR;
                alias->instr_s = "stclr";
                alias->instr_id = AD_INSTR_STCLR;
                break;
            }
        case 0x195:
            {
                instr->instr_s = "ldsminl";
                instr->instr_id = AD_INSTR_LDSMINL;
                alias->instr_s = "stsminl";
                alias->instr_id = AD_INSTR_STSMINL;
                break;
            }
        case 0x131:
            {
                instr->instr_s = "ldclral";
                instr->instr_id = AD_INSTR_LDCLRAL;
                break;
            }
        case 0x1a5:
            {
                instr->instr_s = "ldsmina";
                instr->instr_id = AD_INSTR_LDSMINA;
                break;
            }
        case 0x198:
            {
                instr->instr_s = "swpl";
                instr->instr_id = AD_INSTR_SWPL;
                break;
            }
        case 0x1a8:
            {
                instr->instr_s = "swpa";
                instr->instr_id = AD_INSTR_SWPA;
                break;
            }
        case 0x24:
            {
                instr->instr_s = "ldsmaxab";
                instr->instr_id = AD_INSTR_LDSMAXAB;
                break;
            }
        case 0xb0:
            {
                instr->instr_s = "ldaddalh";
                instr->instr_id = AD_INSTR_LDADDALH;
                break;
            }
        case 0x14:
            {
                instr->instr_s = "ldsmaxlb";
                instr->instr_id = AD_INSTR_LDSMAXLB;
                alias->instr_s = "stsmaxlb";
                alias->instr_id = AD_INSTR_STSMAXLB;
                break;
            }
        case 0x80:
            {
                instr->instr_s = "ldaddh";
                instr->instr_id = AD_INSTR_LDADDH;
                alias->instr_s = "staddh";
                alias->instr_id = AD_INSTR_STADDH;
                break;
            }
        case 0x12:
            {
                instr->instr_s = "ldeorlb";
                instr->instr_id = AD_INSTR_LDEORLB;
                alias->instr_s = "steorlb";
                alias->instr_id = AD_INSTR_STEORLB;
                break;
            }
        case 0x86:
            {
                instr->instr_s = "ldumaxh";
                instr->instr_id = AD_INSTR_LDUMAXH;
                alias->instr_s = "stumaxh";
                alias->instr_id = AD_INSTR_STUMAXH;
                break;
            }
        case 0x22:
            {
                instr->instr_s = "ldeorab";
                instr->instr_id = AD_INSTR_LDEORAB;
                break;
            }
        case 0xb6:
            {
                instr->instr_s = "ldumaxalh";
                instr->instr_id = AD_INSTR_LDUMAXALH;
                break;
            }
        case 0x137:
            {
                instr->instr_s = "lduminal";
                instr->instr_id = AD_INSTR_LDUMINAL;
                break;
            }
        case 0x1a3:
            {
                instr->instr_s = "ldseta";
                instr->instr_id = AD_INSTR_LDSETA;
                break;
            }
        case 0x107:
            {
                instr->instr_s = "ldumin";
                instr->instr_id = AD_INSTR_LDUMIN;
                alias->instr_s = "stumin";
                alias->instr_id = AD_INSTR_STUMIN;
                break;
            }
        case 0x193:
            {
                instr->instr_s = "ldsetl";
                instr->instr_id = AD_INSTR_LDSETL;
                alias->instr_s = "stsetl";
                alias->instr_id = AD_INSTR_STSETL;
                break;
            }
        case 0x30:
            {
                instr->instr_s = "ldaddalb";
                instr->instr_id = AD_INSTR_LDADDALB;
                break;
            }
        case 0xa4:
            {
                instr->instr_s = "ldsmaxah";
                instr->instr_id = AD_INSTR_LDSMAXAH;
                break;
            }
        case 0x0:
            {
                instr->instr_s = "ldaddb";
                instr->instr_id = AD_INSTR_LDADDB;
                alias->instr_s = "staddb";
                alias->instr_id = AD_INSTR_STADDB;
                break;
            }
        case 0x94:
            {
                instr->instr_s = "ldsmaxlh";
                instr->instr_id = AD_INSTR_LDSMAXLH;
                alias->instr_s = "stsmaxlh";
                alias->instr_id = AD_INSTR_STSMAXLH;
                break;
            }
        case 0x115:
            {
                instr->instr_s = "ldsminl";
                instr->instr_id = AD_INSTR_LDSMINL;
                alias->instr_s = "stsminl";
                alias->instr_id = AD_INSTR_STSMINL;
                break;
            }
        case 0x181:
            {
                instr->instr_s = "ldclr";
                instr->instr_id = AD_INSTR_LDCLR;
                alias->instr_s = "stclr";
                alias->instr_id = AD_INSTR_STCLR;
                break;
            }
        case 0x125:
            {
                instr->instr_s = "ldsmina";
                instr->instr_id = AD_INSTR_LDSMINA;
                break;
            }
        case 0x1b1:
            {
                instr->instr_s = "ldclral";
                instr->instr_id = AD_INSTR_LDCLRAL;
                break;
            }
        case 0x118:
            {
                instr->instr_s = "swpl";
                instr->instr_id = AD_INSTR_SWPL;
                break;
            }
        case 0x128:
            {
                instr->instr_s = "swpa";
                instr->instr_id = AD_INSTR_SWPA;
                break;
            }
        case 0x123:
            {
                instr->instr_s = "ldseta";
                instr->instr_id = AD_INSTR_LDSETA;
                break;
            }
        case 0x1b7:
            {
                instr->instr_s = "lduminal";
                instr->instr_id = AD_INSTR_LDUMINAL;
                break;
            }
        case 0x113:
            {
                instr->instr_s = "ldsetl";
                instr->instr_id = AD_INSTR_LDSETL;
                alias->instr_s = "stsetl";
                alias->instr_id = AD_INSTR_STSETL;
                break;
            }
        case 0x187:
            {
                instr->instr_s = "ldumin";
                instr->instr_id = AD_INSTR_LDUMIN;
                alias->instr_s = "stumin";
                alias->instr_id = AD_INSTR_STUMIN;
                break;
            }
        case 0x6:
            {
                instr->instr_s = "ldumaxb";
                instr->instr_id = AD_INSTR_LDUMAXB;
                alias->instr_s = "stumaxb";
                alias->instr_id = AD_INSTR_STUMAXB;
                break;
            }
        case 0x92:
            {
                instr->instr_s = "ldeorlh";
                instr->instr_id = AD_INSTR_LDEORLH;
                alias->instr_s = "steorlh";
                alias->instr_id = AD_INSTR_STEORLH;
                break;
            }
        case 0x36:
            {
                instr->instr_s = "ldumaxalb";
                instr->instr_id = AD_INSTR_LDUMAXALB;
                break;
            }
        case 0xa2:
            {
                instr->instr_s = "ldeorah";
                instr->instr_id = AD_INSTR_LDEORAH;
                break;
            }
        case 0x28:
            {
                instr->instr_s = "swpab";
                instr->instr_id = AD_INSTR_SWPAB;
                break;
            }
        case 0x18:
            {
                instr->instr_s = "swplb";
                instr->instr_id = AD_INSTR_SWPLB;
                break;
            }
        case 0xb1:
            {
                instr->instr_s = "ldclralh";
                instr->instr_id = AD_INSTR_LDCLRALH;
                break;
            }
        case 0x25:
            {
                instr->instr_s = "ldsminab";
                instr->instr_id = AD_INSTR_LDSMINAB;
                break;
            }
        case 0x81:
            {
                instr->instr_s = "ldclrh";
                instr->instr_id = AD_INSTR_LDCLRH;
                alias->instr_s = "stclrh";
                alias->instr_id = AD_INSTR_STCLRH;
                break;
            }
        case 0x15:
            {
                instr->instr_s = "ldsminlb";
                instr->instr_id = AD_INSTR_LDSMINLB;
                alias->instr_s = "stsminlb";
                alias->instr_id = AD_INSTR_STSMINLB;
                break;
            }
        case 0x194:
            {
                instr->instr_s = "ldsmaxl";
                instr->instr_id = AD_INSTR_LDSMAXL;
                alias->instr_s = "stsmaxl";
                alias->instr_id = AD_INSTR_STSMAXL;
                break;
            }
        case 0x100:
            {
                instr->instr_s = "ldadd";
                instr->instr_id = AD_INSTR_LDADD;
                alias->instr_s = "stadd";
                alias->instr_id = AD_INSTR_STADD;
                break;
            }
        case 0x1a4:
            {
                instr->instr_s = "ldsmaxa";
                instr->instr_id = AD_INSTR_LDSMAXA;
                break;
            }
        case 0x1a2:
            {
                instr->instr_s = "ldeora";
                instr->instr_id = AD_INSTR_LDEORA;
                break;
            }
        case 0x136:
            {
                instr->instr_s = "ldumaxal";
                instr->instr_id = AD_INSTR_LDUMAXAL;
                break;
            }
        case 0x192:
            {
                instr->instr_s = "ldeorl";
                instr->instr_id = AD_INSTR_LDEORL;
                alias->instr_s = "steorl";
                alias->instr_id = AD_INSTR_STEORL;
                break;
            }
        case 0x106:
            {
                instr->instr_s = "ldumax";
                instr->instr_id = AD_INSTR_LDUMAX;
                alias->instr_s = "stumax";
                alias->instr_id = AD_INSTR_STUMAX;
                break;
            }
        case 0x87:
            {
                instr->instr_s = "lduminh";
                instr->instr_id = AD_INSTR_LDUMINH;
                alias->instr_s = "stuminh";
                alias->instr_id = AD_INSTR_STUMINH;
                break;
            }
        case 0x13:
            {
                instr->instr_s = "ldsetlb";
                instr->instr_id = AD_INSTR_LDSETLB;
                alias->instr_s = "stsetlb";
                alias->instr_id = AD_INSTR_STSETLB;
                break;
            }
        case 0xb7:
            {
                instr->instr_s = "lduminalh";
                instr->instr_id = AD_INSTR_LDUMINALH;
                break;
            }
        case 0x23:
            {
                instr->instr_s = "ldsetab";
                instr->instr_id = AD_INSTR_LDSETAB;
                break;
            }
        case 0x34:
            {
                instr->instr_s = "ldsmaxalb";
                instr->instr_id = AD_INSTR_LDSMAXALB;
                break;
            }
        case 0xa0:
            {
                instr->instr_s = "ldaddah";
                instr->instr_id = AD_INSTR_LDADDAH;
                break;
            }
        case 0x4:
            {
                instr->instr_s = "ldsmaxb";
                instr->instr_id = AD_INSTR_LDSMAXB;
                alias->instr_s = "stsmaxb";
                alias->instr_id = AD_INSTR_STSMAXB;
                break;
            }
        case 0x90:
            {
                instr->instr_s = "ldaddlh";
                instr->instr_id = AD_INSTR_LDADDLH;
                alias->instr_s = "staddlh";
                alias->instr_id = AD_INSTR_STADDLH;
                break;
            }
        case 0x111:
            {
                instr->instr_s = "ldclrl";
                instr->instr_id = AD_INSTR_LDCLRL;
                alias->instr_s = "stclrl";
                alias->instr_id = AD_INSTR_STCLRL;
                break;
            }
        case 0x185:
            {
                instr->instr_s = "ldsmin";
                instr->instr_id = AD_INSTR_LDSMIN;
                alias->instr_s = "stsmin";
                alias->instr_id = AD_INSTR_STSMIN;
                break;
            }
        case 0x121:
            {
                instr->instr_s = "ldclra";
                instr->instr_id = AD_INSTR_LDCLRA;
                break;
            }
        case 0x1b5:
            {
                instr->instr_s = "ldsminal";
                instr->instr_id = AD_INSTR_LDSMINAL;
                break;
            }
        case 0x188:
            {
                instr->instr_s = "swp";
                instr->instr_id = AD_INSTR_SWP;
                break;
            }
        case 0x130:
            {
                instr->instr_s = "ldaddal";
                instr->instr_id = AD_INSTR_LDADDAL;
                break;
            }
        case 0x1b8:
            {
                instr->instr_s = "swpal";
                instr->instr_id = AD_INSTR_SWPAL;
                break;
            }
        case 0x127:
            {
                instr->instr_s = "ldumina";
                instr->instr_id = AD_INSTR_LDUMINA;
                break;
            }
        case 0x1b3:
            {
                instr->instr_s = "ldsetal";
                instr->instr_id = AD_INSTR_LDSETAL;
                break;
            }
        case 0x117:
            {
                instr->instr_s = "lduminl";
                instr->instr_id = AD_INSTR_LDUMINL;
                alias->instr_s = "stuminl";
                alias->instr_id = AD_INSTR_STUMINL;
                break;
            }
        case 0x183:
            {
                instr->instr_s = "ldset";
                instr->instr_id = AD_INSTR_LDSET;
                alias->instr_s = "stset";
                alias->instr_id = AD_INSTR_STSET;
                break;
            }
        case 0x2:
            {
                instr->instr_s = "ldeorb";
                instr->instr_id = AD_INSTR_LDEORB;
                alias->instr_s = "steorb";
                alias->instr_id = AD_INSTR_STEORB;
                break;
            }
        case 0x96:
            {
                instr->instr_s = "ldumaxlh";
                instr->instr_id = AD_INSTR_LDUMAXLH;
                alias->instr_s = "stumaxlh";
                alias->instr_id = AD_INSTR_STUMAXLH;
                break;
            }
        case 0x32:
            {
                instr->instr_s = "ldeoralb";
                instr->instr_id = AD_INSTR_LDEORALB;
                break;
            }
        case 0xa6:
            {
                instr->instr_s = "ldumaxah";
                instr->instr_id = AD_INSTR_LDUMAXAH;
                break;
            }
        case 0xb8:
            {
                instr->instr_s = "swpalh";
                instr->instr_id = AD_INSTR_SWPALH;
                break;
            }
        case 0x2c:
            {
                instr->instr_s = "ldaprb";
                instr->instr_id = AD_INSTR_LDAPRB;
                break;
            }
        case 0x88:
            {
                instr->instr_s = "swph";
                instr->instr_id = AD_INSTR_SWPH;
                break;
            }
        case 0xb5:
            {
                instr->instr_s = "ldsminalh";
                instr->instr_id = AD_INSTR_LDSMINALH;
                break;
            }
        case 0x21:
            {
                instr->instr_s = "ldclrab";
                instr->instr_id = AD_INSTR_LDCLRAB;
                break;
            }
        case 0x85:
            {
                instr->instr_s = "ldsminh";
                instr->instr_id = AD_INSTR_LDSMINH;
                alias->instr_s = "stsminh";
                alias->instr_id = AD_INSTR_STSMINH;
                break;
            }
        case 0x11:
            {
                instr->instr_s = "ldclrlb";
                instr->instr_id = AD_INSTR_LDCLRLB;
                alias->instr_s = "stclrlb";
                alias->instr_id = AD_INSTR_STCLRLB;
                break;
            }
        case 0x190:
            {
                instr->instr_s = "ldaddl";
                instr->instr_id = AD_INSTR_LDADDL;
                alias->instr_s = "staddl";
                alias->instr_id = AD_INSTR_STADDL;
                break;
            }
        case 0x104:
            {
                instr->instr_s = "ldsmax";
                instr->instr_id = AD_INSTR_LDSMAX;
                alias->instr_s = "stsmax";
                alias->instr_id = AD_INSTR_STSMAX;
                break;
            }
        case 0x1a0:
            {
                instr->instr_s = "ldadda";
                instr->instr_id = AD_INSTR_LDADDA;
                break;
            }
        case 0x134:
            {
                instr->instr_s = "ldsmaxal";
                instr->instr_id = AD_INSTR_LDSMAXAL;
                break;
            }
        case 0x1a6:
            {
                instr->instr_s = "ldumaxa";
                instr->instr_id = AD_INSTR_LDUMAXA;
                break;
            }
        case 0x132:
            {
                instr->instr_s = "ldeoral";
                instr->instr_id = AD_INSTR_LDEORAL;
                break;
            }
        case 0x196:
            {
                instr->instr_s = "ldumaxl";
                instr->instr_id = AD_INSTR_LDUMAXL;
                alias->instr_s = "stumaxl";
                alias->instr_id = AD_INSTR_STUMAXL;
                break;
            }
        case 0x102:
            {
                instr->instr_s = "ldeor";
                instr->instr_id = AD_INSTR_LDEOR;
                alias->instr_s = "steor";
                alias->instr_id = AD_INSTR_STEOR;
                break;
            }
        case 0x83:
            {
                instr->instr_s = "ldseth";
                instr->instr_id = AD_INSTR_LDSETH;
                alias->instr_s = "stseth";
                alias->instr_id = AD_INSTR_STSETH;
                break;
            }
        case 0x17:
            {
                instr->instr_s = "lduminlb";
                instr->instr_id = AD_INSTR_LDUMINLB;
                alias->instr_s = "stuminlb";
                alias->instr_id = AD_INSTR_STUMINLB;
                break;
            }
        case 0xb3:
            {
                instr->instr_s = "ldsetalh";
                instr->instr_id = AD_INSTR_LDSETALH;
                break;
            }
        case 0x27:
            {
                instr->instr_s = "lduminab";
                instr->instr_id = AD_INSTR_LDUMINAB;
                break;
            }
        case 0x12c:
            {
                instr->instr_s = "ldapr";
                instr->instr_id = AD_INSTR_LDAPR;
                break;
            }
    };

    return use_alias;
}

static S32 DisassembleAtomicMemoryInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 A = bits(i->opcode, 23, 23);
    U32 R = bits(i->opcode, 22, 22);
    U32 Rs = bits(i->opcode, 16, 20);
    U32 o3 = bits(i->opcode, 15, 15);
    U32 opc = bits(i->opcode, 12, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    ADD_FIELD(out, size);
    ADD_FIELD(out, V);
    ADD_FIELD(out, A);
    ADD_FIELD(out, R);
    ADD_FIELD(out, Rs);
    ADD_FIELD(out, o3);
    ADD_FIELD(out, opc);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    if(V == 1)
        return 1;

    const char **registers = AD_RTBL_GEN_32;
    U64 sz = AD_RTBL_GEN_32_SZ;

    if(size > 2){
        registers = AD_RTBL_GEN_64;
        sz = AD_RTBL_GEN_64_SZ;
    }

    const char *Rs_s = GET_GEN_REG(registers, Rs, PREFER_ZR);
    const char *Rt_s = GET_GEN_REG(registers, Rt, PREFER_ZR);
    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    struct itab instr = {0}, alias = {0};

    S32 use_alias = get_atomic_memory_op(size, V, A, R, o3, opc, Rt, &instr, &alias);

    S32 instr_id = AD_NONE;

    if(use_alias){
        if(!alias.instr_s)
            return 1;

        concat(&DECODE_STR(out), "%s ", alias.instr_s);
        
        instr_id = alias.instr_id;
    }
    else{
        if(!instr.instr_s)
            return 1;

        concat(&DECODE_STR(out), "%s ", instr.instr_s);

        instr_id = instr.instr_id;
    }

    if(instr_id == AD_INSTR_LDAPR || instr_id == AD_INSTR_LDAPRB || 
            instr_id == AD_INSTR_LDAPRH){
        ADD_REG_OPERAND(out, Rt, sz, PREFER_ZR, _SYSREG(AD_NONE), registers);

        concat(&DECODE_STR(out), "%s, ", Rt_s);
    }
    else{
        ADD_REG_OPERAND(out, Rs, sz, PREFER_ZR, _SYSREG(AD_NONE), registers);

        concat(&DECODE_STR(out), "%s, ", Rs_s);

        /* alias omits Rt */
        if(!use_alias){
            ADD_REG_OPERAND(out, Rt, sz, PREFER_ZR, _SYSREG(AD_NONE), registers);

            concat(&DECODE_STR(out), "%s, ", Rt_s);
        }
    }

    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), registers);

    concat(&DECODE_STR(out), "[%s]", Rn_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleLoadAndStoreRegisterOffsetInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 opc = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 option = bits(i->opcode, 13, 15);
    U32 S = bits(i->opcode, 12, 12);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    ADD_FIELD(out, size);
    ADD_FIELD(out, V);
    ADD_FIELD(out, opc);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, option);
    ADD_FIELD(out, S);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    const char **registers = AD_RTBL_GEN_32;
    U64 sz = _32_BIT;
    
    S32 fp = 0;

    if(V == 0 && (opc == 2 || size == 3)){
        registers = AD_RTBL_GEN_64;
        sz = _64_BIT;
    }
    else if(V == 1){
        fp = 1;

        if(size == 0 && opc != 2){
            registers = AD_RTBL_FP_8;
            sz = _8_BIT;
        }
        else if(size == 0 && (opc == 2 || opc == 3)){
            registers = AD_RTBL_FP_128;
            sz = _128_BIT;
        }
        else if(size == 1){
            registers = AD_RTBL_FP_16;
            sz = _16_BIT;
        }
        else if(size == 2){
            registers = AD_RTBL_FP_32;
            sz = _32_BIT;
        }
        else if(size == 3){
            registers = AD_RTBL_FP_64;
            sz = _64_BIT;
        }
    }

    const char *Rt_s = NULL;
    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);
    const char *Rm_s = NULL;

    if(fp)
        Rt_s = GET_FP_REG(registers, Rt);
    else
        Rt_s = GET_GEN_REG(registers, Rt, NO_PREFER_ZR);

    S32 Rm_sz = _64_BIT;

    if(option & 1)
        Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, PREFER_ZR);
    else{
        Rm_s = GET_GEN_REG(AD_RTBL_GEN_32, Rm, PREFER_ZR);
        Rm_sz = _32_BIT;
    }

    U32 instr_idx = (size << 3) | (V << 2) |  opc;

    if(OOB(instr_idx, pre_post_unsigned_register_idx_instr_tbl))
        return 1;

    struct itab instr = pre_post_unsigned_register_idx_instr_tbl[instr_idx];

    instr_id = instr.instr_id;

    SET_INSTR_ID(out, instr_id);

    if(instr_id != AD_INSTR_PRFM)
        ADD_REG_OPERAND(out, Rt, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), registers);

    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), AD_RTBL_GEN_64);
    ADD_REG_OPERAND(out, Rm, Rm_sz, PREFER_ZR, _SYSREG(AD_NONE), Rm_sz == _32_BIT ?
            AD_RTBL_GEN_32 : AD_RTBL_GEN_64);

    S32 extended = option != 3;
    const char *extend = decode_reg_extend(option);

    if(instr_id == AD_INSTR_PRFM){
        U32 type = bits(Rt, 3, 4);
        U32 target = bits(Rt, 1, 2);
        U32 policy = Rt & 1;

        const char *types[] = { "PLD", "PLI", "PST" };
        const char *targets[] = { "L1", "L2", "L3" };
        const char *policies[] = { "KEEP", "STRM" };

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&Rt);

        if(OOB(type, types) || OOB(target, targets) || OOB(policy, policies))
            concat(&DECODE_STR(out), "%s #%#x, [%s, %s", instr.instr_s, Rt, Rn_s, Rm_s);
        else{
            concat(&DECODE_STR(out), "%s %s%s%s, [%s, %s", instr.instr_s, types[type],
                    targets[target], policies[policy], Rn_s, Rm_s);
        }

        if(option == 3 && !S)
            concat(&DECODE_STR(out), "]");
        else{
            if(option == 3)
                concat(&DECODE_STR(out), ", lsl");
            else
                concat(&DECODE_STR(out), ", %s", extend);

            if(S){
                ADD_IMM_OPERAND(out, AD_IMM_UINT, 3);
                
                concat(&DECODE_STR(out), " #3");
            }

            concat(&DECODE_STR(out), "]");
        }

        return 0;
    }

    if(!instr.instr_s)
        return 1;

    concat(&DECODE_STR(out), "%s %s, [%s, %s", instr.instr_s, Rt_s, Rn_s, Rm_s);

    if(V == 0){
        S32 amount = 0;

        if(instr_id == AD_INSTR_STRB || instr_id == AD_INSTR_LDRB ||
                instr_id == AD_INSTR_LDRSB){
            if(S == 0){
                if(extended)
                    concat(&DECODE_STR(out), ", %s", extend);
            }
            else{
                if(extended){
                    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&S);
                    concat(&DECODE_STR(out), ", %s #%d", extend, S);
                }
                else{
                    ADD_IMM_OPERAND(out, AD_IMM_UINT, 0);
                    concat(&DECODE_STR(out), ", lsl #0");
                }
            }

            concat(&DECODE_STR(out), "]");

            return 0;
        }
        else if(instr_id == AD_INSTR_STR || instr_id == AD_INSTR_LDR){
            if(sz == _64_BIT)
                amount = S == 0 ? 0 : 3;
            else
                amount = S == 0 ? 0 : 2;
        }
        else if(instr_id == AD_INSTR_LDRSW){
            amount = S == 0 ? 0 : 2;
        }

        if(extended){
            concat(&DECODE_STR(out), ", %s", extend);

            if(amount != 0){
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&amount);

                concat(&DECODE_STR(out), " #%d", amount);
            }

            concat(&DECODE_STR(out), "]");
        }
        else{
            if(amount != 0){
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&amount);

                concat(&DECODE_STR(out), ", lsl #%d", amount);
            }

            concat(&DECODE_STR(out), "]");
        }
    }
    else{
        /* shift amount for 128 bit */
        S32 amount = 4;

        if(registers == AD_RTBL_FP_8)
            amount = S;
        else if(registers == AD_RTBL_FP_16)
            amount = S == 0 ? 0 : 1;
        else if(registers == AD_RTBL_FP_32)
            amount = S == 0 ? 0 : 2;
        else if(registers == AD_RTBL_FP_64)
            amount = S == 0 ? 0 : 3;

        if(registers == AD_RTBL_FP_8){
            if(S == 0){
                if(extended)
                    concat(&DECODE_STR(out), ", %s", extend);
            }
            else{
                if(extended){
                    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&amount);
                    concat(&DECODE_STR(out), ", %s #%d", extend, amount);
                }
                else{
                    ADD_IMM_OPERAND(out, AD_IMM_UINT, 0);
                    concat(&DECODE_STR(out), ", lsl #0");
                }
            }

            concat(&DECODE_STR(out), "]");

            return 0;
        }

        if(extended){
            concat(&DECODE_STR(out), ", %s", extend);

            if(amount != 0){
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&amount);

                concat(&DECODE_STR(out), " #%d", amount);
            }

            concat(&DECODE_STR(out), "]");
        }
        else{
            if(amount != 0){
                ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&amount);

                concat(&DECODE_STR(out), ", lsl #%d", amount);
            }

            concat(&DECODE_STR(out), "]");
        }
    }

    return 0;
}

static S32 DisassembleLoadAndStorePACInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 30, 31);
    U32 V = bits(i->opcode, 26, 26);
    U32 M = bits(i->opcode, 23, 23);
    U32 S = bits(i->opcode, 22, 22);
    U32 imm9 = bits(i->opcode, 12, 20);
    U32 W = bits(i->opcode, 11, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rt = bits(i->opcode, 0, 4);

    if(size != 3)
        return 1;

    if(size == 3 && V == 1)
        return 1;

    ADD_FIELD(out, size);
    ADD_FIELD(out, V);
    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, imm9);
    ADD_FIELD(out, W);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);
    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

    ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), AD_RTBL_GEN_64);
    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), AD_RTBL_GEN_64);

    S32 use_key_A = M == 0;
    S32 simm = sign_extend((S << 9) | imm9, 10) << 3;

    concat(&DECODE_STR(out), "ldr");

    if(M == 0){
        instr_id = AD_INSTR_LDRAA;
        concat(&DECODE_STR(out), "aa");
    }
    else{
        instr_id = AD_INSTR_LDRAB;
        concat(&DECODE_STR(out), "ab");
    }

    concat(&DECODE_STR(out), " %s, [%s", Rt_s, Rn_s);

    if(simm == 0)
        concat(&DECODE_STR(out), "]");
    else{
        ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&simm);

        concat(&DECODE_STR(out), ", #"S_X"]", S_A(simm));

        if(W == 1)
            concat(&DECODE_STR(out), "!");
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

S32 LoadsAndStoresDisassemble(struct instruction *i, struct ad_insn *out){
    S32 result = 0;

    S32 post_pre = 0;
    U32 op0 = bits(i->opcode, 28, 31);
    U32 op1 = bits(i->opcode, 26, 26);
    U32 op2 = bits(i->opcode, 23, 24);
    U32 op3 = bits(i->opcode, 16, 21);
    U32 op4 = bits(i->opcode, 10, 11);

    if((op0 & ~4) == 0 && op1 == 1 && (op2 == 0 || op2 == 1) && (op3 & ~0x1f) == 0)
    {
        result = DisassembleLoadStoreMultStructuresInstr(i, out, op2);
        post_pre = (op2 != 0) ? POST_INDEXED : 0;
    }
    else if((op0 & ~4) == 0 && op1 == 1 && (op2 == 2 || op2 == 3))
    {
        result = DisassembleLoadStoreSingleStructuresInstr(i, out, op2 != 2);
        post_pre = (op2 != 2) ? POST_INDEXED : 0;
    }
    else if(op0 == 13 && op1 == 0 && (op2 >> 1) == 1 && (op3 >> 5) == 1)
    {
        result = DisassembleLoadStoreMemoryTagsInstr(i, out);
    }
    else if((op0 & ~12) == 0 && op1 == 0 && (op2 >> 1) == 0)
    {
        result = DisassembleLoadAndStoreExclusiveInstr(i, out);
    }
    else if((op0 & ~12) == 1 && op1 == 0 && (op2 >> 1) == 1 && (op3 & ~0x1f) == 0 && op4 == 0)
    {
        result = DisassembleLDAPR_STLRInstr(i, out);
    }
    else if((op0 & ~12) == 1 && (op2 >> 1) == 0)
    {
        result = DisassembleLoadAndStoreLiteralInstr(i, out);
    }
    else if((op0 & ~12) == 2 && op2 < 4)
    {
        result = DisassembleLoadAndStoreRegisterPairInstr(i, out, op2);
        post_pre = op2;
    }
    else if((op0 & ~12) == 3 && (op2 >> 1) == 0){
        if((op3 & ~0x1f) == 0)
        {
            result = DisassembleLoadAndStoreRegisterInstr(i, out, op4);
            post_pre = op4;
        }
        else{
            if(op4 == 0)
                result = DisassembleAtomicMemoryInstr(i, out);
            else if(op4 == 2)
                result = DisassembleLoadAndStoreRegisterOffsetInstr(i, out);
            else if((op4 & 1) == 1)
                result = DisassembleLoadAndStorePACInstr(i, out);
            else
                result = 1;
        }
    }
    else if((op0 & ~12) == 3 && (op2 >> 1) == 1)
        result = DisassembleLoadAndStoreRegisterInstr(i, out, UNSIGNED_IMMEDIATE);
    else
        result = 1;

    out->post_pre_index = post_pre;

    return result;
}
