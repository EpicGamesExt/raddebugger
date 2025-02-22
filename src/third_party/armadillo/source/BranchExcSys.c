#include <stdio.h>
#include <stdlib.h>

#include "adefs.h"
#include "bits.h"
#include "common.h"
#include "instruction.h"
#include "utils.h"
#include "strext.h"

static S32 DisassembleConditionalImmediateBranchInstr(struct instruction *i,
        struct ad_insn *out){
    U32 o1 = bits(i->opcode, 24, 24);
    U32 imm19 = bits(i->opcode, 5, 23);
    U32 o0 = bits(i->opcode, 4, 4);
    U32 cond = bits(i->opcode, 0, 3);

    if(o1 != 0 && o0 != 0)
        return 1;

    ADD_FIELD(out, o1);
    ADD_FIELD(out, imm19);
    ADD_FIELD(out, o0);
    ADD_FIELD(out, cond);

    U64 imm = sign_extend(imm19 << 2, 64) + i->PC;
    const char *dc = decode_cond(cond);

    ADD_IMM_OPERAND(out, AD_IMM_ULONG, *(U64 *)&imm);

    concat(&DECODE_STR(out), "b.%s #%#lx", dc, imm);

    SET_INSTR_ID(out, AD_INSTR_B);
    SET_CC(out, cond);

    return 0;
}

