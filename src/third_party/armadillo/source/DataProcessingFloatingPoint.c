#include <stdio.h>
#include <stdlib.h>

#include "adefs.h"
#include "bits.h"
#include "common.h"
#include "instruction.h"
#include "utils.h"
#include "strext.h"

static S32 DisassembleCryptographicAESInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 12, 16);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(size != 0)
        return 1;

    ADD_FIELD(out, size);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "aese", AD_INSTR_AESE }, { "aesd", AD_INSTR_AESD },
        { "aesmc", AD_INSTR_AESMC }, { "aesimc", AD_INSTR_AESIMC }
    };

    opcode -= 4;

    if(OOB(opcode, tab))
        return 1;

    const char *instr_s = tab[opcode].instr_s;
    S32 instr_id = tab[opcode].instr_id;

    const char **registers = AD_RTBL_FP_V_128;
    S32 sz = _128_BIT;

    ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

    const char *Rd_s = GET_FP_REG(registers, Rd);
    const char *Rn_s = GET_FP_REG(registers, Rn);

    concat(&DECODE_STR(out), "%s %s.16b, %s.16b", instr_s, Rd_s, Rn_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCryptographicThreeRegisterSHAInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 12, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(size != 0)
        return 1;

    ADD_FIELD(out, size);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "sha1c", AD_INSTR_SHA1C }, { "sha1p", AD_INSTR_SHA1P },
        { "sha1m", AD_INSTR_SHA1M }, { "sha1su0", AD_INSTR_SHA1SU0 },
        { "sha256h", AD_INSTR_SHA256H }, { "sha256h2", AD_INSTR_SHA256H2 },
        { "sha256su1", AD_INSTR_SHA256SU1 }
    };

    if(OOB(opcode, tab))
        return 1;

    const char *instr_s = tab[opcode].instr_s;
    S32 instr_id = tab[opcode].instr_id;

    concat(&DECODE_STR(out), "%s", instr_s);

    if(instr_id != AD_INSTR_SHA1SU0 && instr_id != AD_INSTR_SHA256SU1){
        const char *Rd_s = GET_FP_REG(AD_RTBL_FP_128, Rd);
        ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_128));

        concat(&DECODE_STR(out), " %s", Rd_s);

        const char *Rn_s = NULL;

        if(instr_id == AD_INSTR_SHA256H || instr_id == AD_INSTR_SHA256H2){
            Rn_s = GET_FP_REG(AD_RTBL_FP_128, Rn);
            ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_FP_128));

            concat(&DECODE_STR(out), ", %s", Rn_s);
        }
        else{
            Rn_s = GET_FP_REG(AD_RTBL_FP_32, Rn);
            ADD_REG_OPERAND(out, Rn, _SZ(_32_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_FP_32));

            concat(&DECODE_STR(out), ", %s", Rn_s);
        }
    }
    else{
        const char *Rd_s = GET_FP_REG(AD_RTBL_FP_V_128, Rd);
        ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

        const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);
        ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

        concat(&DECODE_STR(out), " %s.4s, %s.4s", Rd_s, Rn_s);
    }

    const char *Rm_s = GET_FP_REG(AD_RTBL_FP_V_128, Rm);
    ADD_REG_OPERAND(out, Rm, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    concat(&DECODE_STR(out), ", %s.4s", Rm_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCryptographicTwoRegisterSHAInstr(struct instruction *i,
        struct ad_insn *out){
    U32 size = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 12, 16);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(size != 0)
        return 1;

    ADD_FIELD(out, size);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "sha1h", AD_INSTR_SHA1H }, { "sha1su1", AD_INSTR_SHA1SU1 },
        { "sha256su0", AD_INSTR_SHA256SU0 }
    };

    if(OOB(opcode, tab))
        return 1;

    const char *instr_s = tab[opcode].instr_s;
    S32 instr_id = tab[opcode].instr_id;

    concat(&DECODE_STR(out), "%s", instr_s);

    if(instr_id == AD_INSTR_SHA1H){
        const char *Rd_s = GET_FP_REG(AD_RTBL_FP_32, Rd);
        const char *Rn_s = GET_FP_REG(AD_RTBL_FP_32, Rn);

        ADD_REG_OPERAND(out, Rd, _SZ(_32_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_FP_32));
        ADD_REG_OPERAND(out, Rn, _SZ(_32_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_FP_32));

        concat(&DECODE_STR(out), " %s, %s", Rd_s, Rn_s);
    }
    else{
        const char *Rd_s = GET_FP_REG(AD_RTBL_FP_V_128, Rd);
        const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);

        ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_FP_V_128));
        ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_FP_V_128));

        concat(&DECODE_STR(out), " %s.4s, %s.4s", Rd_s, Rn_s);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDCopyInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar){
    U32 Q = bits(i->opcode, 30, 30);
    U32 op = bits(i->opcode, 29, 29);
    U32 imm5 = bits(i->opcode, 16, 20);
    U32 imm4 = bits(i->opcode, 11, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if((imm5 & ~0x10) == 0)
        return 1;

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, op);
    ADD_FIELD(out, imm5);
    ADD_FIELD(out, imm4);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;
    S32 unaliased_instr_id = AD_NONE;

    S32 size = LowestSetBit(imm5, 5);

    if(op == 0 && (imm4 == 0 || imm4 == 1)){
        /* MOV alias for scalar DUP is always preferred */
        instr_s = scalar ? "mov" : "dup";
        instr_id = scalar ? AD_INSTR_MOV : AD_INSTR_DUP;

        unaliased_instr_id = AD_INSTR_DUP;
    }
    else if(op == 0 && imm4 == 5){
        instr_s = "smov";
        instr_id = AD_INSTR_SMOV;

        unaliased_instr_id = AD_INSTR_SMOV;
    }
    else if(op == 0 && imm4 == 7){
        S32 umov_alias = (size == 3 || size == 2);

        instr_s = umov_alias ? "mov" : "umov";
        instr_id = umov_alias ? AD_INSTR_MOV : AD_INSTR_UMOV;

        unaliased_instr_id = AD_INSTR_UMOV;
    }
    else if(Q == 1 && ((op == 0 && imm4 == 3) || op == 1)){
        /* MOV alias for INS (general) and INS (element) is always preferred */
        instr_s = "mov";
        instr_id = AD_INSTR_MOV;

        unaliased_instr_id = AD_INSTR_INS;
    }

    if(!instr_s)
        return 1;

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;

    U32 Rd_sz = 0;
    U32 Rn_sz = 0;

    const char *T = NULL;
    const char *Ts = NULL;

    const char **rtbls[] = {
        AD_RTBL_FP_8, AD_RTBL_FP_16, AD_RTBL_FP_32, AD_RTBL_FP_64
    };

    U32 sizes[] = {
        _8_BIT, _16_BIT, _32_BIT, _64_BIT
    };

    const char *sizes_s[] = {
        "b", "h", "s", "d"
    };

    if(unaliased_instr_id == AD_INSTR_DUP){
        if(scalar){
            if(size == -1 || size > 3)
                return 1;

            Rd_Rtbl = rtbls[size];
            Rd_sz = sizes[size];

            T = sizes_s[size];
        }
        else{
            Rd_Rtbl = AD_RTBL_FP_V_128;
            Rd_sz = _128_BIT;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!T)
                return 1;
        }

        if(imm4 == 0){
            /* DUP (element) */
            Rn_Rtbl = AD_RTBL_FP_V_128;
            Rn_sz = _128_BIT;

            Ts = sizes_s[size];
        }
        else{
            /* DUP (general) */
            if(size == -1 || size > 3)
                return 1;

            Rn_Rtbl = size <= 2 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
            Rn_sz = size <= 2 ? _32_BIT : _64_BIT;
        }
    }
    else if(unaliased_instr_id == AD_INSTR_SMOV || unaliased_instr_id == AD_INSTR_UMOV){
        Rd_Rtbl = Q == 0 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
        Rn_Rtbl = AD_RTBL_FP_V_128;

        Rd_sz = Q == 0 ? _32_BIT : _64_BIT;
        Rn_sz = _128_BIT;

        if(unaliased_instr_id == AD_INSTR_SMOV){
            if((Q == 0 && size > 1) || (Q == 1 && size > 2))
                return 1;
        }
        else{
            if((Q == 0 && size > 2) || (Q == 1 && size != 3))
                return 1;
        }

        Ts = sizes_s[size];
    }
    else{
        Rd_Rtbl = AD_RTBL_FP_V_128;
        Rd_sz = _128_BIT;

        if(op == 1){
            /* INS (element) */
            Rn_Rtbl = AD_RTBL_FP_V_128;
            Rn_sz = _128_BIT;
        }
        else{
            /* INS (general) */
            if(size == -1 || size > 3)
                return 1;

            Rn_Rtbl = size <= 2 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
            Rn_sz = size <= 2 ? _32_BIT : _64_BIT;
        }

        Ts = sizes_s[size];
    }

    if(!Rd_Rtbl || !Rn_Rtbl)
        return 1;

    const char *Rd_s = NULL;
    S32 Rd_prefer_zr = 0;

    if(unaliased_instr_id != AD_INSTR_SMOV && unaliased_instr_id != AD_INSTR_UMOV)
        Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    else{
        Rd_s = GET_GEN_REG(Rd_Rtbl, Rd, PREFER_ZR);
        Rd_prefer_zr = 1;
    }

    const char *Rn_s = NULL;
    S32 Rn_prefer_zr = 0;

    U32 index = (imm5 >> (size + 1));
    U32 index2 = (imm4 >> size);

    /* DUP (general) or INS (general) */
    if((unaliased_instr_id == AD_INSTR_DUP && imm4 == 1) ||
            (unaliased_instr_id == AD_INSTR_INS && op == 0)){
        Rn_s = GET_GEN_REG(Rn_Rtbl, Rn, PREFER_ZR);
        Rn_prefer_zr = 1;
    }
    else{
        Rn_s = GET_FP_REG(Rn_Rtbl, Rn);
    }

    ADD_REG_OPERAND(out, Rd, Rd_sz, Rd_prefer_zr, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, Rn_sz, Rn_prefer_zr, _SYSREG(AD_NONE), Rn_Rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    /* DUP (element, vector) or DUP (general) */
    if(unaliased_instr_id == AD_INSTR_DUP && !scalar)
        concat(&DECODE_STR(out), ".%s", T);
    /* INS (element) or INS (general) */
    else if(unaliased_instr_id == AD_INSTR_INS){
        /* index == index1 for INS (element) */
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&index);
        concat(&DECODE_STR(out), ".%s[%d]", Ts, index);
    }

    concat(&DECODE_STR(out), ", %s", Rn_s);

    /* DUP (general) or INS (general) */
    if((unaliased_instr_id == AD_INSTR_DUP && imm4 == 1) || 
            (unaliased_instr_id == AD_INSTR_INS && op == 0)){
        /* in this case, we're done constructing the decode string */
        SET_INSTR_ID(out, instr_id);

        return 0;
    }

    /* DUP (element, scalar) */
    if(unaliased_instr_id == AD_INSTR_DUP && scalar)
        concat(&DECODE_STR(out), ".%s", T);
    else
        concat(&DECODE_STR(out), ".%s", Ts);

    /* INS (element) */
    if(unaliased_instr_id == AD_INSTR_INS && op == 1){
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&index2);
        concat(&DECODE_STR(out), "[%d]", index2);
    }
    else{
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&index);
        concat(&DECODE_STR(out), "[%d]", index);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

/* This function takes care of:
 *      - Advanced SIMD scalar three same FP16
 *      - Advanced SIMD scalar three same extra
 *      - Advanced SIMD scalar three same
 *      - Advanced SIMD three same (FP16)
 *      - Advanced SIMD three same extra
 *      - Advanced SIMD three same
 */
