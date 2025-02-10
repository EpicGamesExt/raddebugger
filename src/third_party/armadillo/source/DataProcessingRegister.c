#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adefs.h"
#include "bits.h"
#include "common.h"
#include "instruction.h"
#include "utils.h"
#include "strext.h"

#define SHIFTED 0
#define EXTENDED 1

#define REGISTER 0
#define IMMEDIATE 1

static S32 DisassembleDataProcessingTwoSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 Rm = bits(i->opcode, 16, 20); 
    U32 opcode = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, S);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab itab[] = {
        { "subp", AD_INSTR_SUBP }, { NULL, AD_NONE }, { "udiv", AD_INSTR_UDIV },
        { "sdiv", AD_INSTR_SDIV }, { "irg", AD_INSTR_IRG }, { "gmi", AD_INSTR_GMI },
        { NULL, AD_NONE }, { NULL, AD_NONE }, { "lslv", AD_INSTR_LSLV },
        { "lsrv", AD_INSTR_LSRV }, { "asrv", AD_INSTR_ASRV },
        { "rorv", AD_INSTR_RORV }, { "pacga", AD_INSTR_PACGA },
        { NULL, AD_NONE }, { NULL, AD_NONE }, { NULL, AD_NONE }, { "crc32b", AD_INSTR_CRC32B },
        { "crc32h", AD_INSTR_CRC32H }, { "crc32w", AD_INSTR_CRC32W },
        { "crc32x", AD_INSTR_CRC32X }, { "crc32cb", AD_INSTR_CRC32CB },
        { "crc32ch", AD_INSTR_CRC32CH }, { "crc32cw", AD_INSTR_CRC32CW },
        { "crc32cx", AD_INSTR_CRC32CX }
    };

    if(OOB(opcode, itab))
        return 1;

    const char *instr_s = itab[opcode].instr_s;

    if(!instr_s)
        return 1;

    S32 instr_id = itab[opcode].instr_id;

    if(S == 1 && sf == 1 && opcode == 0){
        instr_s = "subps";
        instr_id = AD_INSTR_SUBPS;

        /* subps --> cmpp */
        if(Rd == 0x1f){
            instr_s = "cmpp";
            instr_id = AD_INSTR_CMPP;
        }
    }

    SET_INSTR_ID(out, instr_id);

    concat(&DECODE_STR(out), "%s ", instr_s);

    if(strstr(instr_s, "crc")){
        ADD_REG_OPERAND(out, Rd, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_32));
        ADD_REG_OPERAND(out, Rn, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_32));

        const char *Rd_s = GET_GEN_REG(AD_RTBL_GEN_32, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_32, Rn, PREFER_ZR);

        concat(&DECODE_STR(out), "%s, %s", Rd_s, Rn_s);

        const char *Rm_s = NULL;

        if(sf == 1){
            ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_64));
            Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, PREFER_ZR);
        }
        else{
            ADD_REG_OPERAND(out, Rm, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_32));
            Rm_s = GET_GEN_REG(AD_RTBL_GEN_32, Rm, PREFER_ZR);
        }

        concat(&DECODE_STR(out), ", %s", Rm_s);
    }
    else if(instr_id == AD_INSTR_UDIV || instr_id == AD_INSTR_SDIV ||
            instr_id == AD_INSTR_LSLV || instr_id == AD_INSTR_LSRV ||
            instr_id == AD_INSTR_ASRV || instr_id == AD_INSTR_RORV){
        const char **registers = AD_RTBL_GEN_32;
        S32 sz = _32_BIT;

        if(sf == 1){
            registers = AD_RTBL_GEN_64;
            sz = _64_BIT;
        }

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), "%s, %s, %s", Rd_s, Rn_s, Rm_s);
    }
    else{
        if(instr_id != AD_INSTR_CMPP){
            S32 prefer_zr = instr_id != AD_INSTR_IRG;

            ADD_REG_OPERAND(out, Rd, _SZ(_64_BIT), prefer_zr, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_64));
            const char *Rd_s = GET_GEN_REG(AD_RTBL_GEN_64, Rd, prefer_zr);

            concat(&DECODE_STR(out), "%s, ", Rd_s);
        }

        S32 prefer_zr = instr_id == AD_INSTR_PACGA;

        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), prefer_zr, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, prefer_zr);

        concat(&DECODE_STR(out), "%s", Rn_s);

        if(instr_id == AD_INSTR_IRG && Rm == 0x1f)
            return 0;

        prefer_zr = (instr_id == AD_INSTR_IRG || instr_id == AD_INSTR_GMI);

        ADD_REG_OPERAND(out, Rm, _SZ(_64_BIT), prefer_zr, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_64, Rm, prefer_zr);

        concat(&DECODE_STR(out), ", %s", Rm_s);
    }

    return 0;
}

static S32 DisassembleDataProcessingOneSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 S = bits(i->opcode, 29, 29);
    U32 opcode2 = bits(i->opcode, 16, 20);
    U32 opcode = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, S);
    ADD_FIELD(out, opcode2);
    ADD_FIELD(out, opcode);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    S32 instr_id = AD_NONE;

    if(opcode2 == 0){
        if(sf == 0 && S == 0 && opcode2 == 0 && opcode == 3)
            return 1;

        struct itab tab[] = {
            { "rbit", AD_INSTR_RBIT }, { "rev16", AD_INSTR_REV16 },
            { "rev", AD_INSTR_REV }, { "rev32", AD_INSTR_REV32 },
            { "clz", AD_INSTR_CLZ }, { "cls", AD_INSTR_CLS },
        };

        if(OOB(opcode, tab))
            return 1;

        const char *instr_s = tab[opcode].instr_s;
        instr_id = tab[opcode].instr_id;

        if(sf == 1){
            if(opcode == 2){
                instr_s = "rev32";
                instr_id = AD_INSTR_REV32;
            }
            else if(opcode == 3){
                instr_s = "rev";
                instr_id = AD_INSTR_REV;
            }
        }

        const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
        S32 sz = sf == 1 ? _64_BIT : _32_BIT;

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rn_s);
    }
    else if(opcode2 == 1 && opcode < 8){
        struct itab tab[] = {
            { "pacia", AD_INSTR_PACIA }, { "pacib", AD_INSTR_PACIB },
            { "pacda", AD_INSTR_PACDA }, { "pacdb", AD_INSTR_PACDB },
            { "autia", AD_INSTR_AUTIA }, { "autib", AD_INSTR_AUTIB },
            { "autda", AD_INSTR_AUTDA }, { "autdb", AD_INSTR_AUTDB }
        };

        if(OOB(opcode, tab))
            return 1;

        const char *instr_s = tab[opcode].instr_s;
        instr_id = tab[opcode].instr_id;

        ADD_REG_OPERAND(out, Rd, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), NO_PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        const char *Rd_s = GET_GEN_REG(AD_RTBL_GEN_64, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, NO_PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rn_s);
    }
    else if(opcode2 == 1 && opcode >= 8 && Rn == 0x1f){
        struct itab tab[] = {
            { "paciza", AD_INSTR_PACIZA }, { "pacizb", AD_INSTR_PACIZB },
            { "pacdza", AD_INSTR_PACDZA }, { "pacdzb", AD_INSTR_PACDZB },
            { "autiza", AD_INSTR_AUTIZA }, { "autizb", AD_INSTR_AUTIZB },
            { "autdza", AD_INSTR_AUTDZA }, { "autdzb", AD_INSTR_AUTDZB },
            { "xpaci", AD_INSTR_XPACI }, { "xpacd", AD_INSTR_XPACD }
        };

        opcode -= 8;

        if(OOB(opcode, tab))
            return 1;

        const char *instr_s = tab[opcode].instr_s;
        instr_id = tab[opcode].instr_id;

        ADD_REG_OPERAND(out, Rd, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));

        const char *Rd_s = GET_GEN_REG(AD_RTBL_GEN_64, Rd, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);
    }
    else{
        return 1;
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static const char *decode_shift(U32 op){
    switch(op){
        case 0: return "lsl";
        case 1: return "lsr";
        case 2: return "asr";
        case 3: return "ror";
        default: return NULL;
    };
}

static S32 DisassembleLogicalShiftedRegisterInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 opc = bits(i->opcode, 29, 30);
    U32 shift = bits(i->opcode, 22, 23);
    U32 N = bits(i->opcode, 21, 21);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 imm6 = bits(i->opcode, 10, 15);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(sf == 0 && (imm6 >> 5) == 1)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, opc);
    ADD_FIELD(out, shift);
    ADD_FIELD(out, N);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, imm6);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char **registers = sf == 0 ? AD_RTBL_GEN_32 : AD_RTBL_GEN_64;
    S32 sz = sf == 0 ? _32_BIT : _64_BIT;

    struct itab tab[] = {
        { "and", AD_INSTR_AND }, { "bic", AD_INSTR_BIC }, { "orr", AD_INSTR_ORR },
        { "orn", AD_INSTR_ORN }, { "eor", AD_INSTR_EOR }, { "eon", AD_INSTR_EON },
        { "ands", AD_INSTR_ANDS }, { "bics", AD_INSTR_BICS }
    };

    U32 idx = (opc << 1) | N;

    if(OOB(idx, tab))
        return 1;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
    const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);
    const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

    if(instr_id == AD_INSTR_ORR && shift == 0 && imm6 == 0 && Rn == 0x1f){
        instr_s = "mov";
        instr_id = AD_INSTR_MOV;

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rm_s);
    }
    else if(instr_id == AD_INSTR_ORN && Rn == 0x1f){
        instr_s = "mvn";
        instr_id = AD_INSTR_MVN;

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rm_s);
    }
    else if(instr_id == AD_INSTR_ANDS && Rd == 0x1f){
        instr_s = "tst";
        instr_id = AD_INSTR_TST;

        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rn_s, Rm_s);
    }
    else{
        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);
    }

    SET_INSTR_ID(out, instr_id);

    U32 amount = imm6;

    /* no need to include <shift>, #<amount> */
    if(instr_id == AD_INSTR_MOV || amount == 0)
        return 0;

    const char *shift_type = decode_shift(shift);

    if(!shift_type)
        return 1;

    ADD_SHIFT_OPERAND(out, shift, amount);

    concat(&DECODE_STR(out), ", %s #"S_X"", shift_type, S_A(amount));

    return 0;
}