static S32 DisassembleExcGenInstr(struct instruction *i, struct ad_insn *out){
    U32 opc = bits(i->opcode, 21, 23);
    U32 imm16 = bits(i->opcode, 5, 20);
    U32 op2 = bits(i->opcode, 2, 4);
    U32 LL = bits(i->opcode, 0, 1);

    if(op2 != 0)
        return 1;

    ADD_FIELD(out, opc);
    ADD_FIELD(out, imm16);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, LL);

    S32 instr_id = AD_NONE;

    if(opc == 0 && LL > 0){
        struct {
            const char *instr_s;
            S32 instr_id;
        } tab[] = {
            { NULL, AD_NONE },
            { "svc", AD_INSTR_SVC },
            { "hvc", AD_INSTR_HVC },
            { "smc", AD_INSTR_SMC }
        };

        instr_id = tab[LL].instr_id;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm16);

        concat(&DECODE_STR(out), "%s #%#x", tab[LL].instr_s, imm16);
    }
    else if((opc == 1 || opc == 2) && LL == 0){
        struct {
            const char *instr_s;
            S32 instr_id;
        } tab[] = {
            { NULL, AD_NONE },
            { "brk", AD_INSTR_BRK },
            { "hlt", AD_INSTR_HLT }
        };

        instr_id = tab[opc].instr_id;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm16);

        concat(&DECODE_STR(out), "%s #%#x", tab[opc].instr_s, imm16);
    }
    else if(opc == 5 && LL > 0){
        S32 tab[] = { AD_NONE, AD_INSTR_DCPS1, AD_INSTR_DCPS2, AD_INSTR_DCPS3 };
        instr_id = tab[LL];

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm16);

        concat(&DECODE_STR(out), "dcps%d #%#x", LL, imm16);
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleHintInstr(struct instruction *i, struct ad_insn *out){
    U32 CRm = bits(i->opcode, 8, 11);
    U32 op2 = bits(i->opcode, 5, 7);

    ADD_FIELD(out, CRm);
    ADD_FIELD(out, op2);

    S32 instr_id = AD_NONE;

    struct itab first[] = {
        { "nop", AD_INSTR_NOP },
        { "yield", AD_INSTR_YIELD },
        { "wfe", AD_INSTR_WFE },
        { "wfi", AD_INSTR_WFI },
        { "sev", AD_INSTR_SEV },
        { "sevl", AD_INSTR_SEVL },
        { NULL, AD_NONE },
        { "xpaclri", AD_INSTR_XPACLRI }
    };

    struct itab second[] = {
        { "pacia1716", AD_INSTR_PACIA1716 },
        { NULL, AD_NONE },
        { "pacib1716", AD_INSTR_PACIB1716 },
        { NULL, AD_NONE },
        { "autia1716", AD_INSTR_AUTIA1716 },
        { NULL, AD_NONE },
        { "autib1716", AD_INSTR_AUTIB1716 }
    };

    struct itab third[] = {
        { "esb", AD_INSTR_ESB },
        { "psb csync", AD_INSTR_PSB_CSYNC },
        { "tsb csync", AD_INSTR_TSB_CSYNC },
        { "csdb", AD_INSTR_CSDB }
    };

    struct itab fourth[] = {
        { "paciaz", AD_INSTR_PACIAZ },
        { "paciasp", AD_INSTR_PACIASP },
        { "pacibz", AD_INSTR_PACIBZ },
        { "pacibsp", AD_INSTR_PACIBSP },
        { "autiaz", AD_INSTR_AUTIAZ },
        { "autiasp", AD_INSTR_AUTIASP },
        { "autibz", AD_INSTR_AUTIBZ },
        { "autibsp", AD_INSTR_AUTIBSP }
    };

    if(CRm == 4 && (op2 & ~6) == 0){
        instr_id = AD_INSTR_BTI;

        U32 indirection = op2 >> 1;

        const char *tbl[] = { "", " c", " j", " jc" };

        concat(&DECODE_STR(out), "bti%s", tbl[indirection]);
    }
    else{
        struct itab *tab = NULL;

        if(CRm == 0){
            if(OOB(op2, first))
                return 1;

            tab = first;
        }
        else if(CRm == 1){
            if(OOB(op2, second))
                return 1;

            tab = second;
        }
        else if(CRm == 2){
            if(OOB(op2, third))
                return 1;

            tab = third;
        }
        else if(CRm == 3){
            if(OOB(op2, fourth))
                return 1;

            tab = fourth;
        }

        if(!tab)
            return 1;

        const char *instr_s = tab[op2].instr_s;

        if(!instr_s)
            return 1;

        instr_id = tab[op2].instr_id;

        concat(&DECODE_STR(out), "%s", instr_s);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleBarrierInstr(struct instruction *i,
        struct ad_insn *out){
    U32 CRm = bits(i->opcode, 8, 11);
    U32 op2 = bits(i->opcode, 5, 7);
    U32 Rt = bits(i->opcode, 0, 4);

    if(op2 == 0 || op2 == 1)
        return 1;

    if(Rt != 0x1f)
        return 1;

    ADD_FIELD(out, CRm);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    if(op2 == 4 || op2 == 5){
        const char *barrier_ops[] = { "#0x0", "oshld", "oshst", "osh", "#0x4",
            "nshld", "nshst", "nsh", "#0x8", "ishld", "ishst", "ish", "#0xb",
            "ld", "st", "sy"
        };

        const char *instr_s = NULL;

        if(op2 == 5){
            instr_s = "dmb";
            instr_id = AD_INSTR_DMB;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);
        }
        else if((CRm & ~4) != 0){
            instr_s = "dsb";
            instr_id = AD_INSTR_DSB;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);
        }
        else if(CRm == 0){
            instr_s = "ssbb";
            instr_id = AD_INSTR_SSBB;
        }
        else if(CRm == 4){
            instr_s = "pssbb";
            instr_id = AD_INSTR_PSSBB;
        }

        if(!instr_s)
            return 1;

        concat(&DECODE_STR(out), "%s", instr_s);

        if(instr_id == AD_INSTR_DSB || instr_id == AD_INSTR_DMB)
            concat(&DECODE_STR(out), " %s", barrier_ops[CRm]);
    }
    else if(op2 == 2){
        instr_id = AD_INSTR_CLREX;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);

        concat(&DECODE_STR(out), "clrex");

        if(CRm != 0)
            concat(&DECODE_STR(out), " #%#x", CRm);
    }
    else if(op2 == 6){
        instr_id = AD_INSTR_ISB;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);

        concat(&DECODE_STR(out), "isb");

        if(CRm == 0xf)
            concat(&DECODE_STR(out), " sy");
        else
            concat(&DECODE_STR(out), " #%#x", CRm);
    }
    else if(op2 == 0xf){
        instr_id = AD_INSTR_SB;

        concat(&DECODE_STR(out), "sb");
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassemblePSTATEInstr(struct instruction *i, struct ad_insn *out){
    U32 op1 = bits(i->opcode, 16, 18);
    U32 CRm = bits(i->opcode, 8, 11);
    U32 op2 = bits(i->opcode, 5, 7);
    U32 Rt = bits(i->opcode, 0, 4);

    if(Rt != 0x1f)
        return 1;

    ADD_FIELD(out, op1);
    ADD_FIELD(out, CRm);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    if(op1 == 0 && (op2 == 0 || op2 == 1 || op2 == 2)){
        struct {
            const char *instr_s;
            S32 instr_id;
        } tab[] = {
            { "cfinv", AD_INSTR_CFINV },
            { "xaflag", AD_INSTR_XAFLAG },
            { "axflag", AD_INSTR_AXFLAG }
        };

        if(OOB(op2, tab))
            return 1;

        instr_id = tab[op2].instr_id;

        concat(&DECODE_STR(out), "%s", tab[op2].instr_s);
    }
    else{
        instr_id = AD_INSTR_MSR;

        U32 pstatefield = (op1 << 3) | op2;

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&pstatefield);
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);

        /* easier to split cases up like this rather than testing pstatefield */
        if(op1 == 0){
            const char *ptbl[] = { NULL, NULL, NULL, "UAO", "PAN", "SPSel" };

            if(OOB(op2, ptbl))
                return 1;

            if(!ptbl[op2])
                return 1;

            concat(&DECODE_STR(out), "msr %s", ptbl[op2]);
        }
        else{
            const char *ptbl[] = { NULL, "SSBS", "DIT", NULL, "TCO", NULL,
                "DAIFSet", "DAIFClr"
            };

            if(OOB(op2, ptbl))
                return 1;

            if(!ptbl[op2])
                return 1;

            concat(&DECODE_STR(out), "msr %s", ptbl[op2]);
        }

        concat(&DECODE_STR(out), ", #%#x", CRm);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

enum {
    Sys_AT = 0, Sys_DC, Sys_IC, Sys_TLBI, Sys_SYS
};

static S32 SysOp(U32 op1, U32 CRn, U32 CRm, U32 op2){
    U32 encoding = (op1 << 11);
    encoding |= (CRn << 7);
    encoding |= (CRm << 3);
    encoding |= op2;

    switch(encoding){
        case 0x3c0: return Sys_AT;
        case 0x23c0: return Sys_AT;
        case 0x33c0: return Sys_AT;
        case 0x3c1: return Sys_AT;
        case 0x23c1: return Sys_AT;
        case 0x33c1: return Sys_AT;
        case 0x3c2: return Sys_AT;
        case 0x3c3: return Sys_AT;
        case 0x23c4: return Sys_AT;
        case 0x23c5: return Sys_AT;
        case 0x23c6: return Sys_AT;
        case 0x23c7: return Sys_AT;
        case 0x1ba1: return Sys_DC;
        case 0x3b1: return Sys_DC;
        case 0x3b2: return Sys_DC;
        case 0x1bd1: return Sys_DC;
        case 0x3d2: return Sys_DC;
        case 0x1bd9: return Sys_DC;
        case 0x1bf1: return Sys_DC;
        case 0x3f2: return Sys_DC;
        case 0x1be9: return Sys_DC;
        case 0x388: return Sys_IC;
        case 0x3a8: return Sys_IC;
        case 0x1ba9: return Sys_IC;
        case 0x2401: return Sys_TLBI;
        case 0x2405: return Sys_TLBI;
        case 0x418: return Sys_TLBI;
        case 0x2418: return Sys_TLBI;
        case 0x3418: return Sys_TLBI;
        case 0x419: return Sys_TLBI;
        case 0x2419: return Sys_TLBI;
        case 0x3419: return Sys_TLBI;
        case 0x41a: return Sys_TLBI;
        case 0x41b: return Sys_TLBI;
        case 0x241c: return Sys_TLBI;
        case 0x41d: return Sys_TLBI;
        case 0x241d: return Sys_TLBI;
        case 0x341d: return Sys_TLBI;
        case 0x241e: return Sys_TLBI;
        case 0x41f: return Sys_TLBI;
        case 0x2421: return Sys_TLBI;
        case 0x2425: return Sys_TLBI;
        case 0x438: return Sys_TLBI;
        case 0x2438: return Sys_TLBI;
        case 0x3438: return Sys_TLBI;
        case 0x439: return Sys_TLBI;
        case 0x2439: return Sys_TLBI;
        case 0x3439: return Sys_TLBI;
        case 0x43a: return Sys_TLBI;
        case 0x43b: return Sys_TLBI;
        case 0x243c: return Sys_TLBI;
        case 0x43d: return Sys_TLBI;
        case 0x243d: return Sys_TLBI;
        case 0x343d: return Sys_TLBI;
        case 0x243e: return Sys_TLBI;
        case 0x43f: return Sys_TLBI;
        default: return Sys_SYS;
    };
}

static const char *at_op(U32 encoding){
    switch(encoding){
        case 0x0: return "S1E1R";
        case 0x1: return "S1E1W";
        case 0x2: return "S1E0R";
        case 0x3: return "S1E0W";
        case 0x40: return "S1E2R";
        case 0x41: return "S1E2W";
        case 0x44: return "S12E1R";
        case 0x45: return "S12E1W";
        case 0x46: return "S12E0R";
        case 0x47: return "S12E0W";
        case 0x60: return "S1E3R";
        case 0x61: return "S1E3W";
        case 0x8: return "S1E1RP";
        case 0x9: return "S1E1WP";
        default: return NULL;
    };
}

static const char *dc_op(U32 encoding){
    switch(encoding){
        case 0x31: return "IVAC";
        case 0x32: return "ISW";
        case 0x52: return "CSW";
        case 0x72: return "CISW";
        case 0x1a1: return "ZVA";
        case 0x1d1: return "CVAC";
        case 0x1d9: return "CVAU";
        case 0x1f1: return "CIVAC";
        case 0x33: return "IGVAC";
        case 0x34: return "IGSW";
        case 0x35: return "IGDVAC";
        case 0x36: return "IGDSW";
        case 0x54: return "CGSW";
        case 0x56: return "CGDSW";
        case 0x74: return "CIGSW";
        case 0x76: return "CIGDSW";
        case 0x1a3: return "GVA";
        case 0x1a4: return "GZVA";
        case 0x1d3: return "CGVAC";
        case 0x1d5: return "CGDVAC";
        case 0x1e3: return "CGVAP";
        case 0x1e5: return "CGDVAP";
        case 0x1eb: return "CGVADP";
        case 0x1ed: return "CGDVADP";
        case 0x1f3: return "CIGVAC";
        case 0x1f5: return "CIGDVAC";
        case 0x1e1: return "CVAP";
        case 0x1e9: return "CVADP";
        default: return NULL;
    };
};

static const char *ic_op(U32 encoding){
    switch(encoding){
        case 0x8: return "IALLUIS";
        case 0x28: return "IALLU";
        case 0x1a9: return "IVAU";
        default: return NULL;
    };
}

static const char *tlbi_op(U32 encoding){
    switch(encoding){
        case 0x18: return "VMALLE1IS";
        case 0x19: return "VAE1IS";
        case 0x1a: return "ASIDE1IS";
        case 0x1b: return "VAAE1IS";
        case 0x1d: return "VALE1IS";
        case 0x1f: return "VAALE1IS";
        case 0x38: return "VMALLE1";
        case 0x39: return "VAE1";
        case 0x3a: return "ASIDE1";
        case 0x3b: return "VAAE1";
        case 0x3d: return "VALE1";
        case 0x3f: return "VAALE1";
        case 0x201: return "IPAS2E1IS";
        case 0x205: return "IPAS2LE1IS";
        case 0x218: return "ALLE2IS";
        case 0x219: return "VAE2IS";
        case 0x21c: return "ALLE1IS";
        case 0x21d: return "VALE2IS";
        case 0x21e: return "VMALLS12E1IS";
        case 0x221: return "IPAS2E1";
        case 0x225: return "IPAS2LE1";
        case 0x238: return "ALLE2";
        case 0x239: return "VAE2";
        case 0x23c: return "ALLE1";
        case 0x23d: return "VALE2";
        case 0x23e: return "VMALLS12E1";
        case 0x318: return "ALLE3IS";
        case 0x319: return "VAE3IS";
        case 0x31d: return "VALE3IS";
        case 0x338: return "ALLE3";
        case 0x339: return "VAE3";
        case 0x33d: return "VALE3";
        case 0x8: return "VMALLE1OS";
        case 0x9: return "VAE1OS";
        case 0xa: return "ASIDE1OS";
        case 0xb: return "VAAE1OS";
        case 0xd: return "VALE1OS";
        case 0xf: return "VAALE1OS";
        case 0x11: return "RVAE1IS";
        case 0x13: return "RVAAE1IS";
        case 0x15: return "RVALE1IS";
        case 0x17: return "RVAALE1IS";
        case 0x29: return "RVAE1OS";
        case 0x2b: return "RVAAE1OS";
        case 0x2d: return "RVALE1OS";
        case 0x2f: return "RVAALE1OS";
        case 0x31: return "RVAE1";
        case 0x33: return "RVAAE1";
        case 0x35: return "RVALE1";
        case 0x37: return "RVAALE1";
        case 0x202: return "RIPAS2E1IS";
        case 0x206: return "RIPAS2LE1IS";
        case 0x208: return "ALLE2OS";
        case 0x209: return "VAE2OS";
        case 0x20c: return "ALLE1OS";
        case 0x20d: return "VALE2OS";
        case 0x20e: return "VMALLS12E1OS";
        case 0x211: return "RVAE2IS";
        case 0x215: return "RVALE2IS";
        case 0x220: return "IPAS2E1OS";
        default: return NULL;
    };
}

static S32 DisassembleSystemInstr(struct instruction *i, struct ad_insn *out){
    U32 L = bits(i->opcode, 21, 21);
    U32 op1 = bits(i->opcode, 16, 18);
    U32 CRn = bits(i->opcode, 12, 15);
    U32 CRm = bits(i->opcode, 8, 11);
    U32 op2 = bits(i->opcode, 5, 7);
    U32 Rt = bits(i->opcode, 0, 4);

    if(Rt > AD_RTBL_GEN_64_SZ)
        return 1;

    ADD_FIELD(out, L);
    ADD_FIELD(out, op1);
    ADD_FIELD(out, CRn);
    ADD_FIELD(out, CRm);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rt);

    S32 instr_id = AD_NONE;

    if(L == 0){
        if(CRn == 7 && (CRm >> 1) == 4 && SysOp(op1, 7, CRm, op2) == Sys_AT){
            instr_id = AD_INSTR_AT;

            U32 encoding = (op1 << 4);
            encoding |= ((CRm & 1) << 3);
            encoding |= op2;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&encoding);
            ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

            const char *at_op_s = at_op(encoding);

            if(!at_op_s)
                return 1;

            const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

            concat(&DECODE_STR(out), "at %s, %s", at_op_s, Rt_s);
        }
        else if(op1 == 3 && CRn == 7 && CRm == 3){
            const char *instr_s = NULL;

            if(op2 == 2){
                instr_s = "cfp";
                instr_id = AD_INSTR_CFP;
            }
            else if(op2 == 5){
                instr_s = "dvp";
                instr_id = AD_INSTR_DVP;
            }
            else if(op2 == 7){
                instr_s = "cpp";
                instr_id = AD_INSTR_CPP;
            }
            else{
                return 1;
            }

            ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

            const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

            concat(&DECODE_STR(out), "%s rctx, %s", instr_s, Rt_s);
        }
        else if(CRn == 7 && SysOp(op1, 7, CRm, op2) == Sys_DC){
            instr_id = AD_INSTR_DC;

            U32 encoding = (op1 << 7);
            encoding |= (CRm << 3);
            encoding |= op2;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&encoding);
            ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

            const char *dc_op_s = dc_op(encoding);

            if(!dc_op_s)
                return 1;

            const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

            concat(&DECODE_STR(out), "dc %s, %s", dc_op_s, Rt_s);
        }
        else if(CRn == 7 && SysOp(op1, 7, CRm, op2) == Sys_IC){
            instr_id = AD_INSTR_IC;

            U32 encoding = (op1 << 7);
            encoding |= (CRm << 3);
            encoding |= op2;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&encoding);
            ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

            const char *ic_op_s = ic_op(encoding);

            if(!ic_op_s)
                return 1;

            const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

            concat(&DECODE_STR(out), "ic %s, %s", ic_op_s, Rt_s);
        }
        else if(CRn == 8 && SysOp(op1, 8, CRm, op2) == Sys_TLBI){
            instr_id = AD_INSTR_TLBI;

            U32 encoding = (op1 << 7);
            encoding |= (CRm << 3);
            encoding |= op2;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&encoding);
            ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

            const char *tlbi_op_s = tlbi_op(encoding);

            if(!tlbi_op_s)
                return 1;

            const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

            concat(&DECODE_STR(out), "tlbi %s, %s", tlbi_op_s, Rt_s);
        }
        else{
            instr_id = AD_INSTR_SYS;

            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&op1);
            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRn);
            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);
            ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&op2);

            concat(&DECODE_STR(out), "sys #%#x, C%d, C%d, #%#x",
                    op1, CRn, CRm, op2);

            if(Rt != 0x1f){
                ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

                const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

                concat(&DECODE_STR(out), ", %s", Rt_s);
            }
        }
    }
    else{
        instr_id = AD_INSTR_SYSL;

        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&op1);
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRn);
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&CRm);
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&op2);

        const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, NO_PREFER_ZR);

        concat(&DECODE_STR(out), "sysl %s, #%#x, C%d, C%d, #%#x", Rt_s, op1,
                CRn, CRm, op2);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static const char *get_sysreg(U32 encoding){
    switch(encoding){
        case 0xc081: return "ACTLR_EL1";
        case 0xe081: return "ACTLR_EL2";
        case 0xf081: return "ACTLR_EL3";
        case 0xc288: return "AFSR0_EL1";
        case 0xea88: return "AFSR0_EL12";
        case 0xe288: return "AFSR0_EL2";
        case 0xf288: return "AFSR0_EL3";
        case 0xc289: return "AFSR1_EL1";
        case 0xea89: return "AFSR1_EL12";
        case 0xe289: return "AFSR1_EL2";
        case 0xf289: return "AFSR1_EL3";
        case 0xc807: return "AIDR_EL1";
        case 0xc518: return "AMAIR_EL1";
        case 0xed18: return "AMAIR_EL12";
        case 0xe518: return "AMAIR_EL2";
        case 0xf518: return "AMAIR_EL3";
        case 0xde91: return "AMCFGR_EL0";
        case 0xde92: return "AMCGCR_EL0";
        case 0xde94: return "AMCNTENCLR0_EL0";
        case 0xde98: return "AMCNTENCLR1_EL0";
        case 0xde95: return "AMCNTENSET0_EL0";
        case 0xde99: return "AMCNTENSET1_EL0";
        case 0xde90: return "AMCR_EL0";
        case 0xde93: return "AMUSERENR_EL0";
        case 0xc111: return "APDAKeyHi_EL1";
        case 0xc110: return "APDAKeyLo_EL1";
        case 0xc113: return "APDBKeyHi_EL1";
        case 0xc112: return "APDBKeyLo_EL1";
        case 0xc119: return "APGAKeyHi_EL1";
        case 0xc118: return "APGAKeyLo_EL1";
        case 0xc109: return "APIAKeyHi_EL1";
        case 0xc108: return "APIAKeyLo_EL1";
        case 0xc10b: return "APIBKeyHi_EL1";
        case 0xc10a: return "APIBKeyLo_EL1";
        case 0xc802: return "CCSIDR2_EL1";
        case 0xc800: return "CCSIDR_EL1";
        case 0xc801: return "CLIDR_EL1";
        case 0xdf00: return "CNTFRQ_EL0";
        case 0xe708: return "CNTHCTL_EL2";
        case 0xe729: return "CNTHPS_CTL_EL2";
        case 0xe72a: return "CNTHPS_CVAL_EL2";
        case 0xe728: return "CNTHPS_TVAL_EL2";
        case 0xe711: return "CNTHP_CTL_EL2";
        case 0xe712: return "CNTHP_CVAL_EL2";
        case 0xe710: return "CNTHP_TVAL_EL2";
        case 0xe721: return "CNTHVS_CTL_EL2";
        case 0xe722: return "CNTHVS_CVAL_EL2";
        case 0xe720: return "CNTHVS_TVAL_EL2";
        case 0xe719: return "CNTHV_CTL_EL2";
        case 0xe71a: return "CNTHV_CVAL_EL2";
        case 0xe718: return "CNTHV_TVAL_EL2";
        case 0xc708: return "CNTKCTL_EL1";
        case 0xdf01: return "CNTPCT_EL0";
        case 0xff11: return "CNTPS_CTL_EL1";
        case 0xff12: return "CNTPS_CVAL_EL1";
        case 0xff10: return "CNTPS_TVAL_EL1";
        case 0xdf11: return "CNTP_CTL_EL0";
        case 0xef11: return "CNTP_CTL_EL02";
        case 0xdf12: return "CNTP_CVAL_EL0";
        case 0xef12: return "CNTP_CVAL_EL02";
        case 0xdf10: return "CNTP_TVAL_EL0";
        case 0xef10: return "CNTP_TVAL_EL02";
        case 0xdf02: return "CNTVCT_EL0";
        case 0xe703: return "CNTVOFF_EL2";
        case 0xdf19: return "CNTV_CTL_EL0";
        case 0xef19: return "CNTV_CTL_EL02";
        case 0xdf1a: return "CNTV_CVAL_EL0";
        case 0xef1a: return "CNTV_CVAL_EL02";
        case 0xdf18: return "CNTV_TVAL_EL0";
        case 0xef18: return "CNTV_TVAL_EL02";
        case 0xc681: return "CONTEXTIDR_EL1";
        case 0xee81: return "CONTEXTIDR_EL12";
        case 0xe681: return "CONTEXTIDR_EL2";
        case 0xc082: return "CPACR_EL1";
        case 0xe882: return "CPACR_EL12";
        case 0xe08a: return "CPTR_EL2";
        case 0xf08a: return "CPTR_EL3";
        case 0xd000: return "CSSELR_EL1";
        case 0xd801: return "CTR_EL0";
        case 0xc212: return "CurrentEL";
        case 0xe180: return "DACR32_EL2";
        case 0xda11: return "DAIF";
        case 0x83f6: return "DBGAUTHSTATUS_EL1";
        case 0x83ce: return "DBGCLAIMCLR_EL1";
        case 0x83c6: return "DBGCLAIMSET_EL1";
        case 0x9828: return "DBGDTRRX_EL0"; /* DBGDTRTX_EL0 has same encoding */
        case 0x9820: return "DBGDTR_EL0";
        case 0x80a4: return "DBGPRCR_EL1";
        case 0xa038: return "DBGVCR32_EL2";
        case 0xd807: return "DCZID_EL0";
        case 0xc609: return "DISR_EL1";
        case 0xda15: return "DIT";
        case 0xda29: return "DLR_EL0";
        case 0xda28: return "DSPSR_EL0";
        case 0xc201: return "ELR_EL1";
        case 0xea01: return "ELR_EL12";
        case 0xe201: return "ELR_EL2";
        case 0xf201: return "ELR_EL3";
        case 0xc298: return "ERRIDR_EL1";
        case 0xc299: return "ERRSELR_EL1";
        case 0xc2a3: return "ERXADDR_EL1";
        case 0xc2a1: return "ERXCTLR_EL1";
        case 0xc2a0: return "ERXFR_EL1";
        case 0xc2a8: return "ERXMISC0_EL1";
        case 0xc2a9: return "ERXMISC1_EL1";
        case 0xc2aa: return "ERXMISC2_EL1";
        case 0xc2ab: return "ERXMISC3_EL1";
        case 0xc2a6: return "ERXPFGCDN_EL1";
        case 0xc2a5: return "ERXPFGCTL_EL1";
        case 0xc2a4: return "ERXPFGF_EL1";
        case 0xc2a2: return "ERXSTATUS_EL1";
        case 0xc290: return "ESR_EL1";
        case 0xea90: return "ESR_EL12";
        case 0xe290: return "ESR_EL2";
        case 0xf290: return "ESR_EL3";
        case 0xc300: return "FAR_EL1";
        case 0xeb00: return "FAR_EL12";
        case 0xe300: return "FAR_EL2";
        case 0xf300: return "FAR_EL3";
        case 0xd184: return "FPCR";
        case 0xe298: return "FPEXC32_EL2";
        case 0xd194: return "FPSR";
        case 0xc086: return "GCR_EL1";
        case 0xcc0: return "GMID_EL1";
        case 0xe08f: return "HACR_EL2";
        case 0xe088: return "HCR_EL2";
        case 0xe304: return "HPFAR_EL2";
        case 0xe08b: return "HSTR_EL2";
        case 0xc02c: return "ID_AA64AFR0_EL1";
        case 0xc02d: return "ID_AA64AFR1_EL1";
        case 0xc028: return "ID_AA64DFR0_EL1";
        case 0xc029: return "ID_AA64DFR1_EL1";
        case 0xc030: return "ID_AA64ISAR0_EL1";
        case 0xc031: return "ID_AA64ISAR1_EL1";
        case 0xc038: return "ID_AA64MMFR0_EL1";
        case 0xc039: return "ID_AA64MMFR1_EL1";
        case 0xc03a: return "ID_AA64MMFR2_EL1";
        case 0xc020: return "ID_AA64PFR0_EL1";
        case 0xc021: return "ID_AA64PFR1_EL1";
        case 0xc00b: return "ID_AFR0_EL1";
        case 0xc00a: return "ID_DFR0_EL1";
        case 0xc010: return "ID_ISAR0_EL1";
        case 0xc011: return "ID_ISAR1_EL1";
        case 0xc012: return "ID_ISAR2_EL1";
        case 0xc013: return "ID_ISAR3_EL1";
        case 0xc014: return "ID_ISAR4_EL1";
        case 0xc015: return "ID_ISAR5_EL1";
        case 0xc017: return "ID_ISAR6_EL1";
        case 0xc00c: return "ID_MMFR0_EL1";
        case 0xc00d: return "ID_MMFR1_EL1";
        case 0xc00e: return "ID_MMFR2_EL1";
        case 0xc00f: return "ID_MMFR3_EL1";
        case 0xc016: return "ID_MMFR4_EL1";
        case 0xc008: return "ID_PFR0_EL1";
        case 0xc009: return "ID_PFR1_EL1";
        case 0xc01c: return "ID_PFR2_EL1";
        case 0xe281: return "IFSR32_EL2";
        case 0xc608: return "ISR_EL1";
        case 0xc523: return "LORC_EL1";
        case 0xc521: return "LOREA_EL1";
        case 0xc527: return "LORID_EL1";
        case 0xc522: return "LORN_EL1";
        case 0xc520: return "LORSA_EL1";
        case 0xc510: return "MAIR_EL1";
        case 0xed10: return "MAIR_EL12";
        case 0xe510: return "MAIR_EL2";
        case 0xf510: return "MAIR_EL3";
        case 0x8010: return "MDCCINT_EL1";
        case 0x9808: return "MDCCSR_EL0";
        case 0xe089: return "MDCR_EL2";
        case 0xf099: return "MDCR_EL3";
        case 0x8080: return "MDRAR_EL1";
        case 0x8012: return "MDSCR_EL1";
        case 0xc000: return "MIDR_EL1";
        case 0xc005: return "MPIDR_EL1";
        case 0xc018: return "MVFR0_EL1";
        case 0xc019: return "MVFR1_EL1";
        case 0xc01a: return "MVFR2_EL1";
        case 0xda10: return "NZCV";
        case 0x809c: return "OSDLR_EL1";
        case 0x8002: return "OSDTRRX_EL1";
        case 0x801a: return "OSDTRTX_EL1";
        case 0x8032: return "OSECCR_EL1";
        case 0x8084: return "OSLAR_EL1";
        case 0x808c: return "OSLSR_EL1";
        case 0xc213: return "PAN";
        case 0xc3a0: return "PAR_EL1";
        case 0xc4d7: return "PMBIDR_EL1";
        case 0xc4d0: return "PMBLIMITR_EL1";
        case 0xc4d1: return "PMBPTR_EL1";
        case 0xc4d3: return "PMBSR_EL1";
        case 0xdf7f: return "PMCCFILTR_EL0";
        case 0xdce8: return "PMCCNTR_EL0";
        case 0xdce6: return "PMCEID0_EL0";
        case 0xdce7: return "PMCEID1_EL0";
        case 0xdce2: return "PMCNTENCLR_EL0";
        case 0xdce1: return "PMCNTENSET_EL0";
        case 0xdce0: return "PMCR_EL0";
        case 0xc4f2: return "PMINTENCLR_EL1";
        case 0xc4f1: return "PMINTENSET_EL1";
        case 0xc4f6: return "PMMIR_EL1";
        case 0xdce3: return "PMOVSCLR_EL0";
        case 0xdcf3: return "PMOVSSET_EL0";
        case 0xc4c8: return "PMSCR_EL1";
        case 0xecc8: return "PMSCR_EL12";
        case 0xe4c8: return "PMSCR_EL2";
        case 0xdce5: return "PMSELR_EL0";
        case 0xc4cd: return "PMSEVFR_EL1";
        case 0xc4cc: return "PMSFCR_EL1";
        case 0xc4ca: return "PMSICR_EL1";
        case 0xc4cf: return "PMSIDR_EL1";
        case 0xc4cb: return "PMSIRR_EL1";
        case 0xc4ce: return "PMSLATFR_EL1";
        case 0xdce4: return "PMSWINC_EL0";
        case 0xdcf0: return "PMUSERENR_EL0";
        case 0xdcea: return "PMXEVCNTR_EL0";
        case 0xdce9: return "PMXEVTYPER_EL0";
        case 0xc006: return "REVIDR_EL1";
        case 0xc085: return "RGSR_EL1";
        case 0xc602: return "RMR_EL1";
        case 0xe602: return "RMR_EL2";
        case 0xf602: return "RMR_EL3";
        case 0xd920: return "RNDR";
        case 0xd921: return "RNDRRS";
        case 0xc601: return "RVBAR_EL1";
        case 0xe601: return "RVBAR_EL2";
        case 0xf601: return "RVBAR_EL3";
        case 0xf088: return "SCR_EL3";
        case 0xc080: return "SCTLR_EL1";
        case 0xe880: return "SCTLR_EL12";
        case 0xe080: return "SCTLR_EL2";
        case 0xf080: return "SCTLR_EL3";
        case 0xde87: return "SCXTNUM_EL0";
        case 0xc687: return "SCXTNUM_EL1";
        case 0xee87: return "SCXTNUM_EL12";
        case 0xe687: return "SCXTNUM_EL2";
        case 0xf687: return "SCXTNUM_EL3";
        case 0xe099: return "SDER32_EL2";
        case 0xf089: return "SDER32_EL3";
        case 0xc200: return "SPSR_EL1";
        case 0xea00: return "SPSR_EL12";
        case 0xe200: return "SPSR_EL2";
        case 0xf200: return "SPSR_EL3";
        case 0xe219: return "SPSR_abt";
        case 0xe21b: return "SPSR_fiq";
        case 0xe218: return "SPSR_irq";
        case 0xe21a: return "SPSR_und";
        case 0xc210: return "SPSel";
        case 0xc208: return "SP_EL0";
        case 0xe208: return "SP_EL1";
        case 0xf208: return "SP_EL2";
        case 0xda16: return "SSBS";
        case 0xda17: return "TCO";
        case 0xc102: return "TCR_EL1";
        case 0xe902: return "TCR_EL12";
        case 0xe102: return "TCR_EL2";
        case 0xf102: return "TCR_EL3";
        case 0xc2b1: return "TFSRE0_EL1";
        case 0xc2b0: return "TFSR_EL1";
        case 0xeab0: return "TFSR_EL12";
        case 0xe2b0: return "TFSR_EL2";
        case 0xf2b0: return "TFSR_EL3";
        case 0xde83: return "TPIDRRO_EL0";
        case 0xde82: return "TPIDR_EL0";
        case 0xc684: return "TPIDR_EL1";
        case 0xe682: return "TPIDR_EL2";
        case 0xf682: return "TPIDR_EL3";
        case 0xc091: return "TRFCR_EL1";
        case 0xe891: return "TRFCR_EL12";
        case 0xe091: return "TRFCR_EL2";
        case 0xc100: return "TTBR0_EL1";
        case 0xe900: return "TTBR0_EL12";
        case 0xe100: return "TTBR0_EL2";
        case 0xf100: return "TTBR0_EL3";
        case 0xc101: return "TTBR1_EL1";
        case 0xe901: return "TTBR1_EL12";
        case 0xe101: return "TTBR1_EL2";
        case 0xc214: return "UAO";
        case 0xc600: return "VBAR_EL1";
        case 0xee00: return "VBAR_EL12";
        case 0xe600: return "VBAR_EL2";
        case 0xf600: return "VBAR_EL3";
        case 0xe609: return "VDISR_EL2";
        case 0xe005: return "VMPIDR_EL2";
        case 0xe110: return "VNCR_EL2";
        case 0xe000: return "VPIDR_EL2";
        case 0xe293: return "VSESR_EL2";
        case 0xe132: return "VSTCR_EL2";
        case 0xe130: return "VSTTBR_EL2";
        case 0xe10a: return "VTCR_EL2";
        case 0xe108: return "VTTBR_EL2";
        case 0xdea0: return "AMEVCNTR00_EL0";
        case 0xdea1: return "AMEVCNTR01_EL0";
        case 0xdea2: return "AMEVCNTR02_EL0";
        case 0xdea3: return "AMEVCNTR03_EL0";
        case 0xdea4: return "AMEVCNTR04_EL0";
        case 0xdea5: return "AMEVCNTR05_EL0";
        case 0xdea6: return "AMEVCNTR06_EL0";
        case 0xdea7: return "AMEVCNTR07_EL0";
        case 0xdea8: return "AMEVCNTR08_EL0";
        case 0xdea9: return "AMEVCNTR09_EL0";
        case 0xdeaa: return "AMEVCNTR010_EL0";
        case 0xdeab: return "AMEVCNTR011_EL0";
        case 0xdeac: return "AMEVCNTR012_EL0";
        case 0xdead: return "AMEVCNTR013_EL0";
        case 0xdeae: return "AMEVCNTR014_EL0";
        case 0xdeaf: return "AMEVCNTR015_EL0";
        case 0xdee0: return "AMEVCNTR10_EL0";
        case 0xdee1: return "AMEVCNTR11_EL0";
        case 0xdee2: return "AMEVCNTR12_EL0";
        case 0xdee3: return "AMEVCNTR13_EL0";
        case 0xdee4: return "AMEVCNTR14_EL0";
        case 0xdee5: return "AMEVCNTR15_EL0";
        case 0xdee6: return "AMEVCNTR16_EL0";
        case 0xdee7: return "AMEVCNTR17_EL0";
        case 0xdee8: return "AMEVCNTR18_EL0";
        case 0xdee9: return "AMEVCNTR19_EL0";
        case 0xdeea: return "AMEVCNTR110_EL0";
        case 0xdeeb: return "AMEVCNTR111_EL0";
        case 0xdeec: return "AMEVCNTR112_EL0";
        case 0xdeed: return "AMEVCNTR113_EL0";
        case 0xdeee: return "AMEVCNTR114_EL0";
        case 0xdeef: return "AMEVCNTR115_EL0";
        case 0xdeb0: return "AMEVTYPER00_EL0";
        case 0xdeb1: return "AMEVTYPER01_EL0";
        case 0xdeb2: return "AMEVTYPER02_EL0";
        case 0xdeb3: return "AMEVTYPER03_EL0";
        case 0xdeb4: return "AMEVTYPER04_EL0";
        case 0xdeb5: return "AMEVTYPER05_EL0";
        case 0xdeb6: return "AMEVTYPER06_EL0";
        case 0xdeb7: return "AMEVTYPER07_EL0";
        case 0xdeb8: return "AMEVTYPER08_EL0";
        case 0xdeb9: return "AMEVTYPER09_EL0";
        case 0xdeba: return "AMEVTYPER010_EL0";
        case 0xdebb: return "AMEVTYPER011_EL0";
        case 0xdebc: return "AMEVTYPER012_EL0";
        case 0xdebd: return "AMEVTYPER013_EL0";
        case 0xdebe: return "AMEVTYPER014_EL0";
        case 0xdebf: return "AMEVTYPER015_EL0";
        case 0xdef0: return "AMEVTYPER10_EL0";
        case 0xdef1: return "AMEVTYPER11_EL0";
        case 0xdef2: return "AMEVTYPER12_EL0";
        case 0xdef3: return "AMEVTYPER13_EL0";
        case 0xdef4: return "AMEVTYPER14_EL0";
        case 0xdef5: return "AMEVTYPER15_EL0";
        case 0xdef6: return "AMEVTYPER16_EL0";
        case 0xdef7: return "AMEVTYPER17_EL0";
        case 0xdef8: return "AMEVTYPER18_EL0";
        case 0xdef9: return "AMEVTYPER19_EL0";
        case 0xdefa: return "AMEVTYPER110_EL0";
        case 0xdefb: return "AMEVTYPER111_EL0";
        case 0xdefc: return "AMEVTYPER112_EL0";
        case 0xdefd: return "AMEVTYPER113_EL0";
        case 0xdefe: return "AMEVTYPER114_EL0";
        case 0xdeff: return "AMEVTYPER115_EL0";
        case 0x8005: return "DBGBCR0_EL1";
        case 0x800d: return "DBGBCR1_EL1";
        case 0x8015: return "DBGBCR2_EL1";
        case 0x801d: return "DBGBCR3_EL1";
        case 0x8025: return "DBGBCR4_EL1";
        case 0x802d: return "DBGBCR5_EL1";
        case 0x8035: return "DBGBCR6_EL1";
        case 0x803d: return "DBGBCR7_EL1";
        case 0x8045: return "DBGBCR8_EL1";
        case 0x804d: return "DBGBCR9_EL1";
        case 0x8055: return "DBGBCR10_EL1";
        case 0x805d: return "DBGBCR11_EL1";
        case 0x8065: return "DBGBCR12_EL1";
        case 0x806d: return "DBGBCR13_EL1";
        case 0x8075: return "DBGBCR14_EL1";
        case 0x807d: return "DBGBCR15_EL1";
        case 0x8004: return "DBGBVR0_EL1";
        case 0x800c: return "DBGBVR1_EL1";
        case 0x8014: return "DBGBVR2_EL1";
        case 0x801c: return "DBGBVR3_EL1";
        case 0x8024: return "DBGBVR4_EL1";
        case 0x802c: return "DBGBVR5_EL1";
        case 0x8034: return "DBGBVR6_EL1";
        case 0x803c: return "DBGBVR7_EL1";
        case 0x8044: return "DBGBVR8_EL1";
        case 0x804c: return "DBGBVR9_EL1";
        case 0x8054: return "DBGBVR10_EL1";
        case 0x805c: return "DBGBVR11_EL1";
        case 0x8064: return "DBGBVR12_EL1";
        case 0x806c: return "DBGBVR13_EL1";
        case 0x8074: return "DBGBVR14_EL1";
        case 0x807c: return "DBGBVR15_EL1";
        case 0x8007: return "DBGWCR0_EL1";
        case 0x800f: return "DBGWCR1_EL1";
        case 0x8017: return "DBGWCR2_EL1";
        case 0x801f: return "DBGWCR3_EL1";
        case 0x8027: return "DBGWCR4_EL1";
        case 0x802f: return "DBGWCR5_EL1";
        case 0x8037: return "DBGWCR6_EL1";
        case 0x803f: return "DBGWCR7_EL1";
        case 0x8047: return "DBGWCR8_EL1";
        case 0x804f: return "DBGWCR9_EL1";
        case 0x8057: return "DBGWCR10_EL1";
        case 0x805f: return "DBGWCR11_EL1";
        case 0x8067: return "DBGWCR12_EL1";
        case 0x806f: return "DBGWCR13_EL1";
        case 0x8077: return "DBGWCR14_EL1";
        case 0x807f: return "DBGWCR15_EL1";
        case 0x8006: return "DBGWVR0_EL1";
        case 0x800e: return "DBGWVR1_EL1";
        case 0x8016: return "DBGWVR2_EL1";
        case 0x801e: return "DBGWVR3_EL1";
        case 0x8026: return "DBGWVR4_EL1";
        case 0x802e: return "DBGWVR5_EL1";
        case 0x8036: return "DBGWVR6_EL1";
        case 0x803e: return "DBGWVR7_EL1";
        case 0x8046: return "DBGWVR8_EL1";
        case 0x804e: return "DBGWVR9_EL1";
        case 0x8056: return "DBGWVR10_EL1";
        case 0x805e: return "DBGWVR11_EL1";
        case 0x8066: return "DBGWVR12_EL1";
        case 0x806e: return "DBGWVR13_EL1";
        case 0x8076: return "DBGWVR14_EL1";
        case 0x807e: return "DBGWVR15_EL1";
        case 0xdf40: return "PMEVCNTR0_EL0";
        case 0xdf41: return "PMEVCNTR1_EL0";
        case 0xdf42: return "PMEVCNTR2_EL0";
        case 0xdf43: return "PMEVCNTR3_EL0";
        case 0xdf44: return "PMEVCNTR4_EL0";
        case 0xdf45: return "PMEVCNTR5_EL0";
        case 0xdf46: return "PMEVCNTR6_EL0";
        case 0xdf47: return "PMEVCNTR7_EL0";
        case 0xdf48: return "PMEVCNTR8_EL0";
        case 0xdf49: return "PMEVCNTR9_EL0";
        case 0xdf4a: return "PMEVCNTR10_EL0";
        case 0xdf4b: return "PMEVCNTR11_EL0";
        case 0xdf4c: return "PMEVCNTR12_EL0";
        case 0xdf4d: return "PMEVCNTR13_EL0";
        case 0xdf4e: return "PMEVCNTR14_EL0";
        case 0xdf4f: return "PMEVCNTR15_EL0";
        case 0xdf50: return "PMEVCNTR16_EL0";
        case 0xdf51: return "PMEVCNTR17_EL0";
        case 0xdf52: return "PMEVCNTR18_EL0";
        case 0xdf53: return "PMEVCNTR19_EL0";
        case 0xdf54: return "PMEVCNTR20_EL0";
        case 0xdf55: return "PMEVCNTR21_EL0";
        case 0xdf56: return "PMEVCNTR22_EL0";
        case 0xdf57: return "PMEVCNTR23_EL0";
        case 0xdf58: return "PMEVCNTR24_EL0";
        case 0xdf59: return "PMEVCNTR25_EL0";
        case 0xdf5a: return "PMEVCNTR26_EL0";
        case 0xdf5b: return "PMEVCNTR27_EL0";
        case 0xdf5c: return "PMEVCNTR28_EL0";
        case 0xdf5d: return "PMEVCNTR29_EL0";
        case 0xdf5e: return "PMEVCNTR30_EL0";
        case 0xdf5f: return "PMEVCNTR31_EL0";
        case 0xdf60: return "PMEVTYPER0_EL0";
        case 0xdf61: return "PMEVTYPER1_EL0";
        case 0xdf62: return "PMEVTYPER2_EL0";
        case 0xdf63: return "PMEVTYPER3_EL0";
        case 0xdf64: return "PMEVTYPER4_EL0";
        case 0xdf65: return "PMEVTYPER5_EL0";
        case 0xdf66: return "PMEVTYPER6_EL0";
        case 0xdf67: return "PMEVTYPER7_EL0";
        case 0xdf68: return "PMEVTYPER8_EL0";
        case 0xdf69: return "PMEVTYPER9_EL0";
        case 0xdf6a: return "PMEVTYPER10_EL0";
        case 0xdf6b: return "PMEVTYPER11_EL0";
        case 0xdf6c: return "PMEVTYPER12_EL0";
        case 0xdf6d: return "PMEVTYPER13_EL0";
        case 0xdf6e: return "PMEVTYPER14_EL0";
        case 0xdf6f: return "PMEVTYPER15_EL0";
        case 0xdf70: return "PMEVTYPER16_EL0";
        case 0xdf71: return "PMEVTYPER17_EL0";
        case 0xdf72: return "PMEVTYPER18_EL0";
        case 0xdf73: return "PMEVTYPER19_EL0";
        case 0xdf74: return "PMEVTYPER20_EL0";
        case 0xdf75: return "PMEVTYPER21_EL0";
        case 0xdf76: return "PMEVTYPER22_EL0";
        case 0xdf77: return "PMEVTYPER23_EL0";
        case 0xdf78: return "PMEVTYPER24_EL0";
        case 0xdf79: return "PMEVTYPER25_EL0";
        case 0xdf7a: return "PMEVTYPER26_EL0";
        case 0xdf7b: return "PMEVTYPER27_EL0";
        case 0xdf7c: return "PMEVTYPER28_EL0";
        case 0xdf7d: return "PMEVTYPER29_EL0";
        case 0xdf7e: return "PMEVTYPER30_EL0";
        default: return NULL;
    };
}