static S32 DisassembleAdvancedSIMDThreeSameInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar, S32 fp16, S32 extra){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 a = bits(i->opcode, 23, 23);
    U32 size = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    
    U32 opcode = 0;

    if(fp16)
        opcode = bits(i->opcode, 11, 13);
    else if(extra)
        opcode = bits(i->opcode, 11, 14);
    else
        opcode = bits(i->opcode, 11, 15);

    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, U);

    if(fp16)
        ADD_FIELD(out, a);
    else
        ADD_FIELD(out, size);

    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    /* not a part of all instrs, don't include in field array */
    U32 rot = bits(i->opcode, 11, 12);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(fp16){
        const char **rtbl = AD_RTBL_FP_V_128;
        U32 sz = _128_BIT;

        if(scalar){
            rtbl = AD_RTBL_FP_16;
            sz = _16_BIT;
        }

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);
        const char *Rm_s = GET_FP_REG(rtbl, Rm);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        U32 idx = (U << 4) | (a << 3) | opcode;

        if(scalar){
            struct itab tab[] = {
                /* three blanks, idxes [0-2] */
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { "fmulx", AD_INSTR_FMULX }, { "fcmeq", AD_INSTR_FCMEQ },
                /* two blanks, idxes [5-6] */
                { NULL, AD_NONE }, { NULL, AD_NONE },
                { "frecps", AD_INSTR_FRECPS },
                /* seven blanks, idxes [8-14] */
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { NULL, AD_NONE },
                { "frsqrts", AD_INSTR_FRSQRTS },
                /* four blanks, idxes [16-19] */
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { NULL, AD_NONE },
                { "fcmge", AD_INSTR_FCMGE }, { "facge", AD_INSTR_FACGE },
                /* four blanks, idxes [22-25] */
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { NULL, AD_NONE },
                { "fabd", AD_INSTR_FABD },
                /* one blank, idx 27 */
                { NULL, AD_NONE },
                { "fcmgt", AD_INSTR_FCMGT }, { "facgt", AD_INSTR_FACGT }
            };

            if(OOB(idx, tab))
                return 1;

            instr_s = tab[idx].instr_s;

            if(!instr_s)
                return 1;

            instr_id = tab[idx].instr_id;

            concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);
        }
        else{
            struct itab tab[] = {
                { "fmaxnm", AD_INSTR_FMAXNM }, { "fmla", AD_INSTR_FMLA },
                { "fadd", AD_INSTR_FADD }, { "fmulx", AD_INSTR_FMULX },
                { "fcmeq", AD_INSTR_FCMEQ },
                /* one blank, idx 5 */
                { NULL, AD_NONE },
                { "fmax", AD_INSTR_FMAX }, { "frecps", AD_INSTR_FRECPS },
                { "fminnm", AD_INSTR_FMINNM }, { "fmls", AD_INSTR_FMLS },
                { "fsub", AD_INSTR_FSUB },
                /* three blanks, idxes [11-13] */
                { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE },
                { "fmin", AD_INSTR_FMIN }, { "frsqrts", AD_INSTR_FRSQRTS },
                { "fmaxnmp", AD_INSTR_FMAXNMP },
                /* one blank, idx 17 */
                { NULL, AD_NONE },
                { "faddp", AD_INSTR_FADDP }, { "fmul", AD_INSTR_FMUL },
                { "fcmge", AD_INSTR_FCMGE }, { "facge", AD_INSTR_FACGE },
                { "fmaxp", AD_INSTR_FMAXP }, { "fdiv", AD_INSTR_FDIV },
                { "fminnmp", AD_INSTR_FMINNMP },
                /* one blank, idx 25 */
                { NULL, AD_NONE },
                { "fabd", AD_INSTR_FABD },
                /* one blank, idx 27 */
                { NULL, AD_NONE },
                { "fcmgt", AD_INSTR_FCMGT }, { "fminp", AD_INSTR_FMINP }
            };

            if(OOB(idx, tab))
                return 1;

            instr_s = tab[idx].instr_s;

            if(!instr_s)
                return 1;

            instr_id = tab[idx].instr_id;

            const char *arrangement = Q == 0 ? "4h" : "8h";

            concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s, Rd_s,
                    arrangement, Rn_s, arrangement, Rm_s, arrangement);
        }
    }
    else if(extra){
        if(scalar){
            if(U != 1)
                return 1;

            if(size == 0 || size == 3)
                return 1;

            instr_s = opcode == 0 ? "sqrdmlah" : "sqrdmlsh";
            instr_id = opcode == 0 ? AD_INSTR_SQRDMLAH : AD_INSTR_SQRDMLSH;

            const char **rtbl = AD_RTBL_FP_16;
            U32 sz = _16_BIT;

            if(size == 2){
                rtbl = AD_RTBL_FP_32;
                sz = _32_BIT;
            }

            const char *Rd_s = GET_FP_REG(rtbl, Rd);
            const char *Rn_s = GET_FP_REG(rtbl, Rn);
            const char *Rm_s = GET_FP_REG(rtbl, Rm);

            ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

            concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);
        }
        else{
            const char **rtbl = AD_RTBL_FP_V_128;
            U32 sz = _128_BIT;

            const char *Rd_s = GET_FP_REG(rtbl, Rd);
            const char *Rn_s = GET_FP_REG(rtbl, Rn);
            const char *Rm_s = GET_FP_REG(rtbl, Rm);

            ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

            if(opcode == 2){
                instr_s = U == 0 ? "sdot" : "udot";
                instr_id = U == 0 ? AD_INSTR_SDOT : AD_INSTR_UDOT;

                const char *Ta = Q == 0 ? "2s" : "4s";
                const char *Tb = Q == 0 ? "8b" : "16b";

                concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s,
                        Rd_s, Ta, Rn_s, Tb, Rm_s, Tb);
            }
            else{
                if(opcode < 2){
                    instr_s = opcode == 0 ? "sqrdmlah" : "sqrdmlsh";
                    instr_id = opcode == 0 ? AD_INSTR_SQRDMLAH : AD_INSTR_SQRDMLSH;
                }
                else{
                    if((opcode & ~3) == 8){
                        instr_s = "fcmla";
                        instr_id = AD_INSTR_FCMLA;
                    }
                    else if((opcode & ~2) == 12){
                        instr_s = "fcadd";
                        instr_id = AD_INSTR_FCADD;
                    }
                    else{
                        return 1;
                    }
                }

                const char *arrangement = NULL;

                if(size == 1)
                    arrangement = Q == 0 ? "4h" : "8h";
                else if(size == 2)
                    arrangement = Q == 0 ? "2s" : "4s";
                else if((instr_id == AD_INSTR_FCMLA || instr_id == AD_INSTR_FCADD) &&
                        size == 3 && Q == 1){
                    arrangement = "2d";
                }

                if(!arrangement)
                    return 1;

                concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s,
                        Rd_s, arrangement, Rn_s, arrangement, Rm_s, arrangement);

                if(instr_id == AD_INSTR_FCMLA || instr_id == AD_INSTR_FCADD){
                    U32 rotate = 0;

                    if(instr_id == AD_INSTR_FCMLA)
                        rotate = rot * 90;
                    else
                        rotate = rot == 0 ? 90 : 270;

                    concat(&DECODE_STR(out), ", #%d", rotate);

                    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&rotate);
                }
            }
        }
    }
    else{
        const char **rtbls[] = {
            AD_RTBL_FP_8, AD_RTBL_FP_16, AD_RTBL_FP_32, AD_RTBL_FP_64
        };
        
        U32 sizes[] = {
            _8_BIT, _16_BIT, _32_BIT, _64_BIT
        };

        const char **rtbl = NULL;
        const char *T = NULL;

        U32 sz = 0;
        
        if(opcode == 0){
            if(scalar)
                return 1;

            instr_s = "shadd";
            instr_id = AD_INSTR_SHADD;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 1){
            instr_s = U == 0 ? "sqadd" : "uqadd";
            instr_id = U == 0 ? AD_INSTR_SQADD : AD_INSTR_UQADD;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;
            
            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 2){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "srhadd" : "urhadd";
            instr_id = U == 0 ? AD_INSTR_SRHADD : AD_INSTR_URHADD;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 3){
            if(scalar)
                return 1;

            struct itab u0[] = {
                { "and", AD_INSTR_AND }, { "bic", AD_INSTR_BIC },
                { "orr", AD_INSTR_ORR }, { "orn", AD_INSTR_ORN }
            };

            struct itab u1[] = {
                { "eor", AD_INSTR_EOR }, { "bsl", AD_INSTR_BSL },
                { "bit", AD_INSTR_BIT }, { "bif", AD_INSTR_BIF }
            };

            /* both table sizes are the same, don't need to check u1 */
            if(OOB(size, u0))
                return 1;

            instr_s = U == 0 ? u0[size].instr_s : u1[size].instr_s;
            instr_id = U == 0 ? u0[size].instr_id : u1[size].instr_id;

            T = Q == 0 ? "8b" : "16b";

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 4){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "shsub" : "uhsub";
            instr_id = U == 0 ? AD_INSTR_SHSUB : AD_INSTR_UHSUB;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 5){
            instr_s = U == 0 ? "sqsub" : "uqsub";
            instr_id = U == 0 ? AD_INSTR_SQSUB : AD_INSTR_UQSUB;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 6){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "cmgt" : "cmhi";
            instr_id = U == 0 ? AD_INSTR_CMGT : AD_INSTR_CMHI;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 7){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "cmge" : "cmhs";
            instr_id = U == 0 ? AD_INSTR_CMGE : AD_INSTR_CMHS;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 8){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "sshl" : "ushl";
            instr_id = U == 0 ? AD_INSTR_SSHL : AD_INSTR_USHL;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 9){
            instr_s = U == 0 ? "sqshl" : "uqshl";
            instr_id = U == 0 ? AD_INSTR_SQSHL : AD_INSTR_UQSHL;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0xa){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "srshl" : "urshl";
            instr_id = U == 0 ? AD_INSTR_SRSHL : AD_INSTR_URSHL;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0xb){
            instr_s = U == 0 ? "sqrshl" : "uqrshl";
            instr_id = U == 0 ? AD_INSTR_SQRSHL : AD_INSTR_UQRSHL;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0xc){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "smax" : "umax";
            instr_id = U == 0 ? AD_INSTR_SMAX : AD_INSTR_UMAX;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0xd){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "smin" : "umin";
            instr_id = U == 0 ? AD_INSTR_SMIN : AD_INSTR_UMIN;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0xe){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "sabd" : "uabd";
            instr_id = U == 0 ? AD_INSTR_SABD : AD_INSTR_UABD;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0xf){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "saba" : "uaba";
            instr_id = U == 0 ? AD_INSTR_SABA : AD_INSTR_UABA;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x10){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "add" : "sub";
            instr_id = U == 0 ? AD_INSTR_ADD : AD_INSTR_SUB;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0x11){
            if(scalar && size != 3)
                return 1;

            instr_s = U == 0 ? "cmtst" : "cmeq";
            instr_id = U == 0 ? AD_INSTR_CMTST : AD_INSTR_CMEQ;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0x12){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "mla" : "mls";
            instr_id = U == 0 ? AD_INSTR_MLA : AD_INSTR_MLS;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x13){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "mul" : "pmul";
            instr_id = U == 0 ? AD_INSTR_MUL : AD_INSTR_PMUL;

            if(instr_id == AD_INSTR_PMUL)
                T = Q == 0 ? "8b" : "16b";
            else{
                if(size == 0)
                    T = Q == 0 ? "8b" : "16b";
                else if(size == 1)
                    T = Q == 0 ? "4h" : "8h";
                else if(size == 2)
                    T = Q == 0 ? "2s" : "4s";
            }

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x14){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "smaxp" : "umaxp";
            instr_id = U == 0 ? AD_INSTR_SMAXP : AD_INSTR_UMAXP;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x15){
            if(scalar)
                return 1;

            instr_s = U == 0 ? "sminp" : "uminp";
            instr_id = U == 0 ? AD_INSTR_SMINP : AD_INSTR_UMINP;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x16){
            if(scalar && (size == 0 || size == 3))
                return 1;

            instr_s = U == 0 ? "sqdmulh" : "sqrdmulh";
            instr_id = U == 0 ? AD_INSTR_SQDMULH : AD_INSTR_SQRDMULH;

            if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";

            if(!scalar && !T)
                return 1;

            if(scalar){
                rtbl = rtbls[size];
                sz = sizes[size];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0x17){
            if(scalar)
                return 1;

            if(U == 1)
                return 1;

            instr_s = "addp";
            instr_id = AD_INSTR_ADDP;

            if(size == 0)
                T = Q == 0 ? "8b" : "16b";
            else if(size == 1)
                T = Q == 0 ? "4h" : "8h";
            else if(size == 2)
                T = Q == 0 ? "2s" : "4s";
            else if(size == 3 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x18){
            if(scalar)
                return 1;

            U32 s = size >> 1;

            if(U == 0){
                instr_s = s == 0 ? "fmaxnm" : "fminnm";
                instr_id = s == 0 ? AD_INSTR_FMAXNM : AD_INSTR_FMINNM;
            }
            else{
                instr_s = s == 0 ? "fmaxnmp" : "fminnmp";
                instr_id = s == 0 ? AD_INSTR_FMAXNMP : AD_INSTR_FMINNMP;
            }

            U32 _sz = (size & 1);

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x19){
            if(scalar)
                return 1;

            U32 s = size >> 1;

            if(U == 0){
                instr_s = s == 0 ? "fmla" : "fmls";
                instr_id = s == 0 ? AD_INSTR_FMLA : AD_INSTR_FMLS;
            }
            else{
                if(size == 1 || size == 3)
                    return 1;

                instr_s = size == 0 ? "fmlal2" : "fmlsl2";
                instr_id = size == 0 ? AD_INSTR_FMLAL2 : AD_INSTR_FMLSL2;

                const char *Ta = Q == 0 ? "2s" : "4s";
                const char *Tb = Q == 0 ? "2h" : "4h";

                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;

                const char *Rd_s = GET_FP_REG(rtbl, Rd);
                const char *Rn_s = GET_FP_REG(rtbl, Rn);
                const char *Rm_s = GET_FP_REG(rtbl, Rm);

                ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
                ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
                ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

                concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s, Rd_s,
                        Ta, Rn_s, Tb, Rm_s, Tb);

                SET_INSTR_ID(out, instr_id);

                return 0;
            }

            U32 _sz = (size & 1);

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x1a){
            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(scalar){
                if(U != 1 && s != 1)
                    return 1;

                instr_s = "fabd";
                instr_id = AD_INSTR_FABD;

                rtbl = rtbls[2 + _sz];
                sz = sizes[2 + _sz];
            }
            else{
                if(U == 0){
                    instr_s = s == 0 ? "fadd" : "fsub";
                    instr_id = s == 0 ? AD_INSTR_FADD : AD_INSTR_FSUB;
                }
                else{
                    instr_s = s == 0 ? "faddp" : "fabd";
                    instr_id = s == 0 ? AD_INSTR_FADDP : AD_INSTR_FABD;
                }

                if(_sz == 0)
                    T = Q == 0 ? "2s" : "4s";
                else if(_sz == 1 && Q == 1)
                    T = "2d";

                if(!T)
                    return 1;

                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0x1b){
            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(scalar){
                if(s == 1)
                    return 1;

                if(U == 1 && s == 0)
                    return 1;

                instr_s = "fmulx";
                instr_id = AD_INSTR_FMULX;

                rtbl = rtbls[2 + _sz];
                sz = sizes[2 + _sz];
            }
            else{
                if(s == 1)
                    return 1;

                instr_s = U == 0 ? "fmulx" : "fmul";
                instr_id = U == 0 ? AD_INSTR_FMULX : AD_INSTR_FMUL;

                if(_sz == 0)
                    T = Q == 0 ? "2s" : "4s";
                else if(_sz == 1 && Q == 1)
                    T = "2d";

                if(!T)
                    return 1;

                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }
        }
        else if(opcode == 0x1c){
            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(U == 0 && s == 1)
                return 1;

            if(U == 0){
                instr_s = "fcmeq";
                instr_id = AD_INSTR_FCMEQ;
            }
            else{
                instr_s = s == 0 ? "fcmge" : "fcmgt";
                instr_id = s == 0 ? AD_INSTR_FCMGE : AD_INSTR_FCMGT;
            }

            if(scalar){
                rtbl = rtbls[2 + _sz];
                sz = sizes[2 + _sz];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;
        }
        else if(opcode == 0x1d){
            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(scalar && U == 0)
                return 1;

            if(U == 1){
                instr_s = s == 0 ? "facge" : "facgt";
                instr_id = s == 0 ? AD_INSTR_FACGE : AD_INSTR_FACGT;
            }
            else{
                if(size == 1 || size == 3)
                    return 1;

                instr_s = size == 0 ? "fmlal" : "fmlsl";
                instr_id = size == 0 ? AD_INSTR_FMLAL : AD_INSTR_FMLSL;

                const char *Ta = Q == 0 ? "2s" : "4s";
                const char *Tb = Q == 0 ? "2h" : "4h";

                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;

                const char *Rd_s = GET_FP_REG(rtbl, Rd);
                const char *Rn_s = GET_FP_REG(rtbl, Rn);
                const char *Rm_s = GET_FP_REG(rtbl, Rm);

                ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
                ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
                ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

                concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s, Rd_s,
                        Ta, Rn_s, Tb, Rm_s, Tb);

                SET_INSTR_ID(out, instr_id);

                return 0;
            }
            
            if(scalar){
                rtbl = rtbls[2 + _sz];
                sz = sizes[2 + _sz];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!scalar && !T)
                return 1;
        }
        else if(opcode == 0x1e){
            if(scalar)
                return 1;

            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(U == 0){
                instr_s = s == 0 ? "fmax" : "fmin";
                instr_id = s == 0 ? AD_INSTR_FMAX : AD_INSTR_FMIN;
            }
            else{
                instr_s = s == 0 ? "fmaxp" : "fminp";
                instr_id = s == 0 ? AD_INSTR_FMAXP : AD_INSTR_FMINP;
            }

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else if(opcode == 0x1f){
            U32 s = size >> 1;
            U32 _sz = (size & 1);

            if(scalar && U != 0)
                return 1;

            if(U == 0){
                instr_s = s == 0 ? "frecps" : "frsqrts";
                instr_id = s == 0 ? AD_INSTR_FRECPS : AD_INSTR_FRSQRTS;
            }
            else{
                if(s == 1)
                    return 1;

                instr_s = "fdiv";
                instr_id = AD_INSTR_FDIV;
            }

            if(scalar){
                rtbl = rtbls[2 + _sz];
                sz = sizes[2 + _sz];
            }
            else{
                rtbl = AD_RTBL_FP_V_128;
                sz = _128_BIT;
            }

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;
        }

        if(!rtbl || !instr_s)
            return 1;

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);
        const char *Rm_s = GET_FP_REG(rtbl, Rm);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

        if(scalar)
            concat(&DECODE_STR(out), ", %s, %s", Rn_s, Rm_s);
        else
            concat(&DECODE_STR(out), ".%s, %s.%s, %s.%s", T, Rn_s, T, Rm_s, T);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

/* This function takes care of:
 *      - Advanced SIMD scalar two-register miscellaneous FP16
 *      - Advanced SIMD scalar two-register miscellaneous
 *      - Advanced SIMD two-register miscellaneous (FP16)
 *      - Advanced SIMD two-register miscellaneous
 */
static S32 DisassembleAdvancedSIMDTwoRegisterMiscellaneousInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar, S32 fp16){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 a = bits(i->opcode, 23, 23);
    U32 size = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 12, 16);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, U);

    if(fp16)
        ADD_FIELD(out, a);
    else
        ADD_FIELD(out, size);

    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char **rtbls[] = {
        AD_RTBL_FP_8, AD_RTBL_FP_16, AD_RTBL_FP_32, AD_RTBL_FP_64
    };

    U32 sizes[] = {
        _8_BIT, _16_BIT, _32_BIT, _64_BIT
    };

    const char **rtbl = NULL;
    const char *T = NULL;

    U32 sz = 0;

    S32 add_zero = 0;
    S32 add_zerof = 0;

    if(opcode < 2){
        if(scalar || fp16)
            return 1;

        if(opcode == 1 && U == 1)
            return 1;

        U32 o0 = bits(i->opcode, 12, 12);
        U32 op = (o0 << 1) | U;

        struct itab tab[] = {
            { "rev64", AD_INSTR_REV64 }, { "rev32", AD_INSTR_REV32 },
            { "rev16", AD_INSTR_REV16 }
        };

        if(OOB(op, tab))
            return 1;

        instr_s = tab[op].instr_s;
        instr_id = tab[op].instr_id;

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2)
            T = Q == 0 ? "2s" : "4s";

        if(!T)
            return 1;

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;
    }
    else if(opcode == 2 || opcode == 6){
        if(scalar || fp16)
            return 1;

        if(opcode == 2){
            instr_s = U == 0 ? "saddlp" : "uaddlp";
            instr_id = U == 0 ? AD_INSTR_SADDLP : AD_INSTR_UADDLP;
        }
        else{
            instr_s = U == 0 ? "sadalp" : "uadalp";
            instr_id = U == 0 ? AD_INSTR_SADALP : AD_INSTR_UADALP;
        }

        const char *Ta = NULL;
        const char *Tb = NULL;

        if(size == 0){
            Ta = Q == 0 ? "4h" : "8h";
            Tb = Q == 0 ? "8b" : "16b";
        }
        else if(size == 1){
            Ta = Q == 0 ? "2s" : "4s";
            Tb = Q == 0 ? "4h" : "8h";
        }
        else if(size == 2){
            Ta = Q == 0 ? "1d" : "2d";
            Tb = Q == 0 ? "2s" : "4s";
        }

        if(!Ta || !Tb)
            return 1;

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        concat(&DECODE_STR(out), "%s %s.%s, %s.%s", instr_s, Rd_s, Ta, Rn_s, Tb);

        SET_INSTR_ID(out, instr_id);

        return 0;
    }
    else if(opcode == 3 || opcode == 7){
        if(fp16)
            return 1;

        if(opcode == 3){
            instr_s = U == 0 ? "suqadd" : "usqadd";
            instr_id = U == 0 ? AD_INSTR_SUQADD : AD_INSTR_USQADD;
        }
        else{
            instr_s = U == 0 ? "sqabs" : "sqneg";
            instr_id = U == 0 ? AD_INSTR_SQABS : AD_INSTR_SQNEG;
        }

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2)
            T = Q == 0 ? "2s" : "4s";
        else if(size == 3 && Q == 1)
            T = "2d";

        if(!scalar && !T)
            return 1;

        if(scalar){
            rtbl = rtbls[size];
            sz = sizes[size];
        }
        else{
            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
    }
    else if(opcode == 4){
        if(scalar || fp16)
            return 1;

        instr_s = U == 0 ? "cls" : "clz";
        instr_id = U == 0 ? AD_INSTR_CLS : AD_INSTR_CLZ;

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2)
            T = Q == 0 ? "2s" : "4s";
        else if(size == 3 && Q == 1)
            T = "2d";

        if(!T)
            return 1;

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;
    }
    else if(opcode == 5){
        if(scalar || fp16)
            return 1;

        if(U == 0){
            if(size != 0)
                return 1;

            instr_s = "cnt";
            instr_id = AD_INSTR_CNT;
        }
        else{
            if((size >> 1) == 1)
                return 1;
            
            instr_s = size == 0 ? "not" : "rbit";
            instr_id = size == 0 ? AD_INSTR_NOT : AD_INSTR_RBIT;
        }

        T = Q == 0 ? "8b" : "16b";

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;
    }
    else if(opcode >= 8 && opcode <= 0xa){
        if(fp16)
            return 1;

        if(opcode == 0xa && U == 1)
            return 1;

        if(scalar && size != 3)
            return 1;

        U32 op = bits(i->opcode, 12, 12);
        U32 cop = (op << 1) | U;

        struct itab tab[] = {
            { "cmgt", AD_INSTR_CMGT }, { "cmge", AD_INSTR_CMGE },
            { "cmeq", AD_INSTR_CMEQ }, { "cmle", AD_INSTR_CMLE }
        };

        instr_s = tab[cop].instr_s;
        instr_id = tab[cop].instr_id;

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2)
            T = Q == 0 ? "2s" : "4s";
        else if(size == 3 && Q == 1)
            T = "2d";

        if(!scalar && !T)
            return 1;

        if(scalar){
            rtbl = rtbls[size];
            sz = sizes[size];
        }
        else{
            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }

        add_zero = 1;
    }
    else if(opcode == 0xb){
        if(fp16)
            return 1;

        if(scalar && size != 3)
            return 1;

        instr_s = U == 0 ? "abs" : "neg";
        instr_id = U == 0 ? AD_INSTR_ABS : AD_INSTR_NEG;

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2)
            T = Q == 0 ? "2s" : "4s";
        else if(size == 3 && Q == 1)
            T = "2d";

        if(!scalar && !T)
            return 1;

        if(scalar){
            rtbl = rtbls[size];
            sz = sizes[size];
        }
        else{
            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
    }
    else if(opcode >= 0xc && opcode <= 0xe){
        if(opcode == 0xe && U == 1)
            return 1;

        U32 _sz = (size & 1);

        U32 op = bits(i->opcode, 12, 12);
        U32 cop = (op << 1) | U;

        struct itab tab[] = {
            { "fcmgt", AD_INSTR_FCMGT }, { "fcmge", AD_INSTR_FCMGE },
            { "fcmeq", AD_INSTR_FCMEQ }, { "fcmle", AD_INSTR_FCMLE }
        };

        instr_s = tab[cop].instr_s;
        instr_id = tab[cop].instr_id;

        if(scalar && fp16){
            T = Q == 0 ? "4h" : "8h";

            rtbl = AD_RTBL_FP_16;
            sz = _16_BIT;
        }
        else if(scalar){
            rtbl = rtbls[2 + _sz];
            sz = sizes[2 + _sz];
        }
        else if(fp16){
            T = Q == 0 ? "4h" : "8h";

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else{
            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }

        add_zerof = 1;
    }
    else if(opcode == 0xf){
        if(scalar)
            return 1;

        instr_s = U == 0 ? "fabs" : "fneg";
        instr_id = U == 0 ? AD_INSTR_FABS : AD_INSTR_FNEG;

        U32 _sz = (size & 1);

        if(fp16)
            T = Q == 0 ? "4h" : "8h";
        else{
            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;
        }

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;
    }
    else if(opcode == 0x12 || opcode == 0x14){
        if(fp16)
            return 1;

        if(scalar && size == 3)
            return 1;

        if(U == 0){
            if(opcode == 0x12){
                instr_s = Q == 0 ? "xtn" : "xtn2";
                instr_id = Q == 0 ? AD_INSTR_XTN : AD_INSTR_XTN2;
            }
            else{
                if(scalar){
                    instr_s = "sqxtn";
                    instr_id = AD_INSTR_SQXTN;
                }
                else{
                    instr_s = Q == 0 ? "sqxtn" : "sqxtn2";
                    instr_id = Q == 0 ? AD_INSTR_SQXTN : AD_INSTR_SQXTN2;
                }
            }
        }
        else{
            if(opcode == 0x12){
                if(scalar){
                    instr_s = "sqxtun";
                    instr_id = AD_INSTR_SQXTUN;
                }
                else{
                    instr_s = Q == 0 ? "sqxtun" : "sqxtun2";
                    instr_id = Q == 0 ? AD_INSTR_SQXTUN : AD_INSTR_SQXTUN2;
                }
            }
            else{
                if(scalar){
                    instr_s = "uqxtn";
                    instr_id = AD_INSTR_UQXTN;
                }
                else{
                    instr_s = Q == 0 ? "uqxtn" : "uqxtn2";
                    instr_id = Q == 0 ? AD_INSTR_UQXTN : AD_INSTR_UQXTN2;
                }
            }
        }

        concat(&DECODE_STR(out), "%s", instr_s);

        if(scalar){
            const char **Rd_rtbl = rtbls[size];
            const char **Rn_rtbl = rtbls[1 + size];

            U32 Rd_sz = sizes[size];
            U32 Rn_sz = sizes[1 + size];

            const char *Rd_s = GET_FP_REG(Rd_rtbl, Rd);
            const char *Rn_s = GET_FP_REG(Rn_rtbl, Rn);

            ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_rtbl);
            ADD_REG_OPERAND(out, Rn, Rn_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_rtbl);

            concat(&DECODE_STR(out), " %s, %s", Rd_s, Rn_s);
        }
        else{
            const char *Ta = NULL;
            const char *Tb = NULL;

            if(size == 0){
                Ta = "8h";
                Tb = Q == 0 ? "8b" : "16b";
            }
            else if(size == 1){
                Ta = "4s";
                Tb = Q == 0 ? "4h" : "8h";
            }
            else if(size == 2){
                Ta = "2d";
                Tb = Q == 0 ? "2s" : "4s";
            }

            if(!Ta || !Tb)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;

            const char *Rd_s = GET_FP_REG(rtbl, Rd);
            const char *Rn_s = GET_FP_REG(rtbl, Rn);

            ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

            concat(&DECODE_STR(out), " %s.%s, %s.%s", Rd_s, Tb, Rn_s, Ta);
        }

        SET_INSTR_ID(out, instr_id);

        return 0;
    }
    else if(opcode == 0x13){
        if(fp16 || scalar)
            return 1;

        if(U == 0)
            return 1;

        instr_s = Q == 0 ? "shll" : "shll2";
        instr_id = Q == 0 ? AD_INSTR_SHLL : AD_INSTR_SHLL2;

        const char *Ta = NULL;
        const char *Tb = NULL;

        if(size == 0){
            Ta = "8h";
            Tb = Q == 0 ? "8b" : "16b";
        }
        else if(size == 1){
            Ta = "4s";
            Tb = Q == 0 ? "4h" : "8h";
        }
        else if(size == 2){
            Ta = "2d";
            Tb = Q == 0 ? "2s" : "4s";
        }

        if(!Ta || !Tb)
            return 1;

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        U32 shift = 8 << size;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&shift);

        concat(&DECODE_STR(out), "%s %s.%s, %s.%s, #%#x", instr_s, Rd_s, Ta,
                Rn_s, Tb, shift);

        SET_INSTR_ID(out, instr_id);

        return 0;
    }
    else if(opcode == 0x16){
        if(fp16)
            return 1;

        U32 _sz = (size & 1);

        if(U == 0){
            instr_s = Q == 0 ? "fcvtn" : "fcvtn2";
            instr_id = Q == 0 ? AD_INSTR_FCVTN : AD_INSTR_FCVTN2;
        }
        else{
            if(scalar){
                instr_s = "fcvtxn";
                instr_id = AD_INSTR_FCVTXN;
            }
            else{
                instr_s = Q == 0 ? "fcvtxn" : "fcvtxn2";
                instr_id = Q == 0 ? AD_INSTR_FCVTXN : AD_INSTR_FCVTXN2;
            }
        }

        concat(&DECODE_STR(out), "%s", instr_s);

        if(scalar){
            if(_sz == 0)
                return 1;

            const char **Rd_rtbl = AD_RTBL_FP_32;
            const char **Rn_rtbl = AD_RTBL_FP_64;

            U32 Rd_sz = _32_BIT;
            U32 Rn_sz = _64_BIT;

            const char *Rd_s = GET_FP_REG(Rd_rtbl, Rd);
            const char *Rn_s = GET_FP_REG(Rn_rtbl, Rn);

            ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_rtbl);
            ADD_REG_OPERAND(out, Rn, Rn_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_rtbl);

            concat(&DECODE_STR(out), " %s, %s", Rd_s, Rn_s);
        }
        else{
            const char *Ta = NULL;
            const char *Tb = NULL;

            if(_sz == 0){
                if(instr_id == AD_INSTR_FCVTXN || instr_id == AD_INSTR_FCVTXN2)
                    return 1;

                Ta = "4s";
                Tb = Q == 0 ? "4h" : "8h";
            }
            else if(_sz == 1){
                Ta = "2d";
                Tb = Q == 0 ? "2s" : "4s";
            }

            if(!Ta || !Tb)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;

            const char *Rd_s = GET_FP_REG(rtbl, Rd);
            const char *Rn_s = GET_FP_REG(rtbl, Rn);

            ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
            ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

            concat(&DECODE_STR(out), " %s.%s, %s.%s", Rd_s, Tb, Rn_s, Ta);
        }

        SET_INSTR_ID(out, instr_id);

        return 0;
    }
    else if(opcode == 0x17){
        if(fp16 || scalar)
            return 1;

        U32 _sz = (size & 1);

        instr_s = Q == 0 ? "fcvtl" : "fcvtl2";
        instr_id = Q == 0 ? AD_INSTR_FCVTL : AD_INSTR_FCVTL2;

        const char *Ta = NULL;
        const char *Tb = NULL;

        if(_sz == 0){
            Ta = "4s";
            Tb = Q == 0 ? "4h" : "8h";
        }
        else if(_sz == 1){
            Ta = "2d";
            Tb = Q == 0 ? "2s" : "4s";
        }

        if(!Ta || !Tb)
            return 1;

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        concat(&DECODE_STR(out), "%s %s.%s, %s.%s", instr_s, Rd_s, Ta, Rn_s, Tb);

        SET_INSTR_ID(out, instr_id);

        return 0;
    }
    else if(opcode == 0x18 || opcode == 0x19 || opcode == 0x1e || opcode == 0x1f){
        if(scalar)
            return 1;

        U32 s = size >> 1;
        U32 _sz = (size & 1);

        if(U == 0){
            if(s == 0){
                if(opcode == 0x18){
                    instr_s = "frintn";
                    instr_id = AD_INSTR_FRINTN;
                }
                else if(opcode == 0x19){
                    instr_s = "frintm";
                    instr_id = AD_INSTR_FRINTM;
                }
                else if(opcode == 0x1e){
                    instr_s = "frint32z";
                    instr_id = AD_INSTR_FRINT32Z;
                }
                else{
                    instr_s = "frint64z";
                    instr_id = AD_INSTR_FRINT64Z;
                }
            }
            else{
                if(opcode == 0x1e || opcode == 0x1f)
                    return 1;

                instr_s = opcode == 0x18 ? "frintp" : "frintz";
                instr_id = opcode == 0x18 ? AD_INSTR_FRINTP : AD_INSTR_FRINTZ;
            }
        }
        else{
            if(s == 0){
                if(opcode == 0x18){
                    instr_s = "frinta";
                    instr_id = AD_INSTR_FRINTA;
                }
                else if(opcode == 0x19){
                    instr_s = "frintx";
                    instr_id = AD_INSTR_FRINTX;
                }
                else if(opcode == 0x1e){
                    instr_s = "frint32x";
                    instr_id = AD_INSTR_FRINT32X;
                }
                else{
                    instr_s = "frint64x";
                    instr_id = AD_INSTR_FRINT64X;
                }
            }
            else{
                if(opcode == 0x18 || opcode == 0x1e)
                    return 1;

                instr_s = opcode == 0x19 ? "frinti" : "fsqrt";
                instr_id = opcode == 0x19 ? AD_INSTR_FRINTI : AD_INSTR_FSQRT;
            }
        }

        if(fp16)
            T = Q == 0 ? "4h" : "8h";
        else{
            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";

            if(!T)
                return 1;
        }

        rtbl = AD_RTBL_FP_V_128;
        sz = _128_BIT;
    }
    else if(opcode >= 0x1a && opcode <= 0x1d){
        U32 s = size >> 1;
        U32 tempop = opcode - 0x1a;

        if(U == 0){
            if(s == 0){
                struct itab tab[] = {
                    { "fcvtns", AD_INSTR_FCVTNS }, { "fcvtms", AD_INSTR_FCVTMS },
                    { "fcvtas", AD_INSTR_FCVTAS }, { "scvtf", AD_INSTR_SCVTF }
                };

                if(OOB(tempop, tab))
                    return 1;

                instr_s = tab[tempop].instr_s;
                instr_id = tab[tempop].instr_id;
            }
            else{
                struct itab tab[] = {
                    { "fcvtps", AD_INSTR_FCVTPS }, { "fcvtzs", AD_INSTR_FCVTZS },
                    { "urecpe", AD_INSTR_URECPE }, { "frecpe", AD_INSTR_FRECPE }
                };

                if(OOB(tempop, tab))
                    return 1;

                instr_s = tab[tempop].instr_s;
                instr_id = tab[tempop].instr_id;
            }
        }
        else{
            if(s == 0){
                struct itab tab[] = {
                    { "fcvtnu", AD_INSTR_FCVTNU }, { "fcvtmu", AD_INSTR_FCVTMU },
                    { "fcvtau", AD_INSTR_FCVTAU }, { "ucvtf", AD_INSTR_UCVTF }
                };

                if(OOB(tempop, tab))
                    return 1;

                instr_s = tab[tempop].instr_s;
                instr_id = tab[tempop].instr_id;
            }
            else{
                struct itab tab[] = {
                    { "fcvtpu", AD_INSTR_FCVTPU }, { "fcvtzu", AD_INSTR_FCVTZU },
                    { "ursqrte", AD_INSTR_URSQRTE }, { "frsqrte", AD_INSTR_FRSQRTE }
                };

                if(OOB(tempop, tab))
                    return 1;

                instr_s = tab[tempop].instr_s;
                instr_id = tab[tempop].instr_id;
            }
        }

        U32 _sz = (size & 1);

        if(scalar && fp16){
            T = Q == 0 ? "4h" : "8h";

            rtbl = AD_RTBL_FP_16;
            sz = _16_BIT;
        }
        else if(scalar){
            rtbl = _sz == 0 ? AD_RTBL_FP_32 : AD_RTBL_FP_64;
            sz = _sz == 0 ? _32_BIT : _64_BIT;
        }
        else if(fp16){
            T = Q == 0 ? "4h" : "8h";

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
        else{
            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1){
                if(instr_id == AD_INSTR_URECPE || instr_id == AD_INSTR_URSQRTE)
                    return 1;

                T = "2d";
            }

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }
    }

    if(!rtbl)
        return 1;

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    if(scalar)
        concat(&DECODE_STR(out), ", %s", Rn_s);
    else
        concat(&DECODE_STR(out), ".%s, %s.%s", T, Rn_s, T);

    if(add_zero){
        ADD_IMM_OPERAND(out, AD_IMM_INT, 0);
        concat(&DECODE_STR(out), ", #0");
    }
    else if(add_zerof){
        ADD_IMM_OPERAND(out, AD_IMM_FLOAT, 0);
        concat(&DECODE_STR(out), ", #0.0");
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDScalarPairwiseInstr(struct instruction *i,
        struct ad_insn *out){
    U32 U = bits(i->opcode, 29, 29);
    U32 size = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 12, 16);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, U);
    ADD_FIELD(out, size);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char **Rd_rtbl = NULL;
    const char *T = NULL;

    U32 Rd_sz = 0;

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(opcode == 0x1b){
        if(U == 1 && size != 3)
            return 1;

        instr_s = "addp";
        instr_id = AD_INSTR_ADDP;

        Rd_rtbl = AD_RTBL_FP_64;
        Rd_sz = _64_BIT;

        T = "2d";
    }
    else{
        U32 s = size >> 1;
        U32 _sz = (size & 1);

        if(opcode == 13 && s == 1)
            return 1;

        S32 fp16 = (U == 0);

        U32 tempop = opcode - 12;

        struct itab tab[] = {
            { s == 0 ? "fmaxnmp" : "fminnmp", s == 0 ? AD_INSTR_FMAXNMP : AD_INSTR_FMINNMP },
            { s == 0 ? "faddp" : NULL, s == 0 ? AD_INSTR_FADDP : AD_NONE },
            /* one blank, idx 2 */
            { NULL, AD_NONE },
            { s == 0 ? "fmaxp" : "fminp", s == 0 ? AD_INSTR_FMAXP : AD_INSTR_FMINP }
        };

        if(OOB(tempop, tab))
            return 1;

        instr_s = tab[tempop].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[tempop].instr_id;

        if(fp16 && _sz == 1)
            return 1;

        if(fp16){
            Rd_rtbl = AD_RTBL_FP_16;
            Rd_sz = _16_BIT;

            T = "2h";
        }
        else{
            Rd_rtbl = _sz == 0 ? AD_RTBL_FP_32 : AD_RTBL_FP_64;
            Rd_sz = _sz == 0 ? _32_BIT : _64_BIT;

            T = _sz == 0 ? "2s" : "2d";
        }
    }

    if(!Rd_rtbl)
        return 1;

    const char *Rd_s = GET_FP_REG(Rd_rtbl, Rd);
    const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_rtbl);
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    concat(&DECODE_STR(out), "%s %s, %s.%s", instr_s, Rd_s, Rn_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDThreeDifferentInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 size = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 12, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, U);
    ADD_FIELD(out, size);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rm_Rtbl = NULL;

    U32 Rd_sz = 0;
    U32 Rn_Rm_sz = 0;

    const char *first_T = NULL;
    const char *second_T = NULL;
    const char *third_T = NULL;

    if(scalar){
        if(U == 1)
            return 1;

        if(size == 0 || size == 3)
            return 1;

        U32 tempop = opcode - 9;

        struct itab tab[] = {
            { "sqdmlal", AD_INSTR_SQDMLAL },
            /* one blank, idx 1 */
            { NULL, AD_NONE },
            { "sqdmlsl", AD_INSTR_SQDMLSL },
            /* one blank, idx 3 */
            { NULL, AD_NONE },
            { "sqdmull", AD_INSTR_SQDMULL }
        };

        if(OOB(tempop, tab))
            return 1;

        instr_s = tab[tempop].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[tempop].instr_id;

        Rd_Rtbl = size == 1 ? AD_RTBL_FP_32 : AD_RTBL_FP_64;
        Rn_Rm_Rtbl = size == 1 ? AD_RTBL_FP_16 : AD_RTBL_FP_32;

        Rd_sz = size == 1 ? _32_BIT : _64_BIT;
        Rn_Rm_sz = size == 1 ? _16_BIT : _32_BIT;
    }
    else{
        struct itab u0_tab[] = {
            { Q == 0 ? "saddl" : "saddl2", Q == 0 ? AD_INSTR_SADDL : AD_INSTR_SADDL2 },
            { Q == 0 ? "saddw" : "saddw2", Q == 0 ? AD_INSTR_SADDW : AD_INSTR_SADDW2 },
            { Q == 0 ? "ssubl" : "ssubl2", Q == 0 ? AD_INSTR_SSUBL : AD_INSTR_SSUBL2 },
            { Q == 0 ? "ssubw" : "ssubw2", Q == 0 ? AD_INSTR_SSUBW : AD_INSTR_SSUBW2 },
            { Q == 0 ? "addhn" : "addhn2", Q == 0 ? AD_INSTR_ADDHN : AD_INSTR_ADDHN2 },
            { Q == 0 ? "sabal" : "sabal2", Q == 0 ? AD_INSTR_SABAL : AD_INSTR_SABAL2 },
            { Q == 0 ? "subhn" : "subhn2", Q == 0 ? AD_INSTR_SUBHN : AD_INSTR_SUBHN2 },
            { Q == 0 ? "sabdl" : "sabdl2", Q == 0 ? AD_INSTR_SABDL : AD_INSTR_SABDL2 },
            { Q == 0 ? "smlal" : "smlal2", Q == 0 ? AD_INSTR_SMLAL : AD_INSTR_SMLAL2 },
            { Q == 0 ? "sqdmlal" : "sqdmlal2", Q == 0 ? AD_INSTR_SQDMLAL : AD_INSTR_SQDMLAL2 },
            { Q == 0 ? "smlsl" : "smlsl2", Q == 0 ? AD_INSTR_SMLSL : AD_INSTR_SMLSL2 },
            { Q == 0 ? "sqdmlsl" : "sqdmlsl2", Q == 0 ? AD_INSTR_SQDMLSL : AD_INSTR_SQDMLSL2 },
            { Q == 0 ? "smull" : "smull2", Q == 0 ? AD_INSTR_SMULL : AD_INSTR_SMULL2 },
            { Q == 0 ? "sqdmull" : "sqdmull2", Q == 0 ? AD_INSTR_SQDMULL : AD_INSTR_SQDMULL2 },
            { Q == 0 ? "pmull" : "pmull2", Q == 0 ? AD_INSTR_PMULL : AD_INSTR_PMULL2 },
        };

        struct itab u1_tab[] = {
            { Q == 0 ? "uaddl" : "uaddl2", Q == 0 ? AD_INSTR_UADDL : AD_INSTR_UADDL2 },
            { Q == 0 ? "uaddw" : "uaddw2", Q == 0 ? AD_INSTR_UADDW : AD_INSTR_UADDW2 },
            { Q == 0 ? "usubl" : "usubl2", Q == 0 ? AD_INSTR_USUBL : AD_INSTR_USUBL2 },
            { Q == 0 ? "usubw" : "usubw2", Q == 0 ? AD_INSTR_USUBW : AD_INSTR_USUBW2 },
            { Q == 0 ? "raddhn" : "raddhn2", Q == 0 ? AD_INSTR_RADDHN : AD_INSTR_RADDHN2 },
            { Q == 0 ? "uabal" : "uabal2", Q == 0 ? AD_INSTR_UABAL : AD_INSTR_UABAL2 },
            { Q == 0 ? "rsubhn" : "rsubhn2", Q == 0 ? AD_INSTR_RSUBHN : AD_INSTR_RSUBHN2 },
            { Q == 0 ? "uabdl" : "uabdl2", Q == 0 ? AD_INSTR_UABDL : AD_INSTR_UABDL2 },
            { Q == 0 ? "umlal" : "umlal2", Q == 0 ? AD_INSTR_UMLAL : AD_INSTR_UMLAL2 },
            /* one blank, idx 9 */
            { NULL, AD_NONE },
            { Q == 0 ? "umlsl" : "umlsl2", Q == 0 ? AD_INSTR_UMLSL : AD_INSTR_UMLSL2 },
            /* one blank, idx 11 */
            { NULL, AD_NONE },
            { Q == 0 ? "umull" : "umull2", Q == 0 ? AD_INSTR_UMULL : AD_INSTR_UMULL2 },
        };

        if(U == 0 && OOB(opcode, u0_tab))
            return 1;

        if(U == 1 && OOB(opcode, u1_tab))
            return 1;

        struct itab *tab = U == 0 ? u0_tab : u1_tab;

        instr_s = tab[opcode].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[opcode].instr_id;

        if(instr_id == AD_INSTR_SQDMLAL || instr_id == AD_INSTR_SQDMLSL ||
                instr_id == AD_INSTR_SQDMULL || instr_id == AD_INSTR_SQDMLAL2 ||
                instr_id == AD_INSTR_SQDMLSL2 || instr_id == AD_INSTR_SQDMULL2){
            if(size == 0 || size == 3)
                return 1;
        }

        if(instr_id == AD_INSTR_PMULL || instr_id == AD_INSTR_PMULL2){
            if(size == 1 || size == 2)
                return 1;
        }

        if(instr_id != AD_INSTR_PMULL && instr_id != AD_INSTR_PMULL2 && size == 3)
            return 1;

        const char *Ta = NULL;
        const char *Tb = NULL;

        if(size == 0){
            Ta = "8h";
            Tb = Q == 0 ? "8b" : "16b";
        }
        else if(size == 1){
            Ta = "4s";
            Tb = Q == 0 ? "4h" : "8h";
        }
        else if(size == 2){
            Ta = "2d";
            Tb = Q == 0 ? "2s" : "4s";
        }
        else if(size == 3){
            Ta = "1q";
            Tb = Q == 0 ? "1d" : "2d";
        }

        if(!Ta || !Tb)
            return 1;

        if(opcode == 0 || opcode == 2 || opcode == 5 || opcode >= 7){
            first_T = Ta;
            second_T = third_T = Tb;
        }
        else if(opcode == 1 || opcode == 3){
            first_T = second_T = Ta;
            third_T = Tb;
        }
        else if(opcode == 4 || opcode == 6){
            first_T = Tb;
            second_T = third_T = Ta;
        }

        if(!first_T || !second_T || !third_T)
            return 1;

        Rd_Rtbl = AD_RTBL_FP_V_128;
        Rn_Rm_Rtbl = AD_RTBL_FP_V_128;
        
        Rd_sz = _128_BIT;
        Rn_Rm_sz = _128_BIT;
    }

    if(!Rd_Rtbl || !Rn_Rm_Rtbl)
        return 1;

    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    const char *Rn_s = GET_FP_REG(Rn_Rm_Rtbl, Rn);
    const char *Rm_s = GET_FP_REG(Rn_Rm_Rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, Rn_Rm_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rm_Rtbl);
    ADD_REG_OPERAND(out, Rm, Rn_Rm_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rm_Rtbl);

    if(scalar)
        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);
    else{
        concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s, Rd_s,
                first_T, Rn_s, second_T, Rm_s, third_T);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static U64 VFPExpandImm(U32 imm8){
    const S32 N = 32;
    const S32 E = 8;
    const S32 F = N - E - 1;

    S32 sign = bits(imm8, 7, 7) << (1 + (E - 3) + 2 + (F - 4) + 4);

    S32 exp_p1 = (bits(imm8, 6, 6) ^ 1) << ((E - 3) + 2);
    S32 exp_p2 = replicate(bits(imm8, 6, 6), 1, E - 3) << 2;
    S32 exp_p3 = bits(imm8, 4, 5);

    S32 exp = (exp_p1 | exp_p2 | exp_p3) << ((F - 4) + 4);

    S32 frac = bits(imm8, 0, 4) << (F - 4);

    return sign | exp | frac;
}