static S32 get_extended_Rm(U32 option, char **regstr, U32 Rm,
        U32 *sz, const char ***registers){
    S32 _64_bit = (option & ~4) == 3;

    if(_64_bit)
        concat(regstr, "x");
    else
        concat(regstr, "w");

    if(Rm == 0x1f)
        concat(regstr, "zr");
    else
        concat(regstr, "%d", Rm);

    *sz = _64_bit ? _64_BIT : _32_BIT;
    *registers = _64_bit ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;

    return _64_bit;
}

static char *get_extended_extend_string(U32 option, U32 sf,
        U32 Rd, U32 Rn, U32 imm3){
    char *extend_string = NULL;
    const char *extend = decode_reg_extend(option);

    S32 is_lsl = 0;

    if(Rd == 0x1f || Rn == 0x1f){
        if((sf == 0 && option == 2) || (sf == 1 && option == 3)){
            if(imm3 == 0)
                extend = "";
            else{
                extend = "lsl";
                is_lsl = 1;
            }
        }
    }

    U32 amount = imm3;

    if(*extend)
        concat(&extend_string, "%s", extend);

    if(is_lsl || (!is_lsl && amount != 0))
        concat(&extend_string, " #"S_X"", S_A(amount));

    return extend_string;
}

static S32 DisassembleAddSubtractShiftedOrExtendedInstr(struct instruction *i,
        struct ad_insn *out, S32 kind){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 shift = bits(i->opcode, 22, 23);
    U32 opt = shift;
    U32 Rm = bits(i->opcode, 16, 20);
    U32 option = bits(i->opcode, 13, 15);
    U32 imm3 = bits(i->opcode, 10, 12);
    U32 imm6 = (option << 3) | imm3;
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);

    if(kind == SHIFTED)
        ADD_FIELD(out, shift);
    else
        ADD_FIELD(out, opt);

    ADD_FIELD(out, Rm);

    if(kind == SHIFTED)
        ADD_FIELD(out, imm6);
    else{
        ADD_FIELD(out, option);
        ADD_FIELD(out, imm3);
    }

    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "add", AD_INSTR_ADD }, { "adds", AD_INSTR_ADDS },
        { "sub", AD_INSTR_SUB }, { "subs", AD_INSTR_SUBS }
    };

    U32 idx = (op << 1) | S;

    if(OOB(idx, tab))
        return 1;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    S32 prefer_zr_Rd_Rn = kind == SHIFTED;

    const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
    S32 sz = sf == 1 ? _64_BIT : _32_BIT;

    /* Both shifted and extended have aliases for ADDS and SUBS,
     * but only shifted has aliases for SUB.
     */
    if((instr_id == AD_INSTR_ADDS || instr_id == AD_INSTR_SUBS) && Rd == 0x1f){
        if(instr_id == AD_INSTR_ADDS){
            instr_s = "cmn";
            instr_id = AD_INSTR_CMN;
        }
        else if(instr_id == AD_INSTR_SUBS){
            instr_s = "cmp";
            instr_id = AD_INSTR_CMP;
        }

        ADD_REG_OPERAND(out, Rn, sz, prefer_zr_Rd_Rn, _SYSREG(AD_NONE),
                _RTBL(registers));
        const char *Rn_s = GET_GEN_REG(registers, Rn, prefer_zr_Rd_Rn);

        char *Rm_s = NULL;

        if(kind == SHIFTED || (kind == EXTENDED && sf == 0)){
            ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(registers));
            Rm_s = (char *)GET_GEN_REG(registers, Rm, PREFER_ZR);
        }
        else{
            U32 sz = 0;
            const char **registers = NULL;
            S32 _64_bit = get_extended_Rm(option, &Rm_s, Rm, &sz, &registers);

            ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        }

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rn_s, Rm_s);

        if(kind == EXTENDED && sf == 1)
            free(Rm_s);
    }
    else if((instr_id == AD_INSTR_SUB || instr_id == AD_INSTR_SUBS) &&
            Rn == 0x1f && kind == SHIFTED){
        if(instr_id == AD_INSTR_SUB){
            instr_s = "neg";
            instr_id = AD_INSTR_NEG;
        }
        else if(instr_id == AD_INSTR_SUBS){
            instr_s = "negs";
            instr_id = AD_INSTR_NEGS;
        }

        ADD_REG_OPERAND(out, Rd, sz, prefer_zr_Rd_Rn, _SYSREG(AD_NONE),
                _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, prefer_zr_Rd_Rn);
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s", instr_s, Rd_s, Rm_s);
    }
    else{
        ADD_REG_OPERAND(out, Rd, sz, prefer_zr_Rd_Rn, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, prefer_zr_Rd_Rn, _SYSREG(AD_NONE), _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, prefer_zr_Rd_Rn);
        const char *Rn_s = GET_GEN_REG(registers, Rn, prefer_zr_Rd_Rn);

        char *Rm_s = NULL;

        if(kind == SHIFTED || (kind == EXTENDED && sf == 0)){
            ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(registers));
            Rm_s = (char *)GET_GEN_REG(registers, Rm, PREFER_ZR);
        }
        else{
            U32 sz = 0;
            const char **registers = NULL;
            S32 _64_bit = get_extended_Rm(option, &Rm_s, Rm, &sz, &registers);

            ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        }

        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);

        if(kind == EXTENDED && sf == 1)
            free(Rm_s);
    }

    if(kind == SHIFTED){
        if(shift == 3)
            return 1;

        const char *shift_type = decode_shift(shift);

        U32 amount = imm6;

        if(amount != 0){
            ADD_SHIFT_OPERAND(out, shift, amount);

            concat(&DECODE_STR(out), ", %s #"S_X"", shift_type, S_A(amount));
        }
    }
    else{
        char *extend_string = get_extended_extend_string(option, sf, Rd, Rn, imm3);

        if(extend_string)
            concat(&DECODE_STR(out), ", %s", extend_string);

        free(extend_string);
    }

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleAddSubtractCarryInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
    S32 sz = sf == 1 ? _64_BIT : _32_BIT;

    struct itab tab[] = {
        { "adc", AD_INSTR_ADC }, { "adcs", AD_INSTR_ADCS },
        { "sbc", AD_INSTR_SBC }, { "sbcs", AD_INSTR_SBCS }
    };

    U32 idx = (op << 1) | sf;

    if(OOB(idx, tab))
        return 1;
    
    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    if(instr_id == AD_INSTR_SBC && Rn == 0x1f){
        instr_s = "ngc";
        instr_id = AD_INSTR_NGC;
    }
    else if(instr_id == AD_INSTR_SBCS && Rn == 0x1f){
        instr_s = "ngcs";
        instr_id = AD_INSTR_NGCS;
    }

    ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

    if(instr_id != AD_INSTR_NGC && instr_id != AD_INSTR_NGCS)
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

    ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

    const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
    const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);
    const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

    concat(&DECODE_STR(out), "%s %s, ", instr_s, Rd_s);

    if(instr_id != AD_INSTR_NGC && instr_id != AD_INSTR_NGCS)
        concat(&DECODE_STR(out), "%s, ", Rn_s);

    concat(&DECODE_STR(out), "%s", Rm_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static int DisassembleRotateRightIntoFlagsInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 imm6 = bits(i->opcode, 15, 20);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 o2 = bits(i->opcode, 4, 4);
    U32 mask = bits(i->opcode, 0, 3);
    
    if(sf != 1 && op != 0 && S != 1 && o2 != 0)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);
    ADD_FIELD(out, imm6);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, o2);
    ADD_FIELD(out, mask);

    ADD_REG_OPERAND(out, Rn, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_64));
    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm6);
    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&mask);

    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_64, Rn, PREFER_ZR);

    concat(&DECODE_STR(out), "rmif %s, #"S_X", #"S_X"", Rn_s, S_A(imm6), S_A(mask));

    SET_INSTR_ID(out, AD_INSTR_RMIF);

    return 0;
}