static S32 DisassembleSystemRegisterMoveInstr(struct instruction *i,
        struct ad_insn *out){
    U32 L = bits(i->opcode, 21, 21);
    U32 o0 = bits(i->opcode, 19, 19);
    U32 op1 = bits(i->opcode, 16, 18);
    U32 CRn = bits(i->opcode, 12, 15);
    U32 CRm = bits(i->opcode, 8, 11);
    U32 op2 = bits(i->opcode, 5, 7);
    U32 Rt = bits(i->opcode, 0, 4);

    if(Rt > AD_RTBL_GEN_64_SZ)
        return 1;

    ADD_FIELD(out, L);
    ADD_FIELD(out, o0);
    ADD_FIELD(out, op1);
    ADD_FIELD(out, CRn);
    ADD_FIELD(out, CRm);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rt);

    S32 instr_id = L == 0 ? AD_INSTR_MSR : AD_INSTR_MRS;

    U32 sreg = ((2 + o0) << 14);
    sreg |= (op1 << 11);
    sreg |= (CRn << 7);
    sreg |= (CRm << 3);
    sreg |= op2;

    const char *sreg_s = get_sysreg(sreg);
    S32 free_sreg_s = 0;

    /* if we couldn't get it, this system reg is implementation defined */
    if(!sreg_s){
        free_sreg_s = 1;
        concat((char **)&sreg_s, "S%d_%d_C%d_C%d_%d", 2 + o0, op1, CRn, CRm, op2);
    }

    const char *Rt_s = GET_GEN_REG(AD_RTBL_GEN_64, Rt, PREFER_ZR);

    if(instr_id == AD_INSTR_MRS){
        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), PREFER_ZR, _SYSREG(sreg), _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "mrs %s, %s", Rt_s, sreg_s);
    }
    else{
        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), PREFER_ZR, _SYSREG(sreg), _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rt, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));

        concat(&DECODE_STR(out), "msr %s, %s", sreg_s, Rt_s);
    }

    if(free_sreg_s){
        free((char *)sreg_s);
        sreg_s = NULL;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleUnconditionalBranchRegisterInstr(struct instruction *i,
        struct ad_insn *out){
    U32 opc = bits(i->opcode, 21, 24);
    U32 op2 = bits(i->opcode, 16, 20);
    U32 op3 = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 op4 = bits(i->opcode, 0, 4);

    if(op2 != 0x1f)
        return 1;

    ADD_FIELD(out, opc);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, op3);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, op4);

    S32 instr_id = AD_NONE;

    if((opc == 0 || opc == 1) || (opc == 8 || opc == 9)){
        const char *instr_s = NULL;

        if((opc == 0 || opc == 1) && (op3 == 0 && op4 == 0)){
            instr_s = opc == 0 ? "br" : "blr";
            instr_id = opc == 0 ? AD_INSTR_BR : AD_INSTR_BLR;
        }
        else if((opc == 0 || opc == 1) && op4 == 0x1f){
            if(opc == 0){
                instr_s = op3 == 2 ? "braaz" : "brabz";
                instr_id = op3 == 2 ? AD_INSTR_BRAAZ : AD_INSTR_BRABZ;
            }
            else{
                instr_s = op3 == 2 ? "blraaz" : "blrabz";
                instr_id = op3 == 2 ? AD_INSTR_BLRAAZ : AD_INSTR_BLRABZ;
            }
        }
        else if(opc == 8 || opc == 9){
            if(opc == 8){
                instr_s = op3 == 2 ? "braa" : "brab";
                instr_id = op3 == 2 ? AD_INSTR_BRAA : AD_INSTR_BRAB;
            }
            else{
                instr_s = op3 == 2 ? "blraa" : "blrab";
                instr_id = op3 == 2 ? AD_INSTR_BLRAA : AD_INSTR_BLRAB;
            }
        }

        if(!instr_s)
            return 1;

        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rn_s);

        if(opc == 8 || opc == 9){
            U32 Rm = op4;

            ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
            const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, NO_PREFER_ZR);

            concat(&DECODE_STR(out), ", %s", Rm_s);
        }
    }
    else if(opc == 2 || opc == 4){
        const char *instr_s = NULL;

        if(op3 == 0 && op4 == 0){
            instr_s = opc == 2 ? "ret" : "eret";
            instr_id = opc == 2 ? AD_INSTR_RET : AD_INSTR_ERET;
        }
        else if(op4 == 0x1f){
            if(opc == 2){
                instr_s = op3 == 2 ? "retaa" : "retab";
                instr_id = op3 == 2 ? AD_INSTR_RETAA : AD_INSTR_RETAB;
            }
            else{
                instr_s = op3 == 2 ? "eretaa" : "eretab";
                instr_id = op3 == 2 ? AD_INSTR_ERETAA : AD_INSTR_ERETAB;
            }
        }

        if(!instr_s)
            return 1;

        concat(&DECODE_STR(out), "%s", instr_s);

        if(instr_id == AD_INSTR_RET && Rn != 0x1e){
            ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
            const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

            concat(&DECODE_STR(out), ", %s", Rn_s);
        }
    }
    else if(opc == 5){
        instr_id = AD_INSTR_DRPS;

        concat(&DECODE_STR(out), "drps");
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleUnconditionalBranchImmInstr(struct instruction *i,
        struct ad_insn *out){
    U32 op = bits(i->opcode, 31, 31);
    U32 imm26 = bits(i->opcode, 0, 25);

    ADD_FIELD(out, op);
    ADD_FIELD(out, imm26);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;

    if(op == 0){
        instr_s = "b";
        instr_id = AD_INSTR_B;
    }
    else{
        instr_s = "bl";
        instr_id = AD_INSTR_BL;
    }

    imm26 = sign_extend(imm26 << 2, 28);

    S64 imm = (S64)imm26 + i->PC;

    ADD_IMM_OPERAND(out, AD_IMM_LONG, *(S64 *)&imm);

    concat(&DECODE_STR(out), "%s "S_LX"", instr_s, S_LA(imm));

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleCompareAndBranchImmediateInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 24, 24);
    U32 imm19 = bits(i->opcode, 5, 23);
    U32 Rt = bits(i->opcode, 0, 4);

    const char **registers = AD_RTBL_GEN_32;
    size_t len = AD_RTBL_GEN_32_SZ;

    if(sf == 1){
        registers = AD_RTBL_GEN_64;
        len = AD_RTBL_GEN_64_SZ;
    }

    if(Rt > len)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, imm19);
    ADD_FIELD(out, Rt);

    const char *Rt_s = GET_GEN_REG(registers, Rt, NO_PREFER_ZR);

    const char *instr_s = op == 0 ? "cbz" : "cbnz";
    S32 instr_id = op == 0 ? AD_INSTR_CBZ : AD_INSTR_CBNZ;
    S32 sz = (registers == AD_RTBL_GEN_64 ? _64_BIT : _32_BIT);

    imm19 = sign_extend(imm19 << 2, 21);

    S64 imm = (S64)imm19 + i->PC;

    ADD_REG_OPERAND(out, Rt, sz, NO_PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
    ADD_IMM_OPERAND(out, AD_IMM_LONG, *(S64 *)&imm);

    concat(&DECODE_STR(out), "%s %s, "S_LX"", instr_s, Rt_s, S_LA(imm));

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleTestAndBranchImmediateInstr(struct instruction *i,
        struct ad_insn *out){
    U32 b5 = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 24, 24);
    U32 b40 = bits(i->opcode, 19, 23);
    U32 imm14 = bits(i->opcode, 5, 18);
    U32 Rt = bits(i->opcode, 0, 4);

    const char **registers = AD_RTBL_GEN_32;
    size_t len = AD_RTBL_GEN_32_SZ;

    if(b5 == 1){
        registers = AD_RTBL_GEN_64;
        len = AD_RTBL_GEN_64_SZ;
    }

    if(Rt > len)
        return 1;

    ADD_FIELD(out, b5);
    ADD_FIELD(out, op);
    ADD_FIELD(out, b40);
    ADD_FIELD(out, imm14);
    ADD_FIELD(out, Rt);

    const char *Rt_s = GET_GEN_REG(registers, Rt, PREFER_ZR);

    const char *instr_s = op == 0 ? "tbz" : "tbnz";
    S32 instr_id = op == 0 ? AD_INSTR_TBZ : AD_INSTR_TBNZ;
    S32 sz = (registers == AD_RTBL_GEN_64 ? _64_BIT : _32_BIT);

    U32 bit_pos = (b5 << 6) | b40;

    imm14 = sign_extend(imm14 << 2, 16);

    S64 imm = (signed)imm14 + i->PC;

    ADD_REG_OPERAND(out, Rt, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&bit_pos);
    ADD_IMM_OPERAND(out, AD_IMM_LONG, *(long *)&imm);

    concat(&DECODE_STR(out), "%s %s, #%#x, #"S_LX"", instr_s, Rt_s, bit_pos, S_LA(imm));

    SET_INSTR_ID(out, instr_id);

    return 0;
}