static S32 DisassembleAdvancedSIMDModifiedImmediateInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Q = bits(i->opcode, 30, 30);
    U32 op = bits(i->opcode, 29, 29);
    U32 a = bits(i->opcode, 18, 18);
    U32 b = bits(i->opcode, 17, 17);
    U32 c = bits(i->opcode, 16, 16);
    U32 cmode = bits(i->opcode, 12, 15);
    U32 o2 = bits(i->opcode, 11, 11);
    U32 d = bits(i->opcode, 9, 9);
    U32 e = bits(i->opcode, 8, 8);
    U32 f = bits(i->opcode, 7, 7);
    U32 g = bits(i->opcode, 6, 6);
    U32 h = bits(i->opcode, 5, 5);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Q);
    ADD_FIELD(out, op);
    ADD_FIELD(out, a);
    ADD_FIELD(out, b);
    ADD_FIELD(out, c);
    ADD_FIELD(out, cmode);
    ADD_FIELD(out, o2);
    ADD_FIELD(out, d);
    ADD_FIELD(out, e);
    ADD_FIELD(out, f);
    ADD_FIELD(out, g);
    ADD_FIELD(out, h);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    U32 operation = (cmode << 1) | op;

    if(cmode == 0xf){
        if(Q == 0 && op == 1 && o2 == 0)
            return 1;

        instr_s = "fmov";
        instr_id = AD_INSTR_FMOV;
    }
    else if((operation & ~12) == 0 || (operation & ~4) == 0x10 || (operation & ~2) == 0x18 ||
            (operation & ~1) == 0x1c || operation == 0x1e || operation == 0x1f){
        if(operation == 0x1f && Q == 0)
            return 1;

        instr_s = "movi";
        instr_id = AD_INSTR_MOVI;
    }
    else if((operation & ~12) == 1 || (operation & ~4) == 0x11 || (operation & ~2) == 0x19){
        instr_s = "mvni";
        instr_id = AD_INSTR_MVNI;
    }
    else if((operation & ~12) == 2 || (operation & ~4) == 0x12){
        instr_s = "orr";
        instr_id = AD_INSTR_ORR;
    }
    else if((operation & ~12) == 3 || (operation & ~4) == 0x13){
        instr_s = "bic";
        instr_id = AD_INSTR_BIC;
    }

    if(!instr_s)
        return 1;

    const char **Rd_Rtbl = NULL;
    U32 Rd_sz = 0;

    const char *T = NULL;

    const char *shift_s = NULL;
    S32 shift_type = AD_NONE;
    S32 shift_amt = AD_NONE;

    U32 imm8 = (a << 7) | (b << 6) | (c << 5) | (d << 4) | (e << 3) |
        (f << 2) | (g << 1) | h;

    U64 imm = 0;
    F32 immf = 0.0f;

    if(instr_id == AD_INSTR_FMOV){
        if(Q == 1 && op == 1)
            T = "2d";
        else if(op == 0){
            if(o2 == 0)
                T = Q == 0 ? "2s" : "4s";
            else
                T = Q == 0 ? "4h" : "8h";
        }

        U32 tempimm = VFPExpandImm(imm8);

        immf = *(F32 *)&tempimm;

        Rd_Rtbl = AD_RTBL_FP_V_128;
        Rd_sz = _128_BIT;
    }
    else{
        if(op == 0 && cmode == 14){
            T = Q == 0 ? "8b" : "16b";

            Rd_Rtbl = AD_RTBL_FP_V_128;
            Rd_sz = _128_BIT;
        }
        else if((cmode & ~2) == 8 || (cmode & ~2) == 9){
            T = Q == 0 ? "4h" : "8h";

            shift_s = "lsl";
            shift_type = AD_SHIFT_LSL;
            shift_amt = 8 * ((cmode >> 1) & 1);

            Rd_Rtbl = AD_RTBL_FP_V_128;
            Rd_sz = _128_BIT;
        }
        else if((cmode & ~6) == 0 || (cmode & ~6) == 1 || (cmode & ~1) == 12){
            T = Q == 0 ? "2s" : "4s";

            if((cmode & ~6) == 0 || (cmode & ~6) == 1){
                shift_s = "lsl";
                shift_type = AD_SHIFT_LSL;
                shift_amt = 8 * bits(cmode, 1, 2);
            }
            else{
                shift_s = "msl";
                shift_type = AD_SHIFT_MSL;
                shift_amt = 8 << (cmode & 1);
            }

            Rd_Rtbl = AD_RTBL_FP_V_128;
            Rd_sz = _128_BIT;
        }
        else if(op == 1 && cmode == 14){
            if(Q == 0){
                Rd_Rtbl = AD_RTBL_FP_64;
                Rd_sz = _64_BIT;
            }
            else{
                T = "2d";

                Rd_Rtbl = AD_RTBL_FP_V_128;
                Rd_sz = _128_BIT;
            }
        }

        imm = (replicate(a, 1, 8) << 56) | (replicate(b, 1, 8) << 48) |
            (replicate(c, 1, 8) << 40) | (replicate(d, 1, 8) << 32) |
            (replicate(e, 1, 8) << 24) | (replicate(f, 1, 8) << 16) |
            (replicate(g, 1, 8) << 8) | replicate(h, 1, 8);
    }

    if(!Rd_Rtbl)
        return 1;
    
    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    /* only instr without arrangement specifier is MOVI (64 bit scalar variant) */
    if(!(instr_id == AD_INSTR_MOVI && Q == 0 && op == 1 && cmode == 14)){
        if(!T)
            return 1;

        concat(&DECODE_STR(out), ".%s", T);
    }

    if(instr_id == AD_INSTR_FMOV){
        ADD_IMM_OPERAND(out, AD_IMM_FLOAT, *(U32 *)&immf);

        concat(&DECODE_STR(out), ", #%f", immf);

        /* done constructing decode string for FMOV */
        SET_INSTR_ID(out, instr_id);

        return 0;
    }

    /* at this point, only instr without shift is MOVI (64 bit, both variants) */
    if(instr_id == AD_INSTR_MOVI && op == 1 && cmode == 14){
        ADD_IMM_OPERAND(out, AD_IMM_LONG, *(S64 *)&imm);

        concat(&DECODE_STR(out), ", #"S_LX"", S_LA(imm));

        SET_INSTR_ID(out, instr_id);

        return 0;
    }

    ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&imm8);

    concat(&DECODE_STR(out), ", #"S_X"", S_A(imm8));

    if(shift_type != AD_NONE){
        if(shift_type == AD_SHIFT_MSL || (shift_type == AD_SHIFT_LSL && shift_amt > 0)){
            ADD_SHIFT_OPERAND(out, shift_type, shift_amt);

            concat(&DECODE_STR(out), ", %s #%d", shift_s, shift_amt);
        }
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDShiftByImmediateInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 immh = bits(i->opcode, 19, 22);
    U32 immb = bits(i->opcode, 16, 18);
    U32 opcode = bits(i->opcode, 11, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, U);
    ADD_FIELD(out, immh);
    ADD_FIELD(out, immb);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char **rtbls[] = {
        AD_RTBL_FP_8, AD_RTBL_FP_16, AD_RTBL_FP_32, AD_RTBL_FP_64
    };

    U32 sizes[] = {
        _8_BIT, _16_BIT, _32_BIT, _64_BIT
    };

    if(opcode <= 0xe){
        struct itab tab[] = {
            { U == 0 ? "sshr" : "ushr", U == 0 ? AD_INSTR_SSHR : AD_INSTR_USHR },
            /* one blank, idx 1 */
            { NULL, AD_NONE },
            { U == 0 ? "ssra" : "usra", U == 0 ? AD_INSTR_SSRA : AD_INSTR_USRA },
            /* one blank, idx 3 */
            { NULL, AD_NONE },
            { U == 0 ? "srshr" : "urshr", U == 0 ? AD_INSTR_SRSHR : AD_INSTR_URSHR },
            /* one blank, idx 5 */
            { NULL, AD_NONE },
            { U == 0 ? "srsra" : "ursra", U == 0 ? AD_INSTR_SRSRA : AD_INSTR_URSRA },
            /* one blank, idx 7 */
            { NULL, AD_NONE },
            { U == 0 ? NULL : "sri", U == 0 ? AD_NONE : AD_INSTR_SRI },
            /* one blank, idx 9 */
            { NULL, AD_NONE },
            { U == 0 ? "shl" : "sli", U == 0 ? AD_INSTR_SHL : AD_INSTR_SLI },
            /* one blank, idx 11 */
            { NULL, AD_NONE },
            { U == 0 ? NULL : "sqshlu", U == 0 ? AD_NONE : AD_INSTR_SQSHLU },
            /* one blank, idx 13 */
            { NULL, AD_NONE },
            { U == 0 ? "sqshl" : "uqshl", U == 0 ? AD_INSTR_SQSHL : AD_INSTR_UQSHL },
        };

        if(OOB(opcode, tab))
            return 1;

        instr_s = tab[opcode].instr_s;
        
        if(!instr_s)
            return 1;

        instr_id = tab[opcode].instr_id;

        const char **rtbl = NULL;
        U32 sz = 0;

        const char *T = NULL;

        U32 shift = 0;

        S32 hsb = HighestSetBit(immh, 4);

        if(scalar){
            if(instr_id != AD_INSTR_SQSHL && instr_id != AD_INSTR_SQSHLU &&
                    instr_id != AD_INSTR_UQSHL){
                if((immh >> 3) == 0)
                    return 1;

                rtbl = AD_RTBL_FP_64;
                sz = _64_BIT;
            }
            else{
                if(hsb == -1)
                    return 1;

                rtbl = rtbls[hsb];
                sz = sizes[hsb];
            }
        }
        else{
            if(immh == 1)
                T = Q == 0 ? "8b" : "16b";
            else if((immh & ~1) == 2)
                T = Q == 0 ? "4h" : "8h";
            else if((immh & ~3) == 4)
                T = Q == 0 ? "2s" : "4s";
            else if((immh & ~7) == 8 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }

        if(!rtbl)
            return 1;

        if(opcode <= 8){
            if(scalar)
                shift = ((8 << 3) * 2) - ((immh << 3) | immb);
            else
                shift = ((8 << hsb) * 2) - ((immh << 3) | immb);
        }
        else if(opcode > 8 && opcode <= 0xb){
            if(scalar)
                shift = ((immh << 3) | immb) - (8 << 3);
            else
                shift = ((immh << 3) | immb) - (8 << hsb);
        }
        else if(opcode == 0xc || opcode == 0xe){
            shift = ((immh << 3) | immb) - (8 << hsb);
        }

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

        if(scalar)
            concat(&DECODE_STR(out), ", %s", Rn_s);
        else
            concat(&DECODE_STR(out), ".%s, %s.%s", T, Rn_s, T);

        if(shift > 0){
            concat(&DECODE_STR(out), ", #%#x", shift);

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&shift);
        }
    }
    else if(opcode >= 0x10 && opcode <= 0x14){
        const char **Rd_Rtbl = NULL;
        const char **Rn_Rtbl = NULL;

        U32 Rd_sz = 0;
        U32 Rn_sz = 0;

        const char *Ta = NULL;
        const char *Tb = NULL;

        S32 Ta_first = 0;

        U32 shift = 0;

        if(scalar){
            if(opcode == 0x10){
                if(U == 0)
                    return 1;

                instr_s = "sqshrun";
                instr_id = AD_INSTR_SQSHRUN;
            }
            else if(opcode == 0x11){
                if(U == 0)
                    return 1;

                instr_s = "sqrshrun";
                instr_id = AD_INSTR_SQRSHRUN;
            }
            else if(opcode == 0x12){
                instr_s = U == 0 ? "sqshrn" : "uqshrn";
                instr_id = U == 0 ? AD_INSTR_SQSHRN : AD_INSTR_UQSHRN;
            }
            else if(opcode == 0x13){
                instr_s = U == 0 ? "sqrshrn" : "uqrshrn";
                instr_id = U == 0 ? AD_INSTR_SQRSHRN : AD_INSTR_UQRSHRN;
            }

            if(!instr_s)
                return 1;

            S32 hsb = HighestSetBit(immh, 4);

            if(hsb == -1)
                return 1;

            shift = (2 * (8 << hsb)) - ((immh << 3) | immb);

            if(OOB(hsb, rtbls) || OOB(hsb, sizes) || OOB(hsb + 1, rtbls) ||
                    OOB(hsb + 1, sizes)){
                return 1;
            }

            Rd_Rtbl = rtbls[hsb];
            Rn_Rtbl = rtbls[hsb + 1];

            Rd_sz = sizes[hsb];
            Rn_sz = sizes[hsb + 1];
        }
        else{
            struct itab u0_tab[] = {
                { Q == 0 ? "shrn" : "shrn2", Q == 0 ? AD_INSTR_SHRN : AD_INSTR_SHRN2 },
                { Q == 0 ? "rshrn" : "rshrn2", Q == 0 ? AD_INSTR_RSHRN : AD_INSTR_RSHRN2 },
                { Q == 0 ? "sqshrn" : "sqshrn2", Q == 0 ? AD_INSTR_SQSHRN : AD_INSTR_SQSHRN2 },
                { Q == 0 ? "sqrshrn" : "sqrshrn2", Q == 0 ? AD_INSTR_SQRSHRN : AD_INSTR_SQRSHRN2 },
                { Q == 0 ? "sshll" : "sshll2", Q == 0 ? AD_INSTR_SSHLL : AD_INSTR_SSHLL2 }
            };

            struct itab u1_tab[] = {
                { Q == 0 ? "sqshrun" : "sqshrun2", Q == 0 ? AD_INSTR_SQSHRUN : AD_INSTR_SQSHRUN2 },
                { Q == 0 ? "sqrshrun" : "sqrshrun2", Q == 0 ? AD_INSTR_SQRSHRUN : AD_INSTR_SQRSHRUN2 },
                { Q == 0 ? "uqshrn" : "uqshrn2", Q == 0 ? AD_INSTR_UQSHRN : AD_INSTR_UQSHRN2 },
                { Q == 0 ? "uqrshrn" : "uqrshrn2", Q == 0 ? AD_INSTR_UQRSHRN : AD_INSTR_UQRSHRN2 },
                { Q == 0 ? "ushll" : "ushll2", Q == 0 ? AD_INSTR_USHLL : AD_INSTR_USHLL2 }
            };

            U32 tempop = opcode - 0x10;

            if(U == 0 && OOB(tempop, u0_tab))
                return 1;

            if(U == 1 && OOB(tempop, u1_tab))
                return 1;

            struct itab *tab = U == 0 ? u0_tab : u1_tab;

            instr_s = tab[tempop].instr_s;
            instr_id = tab[tempop].instr_id;

            S32 xshll_alias = 0;
            S32 hsb = HighestSetBit(immh, 4);

            if(instr_id != AD_INSTR_SSHLL && instr_id != AD_INSTR_SSHLL2 &&
                    instr_id != AD_INSTR_USHLL && instr_id != AD_INSTR_USHLL2){
                shift = (2 * (8 << hsb)) - ((immh << 3) | immb);
            }
            else{
                xshll_alias = (immb == 0 && BitCount(immh, 4) == 1);
                Ta_first = 1;

                shift = ((immh << 3) | immb) - (8 << hsb);
            }

            if(xshll_alias){
                if(U == 0){
                    instr_s = Q == 0 ? "sxtl" : "sxtl2";
                    instr_id = Q == 0 ? AD_INSTR_SXTL : AD_INSTR_SXTL2;
                }
                else{
                    instr_s = Q == 0 ? "uxtl" : "uxtl2";
                    instr_id = Q == 0 ? AD_INSTR_UXTL : AD_INSTR_UXTL2;
                }
            }

            if(immh == 1){
                Ta = "8h";
                Tb = Q == 0 ? "8b": "16b";
            }
            else if((immh & ~1) == 2){
                Ta = "4s";
                Tb = Q == 0 ? "4h" : "8h";
            }
            else if((immh & ~3) == 4){
                Ta = "2d";
                Tb = Q == 0 ? "2s" : "4s";
            }

            if(!Ta || !Tb)
                return 1;

            Rd_Rtbl = AD_RTBL_FP_V_128;
            Rn_Rtbl = AD_RTBL_FP_V_128;

            Rd_sz = _128_BIT;
            Rn_sz = _128_BIT;
        }

        if(!Rd_Rtbl || !Rn_Rtbl)
            return 1;

        const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
        const char *Rn_s = GET_FP_REG(Rn_Rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
        ADD_REG_OPERAND(out, Rn, Rn_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

        if(scalar)
            concat(&DECODE_STR(out), ", %s", Rn_s);
        else{
            concat(&DECODE_STR(out), ".%s, %s.%s", Ta_first ? Ta : Tb,
                    Rn_s, Ta_first ? Tb : Ta);
        }

        if(shift > 0){
            concat(&DECODE_STR(out), ", #%#x", shift);

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&shift);
        }
    }
    else if(opcode == 0x1c || opcode == 0x1f){
        if(opcode == 0x1c){
            instr_s = U == 0 ? "scvtf" : "ucvtf";
            instr_id = U == 0 ? AD_INSTR_SCVTF : AD_INSTR_UCVTF;
        }
        else{
            instr_s = U == 0 ? "fcvtzs" : "fcvtzu";
            instr_id = U == 0 ? AD_INSTR_FCVTZS : AD_INSTR_FCVTZU;
        }

        const char **rtbl = NULL;
        U32 sz = 0;

        const char *T = NULL;

        S32 hsb = HighestSetBit(immh, 4);

        if(hsb <= 0)
            return 1;

        U32 fbits = ((8 << hsb) * 2) - ((immh << 3) | immb);

        if(scalar){
            rtbl = rtbls[hsb];
            sz = sizes[hsb];
        }
        else{
            if((immh & ~1) == 2)
                T = Q == 0 ? "4h" : "8h";
            else if((immh & ~3) == 4)
                T = Q == 0 ? "2s" : "4s";
            else if((immh & ~7) == 8 && Q == 1)
                T = "2d";

            if(!T)
                return 1;

            rtbl = AD_RTBL_FP_V_128;
            sz = _128_BIT;
        }

        if(!rtbl)
            return 1;

        const char *Rd_s = GET_FP_REG(rtbl, Rd);
        const char *Rn_s = GET_FP_REG(rtbl, Rn);

        ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

        if(scalar)
            concat(&DECODE_STR(out), ", %s", Rn_s);
        else
            concat(&DECODE_STR(out), ".%s, %s.%s", T, Rn_s, T);

        if(fbits > 0){
            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&fbits);

            concat(&DECODE_STR(out), ", #%#x", fbits);
        }
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDXIndexedElementInstr(struct instruction *i,
        struct ad_insn *out, S32 scalar){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 size = bits(i->opcode, 22, 23);
    U32 L = bits(i->opcode, 21, 21);
    U32 M = bits(i->opcode, 20, 20);
    U32 Rm = bits(i->opcode, 16, 19);
    U32 opcode = bits(i->opcode, 12, 15);
    U32 H = bits(i->opcode, 11, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(!scalar)
        ADD_FIELD(out, Q);

    ADD_FIELD(out, U);
    ADD_FIELD(out, size);
    ADD_FIELD(out, L);
    ADD_FIELD(out, M);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, H);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;
    const char **Rm_Rtbl = AD_RTBL_FP_V_128;

    U32 Rd_sz = 0;
    U32 Rn_sz = 0;
    U32 Rm_sz = _128_BIT;

    U32 s = size >> 1;
    U32 _sz = (size & 1);

    const char *Ta = NULL;
    const char *Tb = NULL;
    const char *Ts = NULL;

    const char *T = NULL;
    S32 only_T = 0;

    U32 rotate = 90 * bits(i->opcode, 13, 14);

    U32 index = 0;

    if((U == 1 && opcode == 0) || opcode == 2 || (U == 0 && opcode == 3) || 
            (U == 1 && opcode == 4) || opcode == 6 ||
            (U == 0 && opcode == 7) || (U == 0 && opcode == 8) || opcode == 10 ||
            (U == 0 && opcode == 11) || (U == 0 && opcode == 12) || opcode == 13 ||
            opcode == 14 || (U == 1 && opcode == 15)){
        if(opcode == 0){
            if(scalar)
                return 1;

            instr_s = "mla";
            instr_id = AD_INSTR_MLA;

            only_T = 1;
        }
        else if(opcode == 2){
            if(scalar)
                return 1;

            if(U == 0){
                instr_s = Q == 0 ? "smlal" : "smlal2";
                instr_id = Q == 0 ? AD_INSTR_SMLAL : AD_INSTR_SMLAL2;
            }
            else{
                instr_s = Q == 0 ? "umlal" : "umlal2";
                instr_id = Q == 0 ? AD_INSTR_UMLAL : AD_INSTR_UMLAL2;
            }
        }
        else if(opcode == 3){
            if(scalar){
                instr_s = "sqdmlal";
                instr_id = AD_INSTR_SQDMLAL;
            }
            else{
                instr_s = Q == 0 ? "sqdmlal" : "sqdmlal2";
                instr_id = Q == 0 ? AD_INSTR_SQDMLAL : AD_INSTR_SQDMLAL2;
            }
        }
        else if(opcode == 4){
            instr_s = "mls";
            instr_id = AD_INSTR_MLS;

            only_T = 1;
        }
        else if(opcode == 6){
            if(scalar)
                return 1;

            if(U == 0){
                instr_s = Q == 0 ? "smlsl" : "smlsl2";
                instr_id = Q == 0 ? AD_INSTR_SMLSL : AD_INSTR_SMLSL2;
            }
            else{
                instr_s = Q == 0 ? "umlsl" : "umlsl2";
                instr_id = Q == 0 ? AD_INSTR_UMLSL : AD_INSTR_UMLSL2;
            }
        }
        else if(opcode == 7){
            if(scalar){
                instr_s = "sqdmlsl";
                instr_id = AD_INSTR_SQDMLSL;
            }
            else{
                instr_s = Q == 0 ? "sqdmlsl" : "sqdmlsl2";
                instr_id = Q == 0 ? AD_INSTR_SQDMLSL : AD_INSTR_SQDMLSL2;
            }
        }
        else if(opcode == 8){
            instr_s = "mul";
            instr_id = AD_INSTR_MUL;

            only_T = 1;
        }
        else if(opcode == 10){
            if(scalar)
                return 1;

            if(U == 0){
                instr_s = Q == 0 ? "smull" : "smull2";
                instr_id = Q == 0 ? AD_INSTR_SMULL : AD_INSTR_SMULL2;
            }
            else{
                instr_s = Q == 0 ? "umull" : "umull2";
                instr_id = Q == 0 ? AD_INSTR_UMULL : AD_INSTR_UMULL2;
            }
        }
        else if(opcode == 11){
            if(scalar){
                instr_s = "sqdmull";
                instr_id = AD_INSTR_SQDMULL;
            }
            else{
                instr_s = Q == 0 ? "sqdmull" : "sqdmull2";
                instr_id = Q == 0 ? AD_INSTR_SQDMULL : AD_INSTR_SQDMULL2;
            }
        }
        else if(opcode == 12){
            instr_s = "sqdmulh";
            instr_id = AD_INSTR_SQDMULH;

            only_T = 1;
        }
        else if(opcode == 13){
            instr_s = U == 0 ? "sqrdmulh" : "sqrdmlah";
            instr_id = U == 0 ? AD_INSTR_SQRDMULH : AD_INSTR_SQRDMLAH;

            only_T = 1;
        }
        else if(opcode == 14){
            if(scalar || size != 2)
                return 1;

            instr_s = U == 0 ? "sdot" : "udot";
            instr_id = U == 0 ? AD_INSTR_SDOT : AD_INSTR_UDOT;
        }
        else if(opcode == 15){
            instr_s = "sqrdmlsh";
            instr_id = AD_INSTR_SQRDMLSH;

            only_T = 1;
        }

        U32 Rmhi;

        switch(size){
            case 1:
                {
                    index = (H << 2) | (L << 1) | M;
                    Rmhi = 0;
                    break;
                }
            case 2:
                {
                    index = (H << 1) | L;
                    Rmhi = M;
                    break;
                }
            default: return 1;
        };

        Rm |= (Rmhi << 4);

        if(instr_id == AD_INSTR_SDOT || instr_id == AD_INSTR_UDOT){
            Ta = Q == 0 ? "2s" : "4s";
            Tb = Q == 0 ? "8b" : "16b";

            Ts = "4b";
        }
        else{
            if(size == 1){
                Ta = "4s";
                Tb = Q == 0 ? "4h" : "8h";
            }
            else{
                Ta = "2d";
                Tb = Q == 0 ? "2s" : "4s";
            }

            Ts = size == 1 ? "h" : "s";

            T = Tb;
        }
    }
    else if(size == 2 && (opcode == 0 || opcode == 4 || opcode == 8 || opcode == 12)){
        if(opcode == 0 || opcode == 8){
            instr_s = opcode == 0 ? "fmlal" : "fmlal2";
            instr_id = opcode == 0 ? AD_INSTR_FMLAL : AD_INSTR_FMLAL2;
        }
        else{
            instr_s = opcode == 8 ? "fmlsl" : "fmlsl2";
            instr_id = opcode == 8 ? AD_INSTR_FMLSL : AD_INSTR_FMLSL2;
        }

        index = (H << 2) | (L << 1) | M;

        Ta = Q == 0 ? "2s" : "4s";
        Tb = Q == 0 ? "2h" : "4h";

        Ts = "h";
    }
    else if((U == 0 && opcode == 1) || (U == 0 && opcode == 5) || opcode == 9){
        if(opcode == 1){
            instr_s = "fmla";
            instr_id = AD_INSTR_FMLA;
        }
        else if(opcode == 5){
            instr_s = "fmls";
            instr_id = AD_INSTR_FMLS;
        }
        else if(opcode == 9){
            instr_s = U == 0 ? "fmul" : "fmulx";
            instr_id = U == 0 ? AD_INSTR_FMUL : AD_INSTR_FMULX;
        }

        if(s == 0){
            index = (H << 2) | (L << 1) | M;

            T = Q == 0 ? "4h" : "8h";
            Ts = "h";
        }
        else{
            U32 Rmhi = M;
            U32 size = (_sz << 1) | L;

            if((size & ~1) == 0)
                index = (H << 1) | L;
            else if(size == 2)
                index = H;
            else if(size == 3)
                return 1;

            if(_sz == 0)
                T = Q == 0 ? "2s" : "4s";
            else if(_sz == 1 && Q == 1)
                T = "2d";
            else
                return 1;

            Ts = _sz == 0 ? "s" : "d";

            Rm |= (Rmhi << 4);
        }

        only_T = 1;
    }
    else if(U == 1 && (size == 1 || size == 2) && (opcode & ~6) == 1){
        instr_s = "fcmla";
        instr_id = AD_INSTR_FCMLA;

        if(size == 1){
            index = (H << 1) | L;

            T = Q == 0 ? "4h" : "8h";
            Ts = "h";
        }
        else{
            index = H;

            T = "4s";
            Ts = "s";
        }

        Rm |= (M << 4);

        only_T = 1;
    }

    if(!instr_s)
        return 1;

    if(scalar){
        if(instr_id == AD_INSTR_SQDMLAL || instr_id == AD_INSTR_SQDMLSL ||
                instr_id == AD_INSTR_SQDMULL){
            Rd_Rtbl = size == 1 ? AD_RTBL_FP_32 : AD_RTBL_FP_64;
            Rn_Rtbl = size == 1 ? AD_RTBL_FP_16 : AD_RTBL_FP_32;

            Rd_sz = size == 1 ? _32_BIT : _64_BIT;
            Rn_sz = size == 1 ? _16_BIT : _32_BIT;
        }
        else if(instr_id == AD_INSTR_FMLA || instr_id == AD_INSTR_FMLS ||
                instr_id == AD_INSTR_FMUL || instr_id == AD_INSTR_FMULX){
            if(s == 0){
                Rd_Rtbl = AD_RTBL_FP_16;
                Rn_Rtbl = Rd_Rtbl;

                Rd_sz = _16_BIT;
                Rn_sz = Rd_sz;
            }
            else{
                Rd_Rtbl = _sz == 0 ? AD_RTBL_FP_32 : AD_RTBL_FP_64;
                Rn_Rtbl = Rd_Rtbl;

                Rd_sz = _sz == 0 ? _32_BIT : _64_BIT;
                Rn_sz = Rd_sz;
            }
        }
        else{
            Rd_Rtbl = size == 1 ? AD_RTBL_FP_16 : AD_RTBL_FP_32;
            Rn_Rtbl = Rd_Rtbl;

            Rd_sz = size == 1 ? _16_BIT : _32_BIT;
            Rn_sz = Rd_sz;
        }
    }
    else{
        Rd_Rtbl = AD_RTBL_FP_V_128;
        Rn_Rtbl = Rd_Rtbl;

        Rd_sz = _128_BIT;
        Rn_sz = Rd_sz;
    }

    if(!Rd_Rtbl || !Rn_Rtbl)
        return 1;

    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    const char *Rn_s = GET_FP_REG(Rn_Rtbl, Rn);
    const char *Rm_s = GET_FP_REG(Rm_Rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, Rn_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);
    ADD_REG_OPERAND(out, Rm, Rm_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rm_Rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    if(scalar)
        concat(&DECODE_STR(out), ", %s", Rn_s);
    else{
        if(only_T)
            concat(&DECODE_STR(out), ".%s, %s.%s", T, Rn_s, T);
        else
            concat(&DECODE_STR(out), ".%s, %s.%s", Ta, Rn_s, Tb);
    }

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&index);
    concat(&DECODE_STR(out), ", %s.%s[%d]", Rm_s, Ts, index);

    if(instr_id == AD_INSTR_FCMLA){
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&rotate);
        concat(&DECODE_STR(out), ", #%d", rotate);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDTableLookupInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Q = bits(i->opcode, 30, 30);
    U32 op2 = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 len = bits(i->opcode, 13, 14);
    U32 op = bits(i->opcode, 12, 12);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(op2 != 0)
        return 1;

    ADD_FIELD(out, Q);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, len);
    ADD_FIELD(out, op);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = op == 0 ? "tbl" : "tbx";
    S32 instr_id = op == 0 ? AD_INSTR_TBL : AD_INSTR_TBX;

    const char *Ta = Q == 0 ? "8b" : "16b";

    const char **rtbl = AD_RTBL_FP_V_128;
    U32 sz = _128_BIT;

    len++;

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s.%s, {", instr_s, Rd_s, Ta);

    for(S32 i=Rn; i<(Rn+len); i++){
        const char *Rn_s = GET_FP_REG(rtbl, i);
        ADD_REG_OPERAND(out, i, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

        if(i == (Rn+len) - 1)
            concat(&DECODE_STR(out), " %s.16b", Rn_s);
        else
            concat(&DECODE_STR(out), " %s.16b,", Rn_s);
    }

    const char *Rm_s = GET_FP_REG(rtbl, Rm);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), " }, %s.%s", Rm_s, Ta);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDPermuteInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Q = bits(i->opcode, 30, 30);
    U32 size = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 12, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Q);
    ADD_FIELD(out, size);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        /* one blank, idx 0 */
        { NULL, AD_NONE },
        { "uzp1", AD_INSTR_UZP1 }, { "trn1", AD_INSTR_TRN1 },
        { "zip1", AD_INSTR_ZIP1 },
        /* one blank, idx 4 */
        { NULL, AD_NONE },
        { "uzp2", AD_INSTR_UZP2 }, { "trn2", AD_INSTR_TRN2 },
        { "zip2", AD_INSTR_ZIP2 },
    };

    if(OOB(opcode, tab))
        return 1;

    const char *instr_s = tab[opcode].instr_s;

    if(!instr_s)
        return 1;

    S32 instr_id = tab[opcode].instr_id;

    const char **rtbl = AD_RTBL_FP_V_128;
    U32 sz = _128_BIT;

    const char *T = NULL;

    if(size == 0)
        T = Q == 0 ? "8b" : "16b";
    else if(size == 1)
        T = Q == 0 ? "4h" : "8h";
    else if(size == 2)
        T = Q == 0 ? "2s" : "4s";
    else if(size == 3 && Q == 1)
        T = "2d";

    if(!T)
        return 1;

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s", instr_s, Rd_s, T, Rn_s,
            T, Rm_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAdvancedSIMDExtractInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Q = bits(i->opcode, 30, 30);
    U32 op2 = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 imm4 = bits(i->opcode, 11, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(op2 != 0)
        return 1;

    if(Q == 0 && ((imm4 >> 3) & 1) == 1)
        return 1;

    ADD_FIELD(out, Q);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, imm4);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *T = Q == 0 ? "8b" : "16b";

    const char **rtbl = AD_RTBL_FP_V_128;
    U32 sz = _128_BIT;

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    S32 index = sign_extend(imm4, 4);

    ADD_IMM_OPERAND(out, AD_IMM_INT, *(S32 *)&index);

    concat(&DECODE_STR(out), "ext %s.%s, %s.%s, %s.%s, #"S_X"", Rd_s, T, Rn_s,
            T, Rm_s, T, S_A(index));

    SET_INSTR_ID(out, AD_INSTR_EXT);

    return 0;
}