static S32 DisassembleEvaluateIntoFlagsInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 opcode2 = bits(i->opcode, 15, 20);
    U32 sz = bits(i->opcode, 14, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 o3 = bits(i->opcode, 4, 4);
    U32 mask = bits(i->opcode, 0, 3);

    if(sf != 0 && op != 0 && S != 1 && opcode2 != 0 && o3 != 0 && mask != 13)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);
    ADD_FIELD(out, opcode2);
    ADD_FIELD(out, sz);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, o3);
    ADD_FIELD(out, mask);

    S32 instr_id = sz == 0 ? AD_INSTR_SETF8 : AD_INSTR_SETF16;

    ADD_REG_OPERAND(out, Rn, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE), _RTBL(AD_RTBL_GEN_32));

    const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_32, Rn, PREFER_ZR);

    concat(&DECODE_STR(out), "setf%d %s", sz == 0 ? 8 : 16, Rn_s);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleConditionalCompareInstr(struct instruction *i,
        struct ad_insn *out, S32 kind){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 imm5 = Rm;
    U32 cond = bits(i->opcode, 12, 15);
    U32 o2 = bits(i->opcode, 10, 10);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 o3 = bits(i->opcode, 4, 4);
    U32 nzcv = bits(i->opcode, 0, 3);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);

    if(kind == REGISTER)
        ADD_FIELD(out, Rm);
    else
        ADD_FIELD(out, imm5);

    ADD_FIELD(out, cond);
    ADD_FIELD(out, o2);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, o3);
    ADD_FIELD(out, nzcv);

    struct itab tab[] = {
        { "ccmn", AD_INSTR_CCMN }, { "ccmp", AD_INSTR_CCMP }
    };

    U32 idx = op;

    if(OOB(idx, tab))
        return 1;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;

    const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
    S32 sz = sf == 1 ? _64_BIT : _32_BIT;

    ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
    const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rn_s);

    if(kind == REGISTER){
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), ", %s", Rm_s);
    }
    else{
        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm5);
        concat(&DECODE_STR(out), ", #"S_X"", S_A(imm5));
    }

    ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&nzcv);
    concat(&DECODE_STR(out), ", #"S_X"", S_A(nzcv));

    const char *cond_s = decode_cond(cond);

    if(!cond_s)
        return 1;

    concat(&DECODE_STR(out), ", %s", cond_s);

    SET_CC(out, cond);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleConditionalSelectInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op = bits(i->opcode, 30, 30);
    U32 S = bits(i->opcode, 29, 29);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 cond = bits(i->opcode, 12, 15);
    U32 op2 = bits(i->opcode, 10, 11);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    if(S == 1)
        return 1;

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op);
    ADD_FIELD(out, S);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, cond);
    ADD_FIELD(out, op2);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    struct itab tab[] = {
        { "csel", AD_INSTR_CSEL }, { "csinc", AD_INSTR_CSINC },
        { "csinv", AD_INSTR_CSINV }, { "csneg", AD_INSTR_CSNEG }
    };

    U32 idx = (op << 1) | (op2 & 1);

    if(OOB(idx, tab))
        return 1;

    const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
    S32 sz = sf == 1 ? _64_BIT : _32_BIT;

    const char *instr_s = tab[idx].instr_s;
    S32 instr_id = tab[idx].instr_id;
    
    if(instr_id == AD_INSTR_CSINC || instr_id == AD_INSTR_CSINV){
        if(Rm != 0x1f && (cond >> 1) != 7 && Rn != 0x1f && Rn == Rm){
            instr_s = instr_id == AD_INSTR_CSINC ? "cinc" : "cinv";
            instr_id = instr_id == AD_INSTR_CSINC ? AD_INSTR_CINC : AD_INSTR_CINV;
        }
        else if(Rm == 0x1f && (cond >> 1) != 7 && Rn == 0x1f){
            instr_s = instr_id == AD_INSTR_CSINC ? "cset" : "csetm";
            instr_id = instr_id == AD_INSTR_CSINC ? AD_INSTR_CSET : AD_INSTR_CSETM;
        }
    }
    else if(instr_id == AD_INSTR_CSNEG){
        if((cond >> 1) != 7 && Rn == Rm){
            instr_s = "cneg";
            instr_id = AD_INSTR_CNEG;
        }
    }

    ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
    const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);

    concat(&DECODE_STR(out), "%s %s", instr_s, Rd_s);

    if(instr_id != AD_INSTR_CSET && instr_id != AD_INSTR_CSETM){
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);

        concat(&DECODE_STR(out), ", %s", Rn_s);
    }

    if(instr_id != AD_INSTR_CINC && instr_id != AD_INSTR_CSET &&
            instr_id != AD_INSTR_CINV && instr_id != AD_INSTR_CSETM &&
            instr_id != AD_INSTR_CNEG){
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), ", %s", Rm_s);
    }
    else{
        /* for the aliases, LSB of cond is inverted */
        cond &= ~1;
    }

    const char *cond_s = decode_cond(cond);

    if(!cond_s)
        return 1;

    concat(&DECODE_STR(out), ", %s", cond_s);

    SET_CC(out, cond);

    SET_INSTR_ID(out, instr_id);

    return 0;
}