S32 BranchExcSysDisassemble(struct instruction *i, struct ad_insn *out){
    U32 op0 = bits(i->opcode, 29, 31);
    U32 op1 = bits(i->opcode, 12, 25);
    U32 op2 = bits(i->opcode, 0, 4);

    S32 result = 0;

    if(op0 == 2)
        result = DisassembleConditionalImmediateBranchInstr(i, out);
    else if(op0 == 6){
        if((op1 >> 12) == 0)
            result = DisassembleExcGenInstr(i, out);
        else if(op1 == 0x1032 && op2 == 0x1f)
            result = DisassembleHintInstr(i, out);
        else if(op1 == 0x1033)
            result = DisassembleBarrierInstr(i, out);
        else if((op1 & ~0x70) == 0x1004)
            result = DisassemblePSTATEInstr(i, out);
        else if(((op1 >> 7) & ~4) == 0x21)
            result = DisassembleSystemInstr(i, out);
        else if(((op1 >> 8) & ~2) == 0x11)
            result = DisassembleSystemRegisterMoveInstr(i, out);
        else if((op1 >> 13) == 1)
            result = DisassembleUnconditionalBranchRegisterInstr(i, out);
        else
            result = 1;
    }
    else if((op0 & ~4) == 0){
        result = DisassembleUnconditionalBranchImmInstr(i, out);
    }
    else if((op0 & ~4) == 1){
        if((op1 >> 13) == 0)
            result = DisassembleCompareAndBranchImmediateInstr(i, out);
        else
            result = DisassembleTestAndBranchImmediateInstr(i, out);
    }
    else{
        result = 1;
    }

    return result;
}