static S32 DisassembleAdvancedSIMDAcrossLanesInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Q = bits(i->opcode, 30, 30);
    U32 U = bits(i->opcode, 29, 29);
    U32 size = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 12, 16);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Q);
    ADD_FIELD(out, U);
    ADD_FIELD(out, size);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char **Rd_Rtbl = NULL;
    U32 Rd_sz = 0;

    const char *T = NULL;

    if(opcode == 3 || opcode == 10 || opcode == 0x1a || opcode == 0x1b){
        if(size == 3)
            return 1;

        const char **rtbls_o3[] = {
            AD_RTBL_FP_16, AD_RTBL_FP_32, AD_RTBL_FP_64
        };

        const char **rtbls[] = {
            AD_RTBL_FP_8, AD_RTBL_FP_16, AD_RTBL_FP_32
        };

        U32 sizes_o3[] = {
            _16_BIT, _32_BIT, _64_BIT
        };

        U32 sizes[] = {
            _8_BIT, _16_BIT, _32_BIT
        };

        if(opcode == 3){
            instr_s = U == 0 ? "saddlv" : "uaddlv";
            instr_id = U == 0 ? AD_INSTR_SADDLV : AD_INSTR_UADDLV;
        }
        else if(opcode == 10){
            instr_s = U == 0 ? "smaxv" : "umaxv";
            instr_id = U == 0 ? AD_INSTR_SMAXV : AD_INSTR_UMAXV;
        }
        else if(opcode == 0x1a){
            instr_s = U == 0 ? "sminv" : "uminv";
            instr_id = U == 0 ? AD_INSTR_SMINV : AD_INSTR_UMINV;
        }
        else{
            if(U == 1)
                return 1;

            instr_s = "addv";
            instr_id = AD_INSTR_ADDV;
        }

        if(opcode == 3){
            Rd_Rtbl = rtbls_o3[size];
            Rd_sz = sizes_o3[size];
        }
        else{
            Rd_Rtbl = rtbls[size];
            Rd_sz = sizes[size];
        }

        if(size == 0)
            T = Q == 0 ? "8b" : "16b";
        else if(size == 1)
            T = Q == 0 ? "4h" : "8h";
        else if(size == 2 && Q == 1)
            T = "4s";
    }
    else{
        if(opcode != 12 && opcode != 15)
            return 1;
            
        if(U == 0 && (size == 1 || size == 3))
            return 1;

        U32 sz = (size & 1);

        if(sz == 1)
            return 1;

        if(U == 0){
            T = Q == 0 ? "4h" : "8h";

            Rd_Rtbl = AD_RTBL_FP_16;
            Rd_sz = _16_BIT;
        }
        else{
            if(Q == 1)
                T = "4s";

            Rd_Rtbl = AD_RTBL_FP_32;
            sz = _32_BIT;
        }

        U32 s = size >> 1;
        U32 compar = U == 0 ? size : s;
       
        if(opcode == 12){
            instr_s = compar == 0 ? "fmaxnmv" : "fminnmv";
            instr_id = compar == 0 ? AD_INSTR_FMAXNMV : AD_INSTR_FMINNMV;
        }
        else{
            instr_s = compar == 0 ? "fmaxv" : "fminv";
            instr_id = compar == 0 ? AD_INSTR_FMAXV : AD_INSTR_FMINV;
        }
    }

    if(!Rd_Rtbl || !T)
        return 1;

    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    concat(&DECODE_STR(out), "%s %s, %s.%s", instr_s, Rd_s, Rn_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCryptographicThreeRegisterImm2Instr(struct instruction *i,
        struct ad_insn *out){
    U32 Rm = bits(i->opcode, 16, 20);
    U32 imm2 = bits(i->opcode, 12, 13);
    U32 opcode = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Rm);
    ADD_FIELD(out, imm2);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "sm3tt1a", AD_INSTR_SM3TT1A }, { "sm3tt1b", AD_INSTR_SM3TT1B },
        { "sm3tt2a", AD_INSTR_SM3TT2A }, { "sm3tt2b", AD_INSTR_SM3TT2B }
    };

    const char *instr_s = tab[opcode].instr_s;
    S32 instr_id = tab[opcode].instr_id;

    const char **rtbl = AD_RTBL_FP_V_128;
    U32 sz = _128_BIT;

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm2);

    concat(&DECODE_STR(out), "%s %s.4s, %s.4s, %s.s[%d]", instr_s, Rd_s, Rn_s,
            Rm_s, imm2);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCryptographicThreeRegisterSHA512Instr(struct instruction *i,
        struct ad_insn *out){
    U32 Rm = bits(i->opcode, 16, 20);
    U32 O = bits(i->opcode, 14, 14);
    U32 opcode = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Rm);
    ADD_FIELD(out, O);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    U32 idx = (O << 2) | opcode;

    struct itab tab[] = {
        { "sha512h", AD_INSTR_SHA512H }, { "sha512h2", AD_INSTR_SHA512H2 },
        { "sha512su1", AD_INSTR_SHA512SU1 }, { "rax1", AD_INSTR_RAX1 },
        { "sm3partw1", AD_INSTR_SM3PARTW1 }, { "sm3partw2", AD_INSTR_SM3PARTW2 },
        { "sm4ekey", AD_INSTR_SM4EKEY }
    };

    if(OOB(idx, tab))
        return 1;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;

    if(instr_id == AD_INSTR_SHA512H || instr_id == AD_INSTR_SHA512H2){
        Rd_Rtbl = AD_RTBL_FP_128;
        Rn_Rtbl = AD_RTBL_FP_128;
    }
    else{
        Rd_Rtbl = AD_RTBL_FP_V_128;
        Rn_Rtbl = AD_RTBL_FP_V_128;
    }

    const char *T = NULL;
    S32 T_on_all = 1;

    if(instr_id != AD_INSTR_SM3PARTW1 && instr_id != AD_INSTR_SM3PARTW2 &&
            instr_id != AD_INSTR_SM4EKEY){
        T = "2d";
        T_on_all = (instr_id != AD_INSTR_SHA512H && instr_id != AD_INSTR_SHA512H2);
    }
    else{
        T = "4s";
    }

    const char **Rm_Rtbl = AD_RTBL_FP_V_128;

    U32 sz = _128_BIT;

    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    const char *Rn_s = GET_FP_REG(Rn_Rtbl, Rn);
    const char *Rm_s = GET_FP_REG(Rm_Rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rm_Rtbl);

    concat(&DECODE_STR(out), "%s", instr_s);

    if(T_on_all)
        concat(&DECODE_STR(out), " %s.%s, %s.%s, %s.%s", Rd_s, T, Rn_s, T, Rm_s, T);
    else
        concat(&DECODE_STR(out), " %s, %s, %s.%s", Rd_s, Rn_s, Rm_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCryptographicFourRegisterInstr(struct instruction *i,
        struct ad_insn *out){
    U32 Op0 = bits(i->opcode, 21, 22);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 Ra = bits(i->opcode, 10, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(Op0 == 3)
        return 1;

    ADD_FIELD(out, Op0);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, Ra);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    const char *T = NULL;

    if(Op0 == 0){
        instr_s = "eor3";
        instr_id = AD_INSTR_EOR3;

        T = "16b";
    }
    else if(Op0 == 1){
        instr_s = "bcax";
        instr_id = AD_INSTR_BCAX;

        T = "16b";
    }
    else{
        instr_s = "sm3ss1";
        instr_id = AD_INSTR_SM3SS1;

        T = "4s";
    }

    const char *Rd_s = GET_FP_REG(AD_RTBL_FP_V_128, Rd);
    const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);
    const char *Rm_s = GET_FP_REG(AD_RTBL_FP_V_128, Rm);
    const char *Ra_s = GET_FP_REG(AD_RTBL_FP_V_128, Ra);
    
    ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Rm, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Ra, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    concat(&DECODE_STR(out), "%s %s.%s, %s.%s, %s.%s, %s.%s", instr_s, Rd_s, T,
            Rn_s, T, Rm_s, T, Ra_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleXARInstr(struct instruction *i, struct ad_insn *out){
    U32 Rm = bits(i->opcode, 16, 20);
    U32 imm6 = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, Rm);
    ADD_FIELD(out, imm6);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *Rd_s = GET_FP_REG(AD_RTBL_FP_V_128, Rd);
    const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);
    const char *Rm_s = GET_FP_REG(AD_RTBL_FP_V_128, Rm);

    ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Rm, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm6);

    concat(&DECODE_STR(out), "xar %s.2d, %s.2d, %s.2d, #"S_X"", Rd_s, Rn_s,
            Rm_s, S_A(imm6));

    SET_INSTR_ID(out, AD_INSTR_XAR);

    return 0;
}