static S32 DisassembleDataProcessingThreeSourceInstr(struct instruction *i,
        struct ad_insn *out){
    U32 sf = bits(i->opcode, 31, 31);
    U32 op54 = bits(i->opcode, 29, 30);
    U32 op31 = bits(i->opcode, 21, 23);
    U32 Rm = bits(i->opcode, 16, 20);
    U32 o0 = bits(i->opcode, 15, 15);
    U32 Ra = bits(i->opcode, 10, 14);
    U32 Rn = bits(i->opcode, 5, 9);
    U32 Rd = bits(i->opcode, 0, 4);

    ADD_FIELD(out, sf);
    ADD_FIELD(out, op54);
    ADD_FIELD(out, op31);
    ADD_FIELD(out, Rm);
    ADD_FIELD(out, o0);
    ADD_FIELD(out, Ra);
    ADD_FIELD(out, Rn);
    ADD_FIELD(out, Rd);

    const char *instr_s = NULL;
    S32 instr_id = AD_NONE;
    
    if(op31 == 0){
        const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
        S32 sz = sf == 1 ? _64_BIT : _32_BIT;

        struct itab tab[] = {
            { "madd", AD_INSTR_MADD }, { "msub", AD_INSTR_MSUB }
        };

        instr_s = tab[o0].instr_s;
        instr_id = tab[o0].instr_id;

        if(instr_id == AD_INSTR_MADD && Ra == 0x1f){
            instr_s = "mul";
            instr_id = AD_INSTR_MUL;
        }
        else if(instr_id == AD_INSTR_MSUB && Ra == 0x1f){
            instr_s = "mneg";
            instr_id = AD_INSTR_MNEG;
        }

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);

        if(instr_id != AD_INSTR_MUL && instr_id != AD_INSTR_MNEG){
            ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
            const char *Ra_s = GET_GEN_REG(registers, Ra, PREFER_ZR);

            concat(&DECODE_STR(out), ", %s", Ra_s);
        }
    }
    else if(op31 == 2 || op31 == 6){
        const char **registers = sf == 1 ? AD_RTBL_GEN_64 : AD_RTBL_GEN_32;
        S32 sz = sf == 1 ? _64_BIT : _32_BIT;

        instr_s = op31 == 2 ? "smulh" : "umulh";
        instr_id = op31 == 2 ? AD_INSTR_SMULH : AD_INSTR_UMULH;

        ADD_REG_OPERAND(out, Rd, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rn, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));
        ADD_REG_OPERAND(out, Rm, sz, PREFER_ZR, _SYSREG(AD_NONE), _RTBL(registers));

        const char *Rd_s = GET_GEN_REG(registers, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(registers, Rn, PREFER_ZR);
        const char *Rm_s = GET_GEN_REG(registers, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);
    }
    else if(op31 == 1 || op31 == 5){
        if(op31 == 1){
            instr_s = o0 == 0 ? "smaddl" : "smsubl";
            instr_id = o0 == 0 ? AD_INSTR_SMADDL : AD_INSTR_SMSUBL;
        }
        else{
            instr_s = o0 == 0 ? "umaddl" : "umsubl";
            instr_id = o0 == 0 ? AD_INSTR_UMADDL : AD_INSTR_UMSUBL;
        }

        if(Ra == 0x1f){
            if(instr_id == AD_INSTR_SMADDL){
                instr_s = "smull";
                instr_id = AD_INSTR_SMULL;
            }
            else if(instr_id == AD_INSTR_SMSUBL){
                instr_s = "smnegl";
                instr_id = AD_INSTR_SMNEGL;
            }
            else if(instr_id == AD_INSTR_UMADDL){
                instr_s = "umull";
                instr_id = AD_INSTR_UMULL;
            }
            else if(instr_id == AD_INSTR_UMSUBL){
                instr_s = "umnegl";
                instr_id = AD_INSTR_UMNEGL;
            }
        }

        ADD_REG_OPERAND(out, Rd, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_64));
        ADD_REG_OPERAND(out, Rn, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_32));
        ADD_REG_OPERAND(out, Rm, _SZ(_32_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                _RTBL(AD_RTBL_GEN_32));

        const char *Rd_s = GET_GEN_REG(AD_RTBL_GEN_64, Rd, PREFER_ZR);
        const char *Rn_s = GET_GEN_REG(AD_RTBL_GEN_32, Rn, PREFER_ZR);
        const char *Rm_s = GET_GEN_REG(AD_RTBL_GEN_32, Rm, PREFER_ZR);

        concat(&DECODE_STR(out), "%s %s, %s, %s", instr_s, Rd_s, Rn_s, Rm_s);

        if(instr_id != AD_INSTR_SMULL && instr_id != AD_INSTR_SMNEGL &&
                instr_id != AD_INSTR_UMULL && instr_id != AD_INSTR_UMNEGL){
            ADD_REG_OPERAND(out, Ra, _SZ(_64_BIT), PREFER_ZR, _SYSREG(AD_NONE),
                    _RTBL(AD_RTBL_GEN_64));
            const char *Ra_s = GET_GEN_REG(AD_RTBL_GEN_64, Ra, PREFER_ZR);

            concat(&DECODE_STR(out), ", %s", Ra_s);
        }
    }

    if(!instr_s)
        return 1;

    SET_INSTR_ID(out, instr_id);

    return 0;
}