static S32 DisassembleCryptographicTwoRegisterSHA512Instr(struct instruction *i,
        struct ad_insn *out){
    U32 opcode = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(opcode > 1)
        return 1;

    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;
    const char *T = NULL;

    if(opcode == 0){
        instr_s = "sha512su0";
        instr_id = AD_INSTR_SHA512SU0;

        T = "2d";
    }
    else{
        instr_s = "sm4e";
        instr_id = AD_INSTR_SM4E;

        T = "4s";
    }

    const char *Rd_s = GET_FP_REG(AD_RTBL_FP_V_128, Rd);
    const char *Rn_s = GET_FP_REG(AD_RTBL_FP_V_128, Rn);

    ADD_REG_OPERAND(out, Rd, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));
    ADD_REG_OPERAND(out, Rn, _SZ(_128_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_FP_V_128));

    concat(&DECODE_STR(out), "%s %s.%s, %s.%s", instr_s, Rd_s, T, Rn_s, T);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleConversionBetweenFloatingPointAndFixedPointInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 rmode = bits(i->opcode, 19, 20);
    U32 opcode = bits(i->opcode, 16, 18);
    U32 scale = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(opcode > 3)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, rmode);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, scale);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(opcode == 0){
        instr_s = "fcvtzs";
        instr_id = AD_INSTR_FCVTZS;
    }
    else if(opcode == 1){
        instr_s = "fcvtzu";
        instr_id = AD_INSTR_FCVTZU;
    }
    else if(opcode == 2){
        instr_s = "scvtf";
        instr_id = AD_INSTR_SCVTF;
    }
    else if(opcode == 3){
        instr_s = "ucvtf";
        instr_id = AD_INSTR_UCVTF;
    }

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;

    U32 Rd_sz = 0;
    U32 Rn_sz = 0;

    U32 ftype = ptype;

    if(ftype == 2)
        return 1;

    if(instr_id == AD_INSTR_FCVTZS || instr_id == AD_INSTR_FCVTZU){
        Rd_Rtbl = sf == 0 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
        Rd_sz = sf == 0 ? _32_BIT : _64_BIT;

        if(ftype == 0){
            Rn_Rtbl = AD_RTBL_FP_32;
            Rn_sz = _32_BIT;
        }
        else if(ftype == 1){
            Rn_Rtbl = AD_RTBL_FP_64;
            Rn_sz = _64_BIT;
        }
        else{
            Rn_Rtbl = AD_RTBL_FP_16;
            Rn_sz = _16_BIT;
        }
    }
    else{
        Rn_Rtbl = sf == 0 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
        Rn_sz = sf == 0 ? _32_BIT : _64_BIT;

        if(ftype == 0){
            Rd_Rtbl = AD_RTBL_FP_32;
            Rd_sz = _32_BIT;
        }
        else if(ftype == 1){
            Rd_Rtbl = AD_RTBL_FP_64;
            Rd_sz = _64_BIT;
        }
        else{
            Rd_Rtbl = AD_RTBL_FP_16;
            Rd_sz = _16_BIT;
        }
    }

    if(!Rd_Rtbl || !Rn_Rtbl)
        return 1;

    const char *Rd_s = NULL;
    const char *Rn_s = NULL;

    if(instr_id == AD_INSTR_FCVTZS || instr_id == AD_INSTR_FCVTZU){
        Rd_s = GET_GEN_REG(Rd_Rtbl, Rd, PREFER_ZR);
        Rn_s = GET_FP_REG(Rn_Rtbl, Rn);
    }
    else{
        Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
        Rn_s = GET_GEN_REG(Rn_Rtbl, Rn, PREFER_ZR);
    }

    ADD_REG_OPERAND(out, Rd, Rd_sz, PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, Rn_sz, PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);

    U32 fbits = 64 - scale;

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&fbits);

    concat(&DECODE_STR(out), "%s %s, %s, #%#x", instr_s, Rd_s, Rn_s, fbits);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleConversionBetweenFloatingPointAndIntegerInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 rmode = bits(i->opcode, 19, 20);
    U32 opcode = bits(i->opcode, 16, 18);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, rmode);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    U32 operation = (bits(opcode, 1, 2) << 2) | rmode;
    U32 u = (opcode & 1);

    /* if this is zero, S32 to float */
    S32 float_to_int = 0;
    /* append .d[1]? */
    S32 part = 0;

    const char **Flt_Rtbl = NULL;
    const char **Gen_Rtbl = sf == 0 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;

    U32 ftype = ptype;

    U32 intsize = sf == 0 ? _32_BIT : _64_BIT;
    U32 fltsize;

    if(ftype == 0){
        Flt_Rtbl = AD_RTBL_FP_32;
        fltsize = _32_BIT;
    }
    else if(ftype == 1){
        Flt_Rtbl = AD_RTBL_FP_64;
        fltsize = _64_BIT;
    }
    else if(ftype == 2){
        if(operation != 13)
            return 1;

        Flt_Rtbl = AD_RTBL_FP_V_128;
        fltsize = _128_BIT;
    }
    else{
        Flt_Rtbl = AD_RTBL_FP_16;
        fltsize = _16_BIT;
    }

    if((operation & ~3) == 0){
        /* fcvt[npmz][us] */
        if(rmode == 0){
            instr_s = u == 0 ? "fcvtns" : "fcvtnu";
            instr_id = u == 0 ? AD_INSTR_FCVTNS : AD_INSTR_FCVTNU;
        }
        else if(rmode == 1){
            instr_s = u == 0 ? "fcvtps" : "fcvtpu";
            instr_id = u == 0 ? AD_INSTR_FCVTPS : AD_INSTR_FCVTPU;
        }
        else if(rmode == 2){
            instr_s = u == 0 ? "fcvtms" : "fcvtmu";
            instr_id = u == 0 ? AD_INSTR_FCVTMS : AD_INSTR_FCVTMU;
        }
        else{
            instr_s = u == 0 ? "fcvtzs" : "fcvtzu";
            instr_id = u == 0 ? AD_INSTR_FCVTZS : AD_INSTR_FCVTZU;
        }

        float_to_int = 1;
    }
    else if(operation == 4){
        instr_s = u == 0 ? "scvtf" : "ucvtf";
        instr_id = u == 0 ? AD_INSTR_SCVTF : AD_INSTR_UCVTF;
    }
    else if(operation == 8){
        instr_s = u == 0 ? "fcvtas" : "fcvtau";
        instr_id = u == 0 ? AD_INSTR_FCVTAS : AD_INSTR_FCVTAU;

        float_to_int = 1;
    }
    else if(operation == 12 || operation == 13){
        instr_s = "fmov";
        instr_id = AD_INSTR_FMOV;

        if((opcode & 1) == 0)
            float_to_int = 1;

        if(operation == 13)
            part = 1;
    }
    else if(operation == 15){
        instr_s = "fjcvtzs";
        instr_id = AD_INSTR_FJCVTZS;

        float_to_int = 1;
    }
    else{
        return 1;
    }

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;

    U32 Rd_sz = 0;
    U32 Rn_sz = 0;

    const char *Rd_s = NULL;
    const char *Rn_s = NULL;

    if(float_to_int){
        Rd_Rtbl = Gen_Rtbl;
        Rn_Rtbl = Flt_Rtbl;

        Rd_sz = intsize;
        Rn_sz = fltsize;

        Rd_s = GET_GEN_REG(Rd_Rtbl, Rd, PREFER_ZR);
        Rn_s = GET_FP_REG(Rn_Rtbl, Rn);
    }
    else{
        Rd_Rtbl = Flt_Rtbl;
        Rn_Rtbl = Gen_Rtbl;

        Rd_sz = fltsize;
        Rn_sz = intsize;

        Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
        Rn_s = GET_GEN_REG(Rn_Rtbl, Rn, PREFER_ZR);
    }

    ADD_REG_OPERAND(out, Rd, Rd_sz, PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    if(!float_to_int && part){
        ADD_IMM_OPERAND(out, AD_IMM_UINT, 1);
        concat(&DECODE_STR(out), ".d[1]");
    }

    ADD_REG_OPERAND(out, Rn, Rn_sz, PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);

    concat(&DECODE_STR(out), ", %s", Rn_s);

    if(float_to_int && part){
        ADD_IMM_OPERAND(out, AD_IMM_UINT, 1);
        concat(&DECODE_STR(out), ".d[1]");
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleFloatingPointDataProcessingOneSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 opcode = bits(i->opcode, 15, 20);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(M == 1 || S == 1 || ptype == 2)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    struct itab *tab = NULL;

    struct itab ptype0_tab[] = {
        { "fmov", AD_INSTR_FMOV }, { "fabs", AD_INSTR_FABS },
        { "fneg", AD_INSTR_FNEG }, { "fsqrt", AD_INSTR_FSQRT },
        /* one blank, idx 4 */
        { NULL, AD_NONE },
        { "fcvt", AD_INSTR_FCVT },
        /* one blank, idx 6 */
        { NULL, AD_NONE },
        { "fcvt", AD_INSTR_FCVT }, { "frintn", AD_INSTR_FRINTN },
        { "frintp", AD_INSTR_FRINTP }, { "frintm", AD_INSTR_FRINTM },
        { "frintz", AD_INSTR_FRINTZ }, { "frinta", AD_INSTR_FRINTA },
        /* one blank, idx 13 */
        { NULL, AD_NONE },
        { "frintx", AD_INSTR_FRINTX }, { "frinti", AD_INSTR_FRINTI },
        { "frint32z", AD_INSTR_FRINT32Z }, { "frint32x", AD_INSTR_FRINT32X },
        { "frint64z", AD_INSTR_FRINT64Z }, { "frint64x", AD_INSTR_FRINT64X }
    };

    struct itab ptype1_tab[] = {
        { "fmov", AD_INSTR_FMOV }, { "fabs", AD_INSTR_FABS },
        { "fneg", AD_INSTR_FNEG }, { "fsqrt", AD_INSTR_FSQRT },
        { "fcvt", AD_INSTR_FCVT },
        /* two blanks, idxes [5-6] */
        { NULL, AD_NONE },
        { NULL, AD_NONE },
        { "fcvt", AD_INSTR_FCVT }, { "frintn", AD_INSTR_FRINTN },
        { "frintp", AD_INSTR_FRINTP }, { "frintm", AD_INSTR_FRINTM },
        { "frintz", AD_INSTR_FRINTZ }, { "frinta", AD_INSTR_FRINTA },
        /* one blank, idx 13 */
        { NULL, AD_NONE },
        { "frintx", AD_INSTR_FRINTX }, { "frinti", AD_INSTR_FRINTI },
        { "frint32z", AD_INSTR_FRINT32Z }, { "frint32x", AD_INSTR_FRINT32X },
        { "frint64z", AD_INSTR_FRINT64Z }, { "frint64x", AD_INSTR_FRINT64X }
    };

    struct itab ptype3_tab[] = {
        { "fmov", AD_INSTR_FMOV }, { "fabs", AD_INSTR_FABS },
        { "fneg", AD_INSTR_FNEG }, { "fsqrt", AD_INSTR_FSQRT },
        { "fcvt", AD_INSTR_FCVT },
        { "fcvt", AD_INSTR_FCVT },
        /* two blanks, idxes [6-7] */
        { NULL, AD_NONE },
        { NULL, AD_NONE },
        { "frintn", AD_INSTR_FRINTN },
        { "frintp", AD_INSTR_FRINTP }, { "frintm", AD_INSTR_FRINTM },
        { "frintz", AD_INSTR_FRINTZ }, { "frinta", AD_INSTR_FRINTA },
        /* one blank, idx 13 */
        { NULL, AD_NONE },
        { "frintx", AD_INSTR_FRINTX }, { "frinti", AD_INSTR_FRINTI }
    };

    if(ptype == 0){
        if(OOB(opcode, ptype0_tab))
            return 1;

        tab = ptype0_tab;
    }
    else if(ptype == 1){
        if(OOB(opcode, ptype1_tab))
            return 1;

        tab = ptype1_tab;
    }
    else{
        if(OOB(opcode, ptype3_tab))
            return 1;

        tab = ptype3_tab;
    }

    instr_s = tab[opcode].instr_s;

    if(!instr_s)
        return 1;

    instr_id = tab[opcode].instr_id;

    U32 ftype = ptype;

    const char **Rd_Rtbl = NULL;
    const char **Rn_Rtbl = NULL;

    U32 Rd_sz = 0;
    U32 Rn_sz = 0;

    if(instr_id == AD_INSTR_FCVT){
        U32 opc = bits(i->opcode, 15, 16);

        if(ftype == opc)
            return 1;

        if(ftype == 0){
            Rn_Rtbl = AD_RTBL_FP_32;
            Rn_sz = _32_BIT;
        }
        else if(ftype == 1){
            Rn_Rtbl = AD_RTBL_FP_64;
            Rn_sz = _64_BIT;
        }
        else{
            Rn_Rtbl = AD_RTBL_FP_16;
            Rn_sz = _16_BIT;
        }

        if(opc == 0){
            Rd_Rtbl = AD_RTBL_FP_32;
            Rd_sz = _32_BIT;
        }
        else if(opc == 1){
            Rd_Rtbl = AD_RTBL_FP_64;
            Rd_sz = _64_BIT;
        }
        else{
            Rd_Rtbl = AD_RTBL_FP_16;
            Rd_sz = _16_BIT;
        }
    }
    else{
        if(ftype == 0){
            Rd_Rtbl = Rn_Rtbl = AD_RTBL_FP_32;
            Rd_sz = Rn_sz = _32_BIT;
        }
        else if(ftype == 1){
            Rd_Rtbl = Rn_Rtbl = AD_RTBL_FP_64;
            Rd_sz = Rn_sz = _64_BIT;
        }
        else{
            Rd_Rtbl = Rn_Rtbl = AD_RTBL_FP_16;
            Rd_sz = Rn_sz = _16_BIT;
        }
    }

    const char *Rd_s = GET_FP_REG(Rd_Rtbl, Rd);
    const char *Rn_s = GET_FP_REG(Rn_Rtbl, Rn);

    ADD_REG_OPERAND(out, Rd, Rd_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rd_Rtbl);
    ADD_REG_OPERAND(out, Rn, Rn_sz, NO_PREFER_ZR, _SYSREG(AD_NONE), Rn_Rtbl);

    concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rn_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleFloatingPointCompareInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 op = bits(i->opcode, 14, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 opcode2 = bits(i->opcode, 0, 4);

    if(ptype == 2)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, op);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, opcode2);

    U32 opc = bits(i->opcode, 3, 4);

    S32 signal_all_nans = ((opc >> 1) & 1);
    S32 cmp_with_zero = (opc & 1);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(signal_all_nans){
        instr_s = "fcmpe";
        instr_id = AD_INSTR_FCMPE;
    }
    else{
        instr_s = "fcmp";
        instr_id = AD_INSTR_FCMP;
    }

    const char **rtbl = NULL;
    U32 sz = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
    }

    const char *Rn_s = GET_FP_REG(rtbl, Rn);

    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rn_s);

    if(cmp_with_zero){
        ADD_IMM_OPERAND(out, AD_IMM_FLOAT, 0);
        concat(&DECODE_STR(out), ", #0.0");
    }
    else{
        ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
        const char *Rm_s = GET_FP_REG(rtbl, Rm);

        concat(&DECODE_STR(out), ", %s", Rm_s);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleFloatingPointImmediateInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 imm8 = bits(i->opcode, 13, 20);
    U32 imm5 = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(M == 1 || S == 1 || ptype == 2 || imm5 != 0)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, imm8);
    ADD_FIELD(out, imm5);
    ADD_FIELD(out, Rd);

    const char **rtbl = NULL;
    U32 sz = 0;
    U32 datasize = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
        datasize = 32;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
        datasize = 64;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
        datasize = 16;
    }

    const char *Rd_s = GET_FP_REG(rtbl, Rd);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    U32 imm = VFPExpandImm(imm8);

    F32 immf = *(F32 *)&imm;
    ADD_IMM_OPERAND(out, AD_IMM_FLOAT, imm);

    concat(&DECODE_STR(out), "fmov %s, #%f", Rd_s, immf);

    SET_INSTR_ID(out, AD_INSTR_FMOV);

    return 0;
}

static S32 DisassembleFloatingPointConditionalCompare(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 cond = bits(i->opcode, 12, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 op = bits(i->opcode, 4, 4);
    U32 nzcv = bits(i->opcode, 0, 3);

    if(M == 1 || S == 1 || ptype == 2)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, cond);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, op);
    ADD_FIELD(out, nzcv);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(op == 0){
        instr_s = "fccmp";
        instr_id = AD_INSTR_FCCMP;
    }
    else{
        instr_s = "fccmpe";
        instr_id = AD_INSTR_FCCMPE;
    }

    const char **rtbl = NULL;
    U32 sz = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
    }

    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&nzcv);

    const char *dc = decode_cond(cond);

    concat(&DECODE_STR(out), "%s %s, %s, #%#x, %s", instr_s, Rn_s, Rm_s, nzcv, dc);

    SET_INSTR_ID(out, instr_id);
    SET_CC(out, cond);

    return 0;
}

static S32 DisassembleFloatingPointDataProcessingTwoSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 12, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(M == 1 || S == 1 || ptype == 2)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "fmul", AD_INSTR_FMUL }, { "fdiv", AD_INSTR_FDIV },
        { "fadd", AD_INSTR_FADD }, { "fsub", AD_INSTR_FSUB },
        { "fmax", AD_INSTR_FMAX }, { "fmin", AD_INSTR_FMIN },
        { "fmaxnm", AD_INSTR_FMAXNM }, { "fminnm", AD_INSTR_FMINNM },
        { "fnmul", AD_INSTR_FNMUL }
    };

    if(OOB(opcode, tab))
        return 1;

    const char *instr_s = tab[opcode].instr_s;
    S32 instr_id = tab[opcode].instr_id;

    const char **rtbl = NULL;
    U32 sz = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
    }

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleFloatingPointConditionalSelectInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 cond = bits(i->opcode, 12, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(M == 1 || S == 1 || ptype == 2)
        return 1;

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, cond);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char **rtbl = NULL;
    U32 sz = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
    }

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    const char *dc = decode_cond(cond);

    concat(&DECODE_STR(out), "fcsel %s, %s, %s, %s", Rd_s, Rn_s, Rm_s, dc);

    SET_INSTR_ID(out, AD_INSTR_FCSEL);
    SET_CC(out, cond);

    return 0;
}