S32 DataProcessingRegisterDisassemble(struct instruction *i,
        struct ad_insn *out){
    S32 result = 0;
    
    U32 op0 = bits(i->opcode, 30, 30);
    U32 op1 = bits(i->opcode, 28, 28);
    U32 op2 = bits(i->opcode, 21, 24);
    U32 op3 = bits(i->opcode, 10, 15);

    if(op0 == 0 && op1 == 1 && op2 == 6)
        result = DisassembleDataProcessingTwoSourceInstr(i, out);
    else if(op0 == 1 && op1 == 1 && op2 == 6)
        result = DisassembleDataProcessingOneSourceInstr(i, out);
    else if(op1 == 0 && (op2 & ~7) == 0)
        result = DisassembleLogicalShiftedRegisterInstr(i, out);
    else if(op1 == 0 && ((op2 & ~6) == 8 || (op2 & ~6) == 9))
        result = DisassembleAddSubtractShiftedOrExtendedInstr(i, out, op2 & 1);
    else if(op1 == 1 && op2 == 0 && op3 == 0)
        result = DisassembleAddSubtractCarryInstr(i, out);
    else if(op1 == 1 && op2 == 0 && (op3 & ~0x20) == 1)
        result = DisassembleRotateRightIntoFlagsInstr(i, out);
    else if(op1 == 1 && op2 == 0 && (op3 & ~0x30) == 2)
        result = DisassembleEvaluateIntoFlagsInstr(i, out);
    else if(op1 == 1 && op2 == 2 && ((op3 & ~0x3d) == 0 || (op3 & ~0x3d) == 2))
        result = DisassembleConditionalCompareInstr(i, out, op3 & ~0x3d);
    else if(op1 == 1 && op2 == 4)
        result = DisassembleConditionalSelectInstr(i, out);
    else if(op1 == 1 && (op2 >> 3) == 1)
        result = DisassembleDataProcessingThreeSourceInstr(i, out);
    else
        result = 1;

    return result;
}