static S32 DisassembleFloatingPointDataProcessingThreeSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 M = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 ptype = bits(i->opcode, 22, 23);
    U32 o1 = bits(i->opcode, 21, 21);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 o0 = bits(i->opcode, 15, 15);
    U32 Ra = bits(i->opcode, 10, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, M);
    ADD_FIELD(out, S);
    ADD_FIELD(out, ptype);
    ADD_FIELD(out, o1);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, o0);
    ADD_FIELD(out, Ra);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    if(M == 1 || S == 1 || ptype == 2)
        return 1;

    struct itab tab[] = {
        { "fmadd", AD_INSTR_FMADD }, { "fmsub", AD_INSTR_FMSUB },
        { "fnmadd", AD_INSTR_FNMADD }, { "fnmsub", AD_INSTR_FNMSUB }
    };

    U32 idx = (o1 << 1) | o0;

    if(OOB(idx, tab))
        return 1;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    const char **rtbl = NULL;
    U32 sz = 0;

    U32 ftype = ptype;

    if(ftype == 0){
        rtbl = AD_RTBL_FP_32;
        sz = _32_BIT;
    }
    else if(ftype == 1){
        rtbl = AD_RTBL_FP_64;
        sz = _64_BIT;
    }
    else{
        rtbl = AD_RTBL_FP_16;
        sz = _16_BIT;
    }

    const char *Rd_s = GET_FP_REG(rtbl, Rd);
    const char *Rn_s = GET_FP_REG(rtbl, Rn);
    const char *Rm_s = GET_FP_REG(rtbl, Rm);
    const char *Ra_s = GET_FP_REG(rtbl, Ra);

    ADD_REG_OPERAND(out, Rd, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rn, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Rm, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);
    ADD_REG_OPERAND(out, Ra, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), rtbl);

    concat(&DECODE_STR(out), "%s %s, %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s, Ra_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

S32 DataProcessingFloatingPointDisassemble(struct instruction *i, struct ad_insn *out){
    S32 result = 0;

    U32 op0 = bits(i->opcode, 28, 31);
    U32 op1 = bits(i->opcode, 23, 24);
    U32 op2 = bits(i->opcode, 19, 22);
    U32 op3 = bits(i->opcode, 10, 18);

    if(op0 == 4 && (op1 & ~1) == 0 && (op2 & ~8) == 5 && (op3 & ~0x7c) == 2)
        result = DisassembleCryptographicAESInstr(i, out);
    else if(op0 == 5 && (op1 & ~1) == 0 && (op2 & ~11) == 0 && (op3 & ~0x1dc) == 0)
        result = DisassembleCryptographicThreeRegisterSHAInstr(i, out);
    else if(op0 == 5 && (op1 & ~1) == 0 && (op2 & ~8) == 5 && (op3 & ~0x7c) == 2)
        result = DisassembleCryptographicTwoRegisterSHAInstr(i, out);
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            op1 == 0 &&
            (op2 & ~3) == 0 &&
            (op3 & ~0x1de) == 1){
        S32 scalar = (op0 & 1);
        result = DisassembleAdvancedSIMDCopyInstr(i, out, scalar);
    }
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            (op1 & ~1) == 0 &&
            ((op2 & ~3) == 8 || (op2 & ~11) == 4 || (op2 & ~11) == 0) &&
            ((op3 & ~0x1ce) == 1 || (op3 & ~0x1de) == 0x21 || (op3 & ~0x1fe) == 1)){
        S32 scalar = (op0 & 1);
        S32 fp16 = (op2 >> 2) == 2 && (op3 & ~0x1ce) == 1;
        S32 extra = (op2 & ~11) == 0 && (op3 & ~0x1de) == 0x21;

        result = DisassembleAdvancedSIMDThreeSameInstr(i, out, scalar, fp16, extra);
    }
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            (op1 & ~1) == 0 &&
            ((op2 == 0xf) || (op2 & ~8) == 4) &&
            (op3 & ~0x7c) == 2){
        S32 scalar = (op0 & 1);
        S32 fp16 = (op2 == 0xf);

        result = DisassembleAdvancedSIMDTwoRegisterMiscellaneousInstr(i, out, scalar, fp16);
    }
    else if((op0 & ~2) == 5 && (op1 & ~1) == 0 && (op2 & ~8) == 6 && (op3 & ~0x7c) == 2)
        result = DisassembleAdvancedSIMDScalarPairwiseInstr(i, out);
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            (op1 & ~1) == 0 &&
            (op2 & ~11) == 4 &&
            (op3 & ~0x1fc) == 0){
        S32 scalar = (op0 & 1);
        result = DisassembleAdvancedSIMDThreeDifferentInstr(i, out, scalar);
    }
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            op1 == 2 &&
            (op3 & ~0x1fe) == 1){
        if(op2 == 0)
            result = DisassembleAdvancedSIMDModifiedImmediateInstr(i, out);
        else{
            S32 scalar = (op0 & 1);
            result = DisassembleAdvancedSIMDShiftByImmediateInstr(i, out, scalar);
        }
    }
    else if(((op0 & ~2) == 5 || (op0 & ~6) == 0) &&
            (op1 & ~1) == 2 &&
            (op3 & ~0x1fe) == 0){
        S32 scalar = (op0 & 1);
        result = DisassembleAdvancedSIMDXIndexedElementInstr(i, out, scalar);
    }
    else if((op0 & ~4) == 0 && (op1 & ~1) == 0 && (op2 & ~11) == 0 && (op3 & ~0x1dc) == 0)
        result = DisassembleAdvancedSIMDTableLookupInstr(i, out);
    else if((op0 & ~4) == 0 && (op1 & ~1) == 0 && (op2 & ~11) == 0 && (op3 & ~0x1dc) == 2)
        result = DisassembleAdvancedSIMDPermuteInstr(i, out);
    else if((op0 & ~4) == 2 && (op1 & ~1) == 0 && (op2 & ~11) == 0 && (op3 & ~0x1de) == 0)
        result = DisassembleAdvancedSIMDExtractInstr(i, out);
    else if((op0 & ~6) == 0 && (op1 & ~1) == 0 && (op2 & ~8) == 6 && (op3 & ~0x7c) == 2)
        result = DisassembleAdvancedSIMDAcrossLanesInstr(i, out);
    else if(op0 == 12 && op1 == 0 && (op2 & ~3) == 8 && (op3 & ~0x1cf) == 0x20)
        result = DisassembleCryptographicThreeRegisterImm2Instr(i, out);
    else if(op0 == 12 && op1 == 0 && (op2 & ~3) == 12 && (op3 & ~0x1d3) == 0x20)
        result = DisassembleCryptographicThreeRegisterSHA512Instr(i, out);
    else if(op0 == 12 && op1 == 0 && (op3 & ~0x1df) == 0)
        result = DisassembleCryptographicFourRegisterInstr(i, out);
    else if(op0 == 12 && op1 == 1 && (op2 & ~3) == 0)
        result = DisassembleXARInstr(i, out);
    else if(op0 == 12 && op1 == 1 && op2 == 8 && (op3 & ~3) == 0x20)
        result = DisassembleCryptographicTwoRegisterSHA512Instr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 0)
        result = DisassembleConversionBetweenFloatingPointAndFixedPointInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1c0) == 0)
        result = DisassembleConversionBetweenFloatingPointAndIntegerInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1e0) == 0x10)
        result = DisassembleFloatingPointDataProcessingOneSourceInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1f0) == 8)
        result = DisassembleFloatingPointCompareInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1f8) == 4)
        result = DisassembleFloatingPointImmediateInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1fc) == 1)
        result = DisassembleFloatingPointConditionalCompare(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1fc) == 2)
        result = DisassembleFloatingPointDataProcessingTwoSourceInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 0 && (op2 & ~11) == 4 && (op3 & ~0x1fc) == 3)
        result = DisassembleFloatingPointConditionalSelectInstr(i, out);
    else if((op0 & ~10) == 1 && (op1 & ~1) == 2)
        result = DisassembleFloatingPointDataProcessingThreeSourceInstr(i, out);
    else
        result = 1;

    return result;
}
