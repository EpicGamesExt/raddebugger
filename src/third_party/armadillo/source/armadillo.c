#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adefs.h"
#include "bits.h"
#include "common.h"
#include "instruction.h"
#include "strext.h"

#include "BranchExcSys.h"
#include "DataProcessingImmediate.h"
#include "DataProcessingFloatingPoint.h"
#include "DataProcessingRegister.h"
#include "LoadsAndStores.h"

static S32 _ArmadilloDisassemble(struct instruction *i,
        struct ad_insn **_out){
    struct ad_insn *out = *_out;

    U32 op0 = bits(i->opcode, 25, 28);

    if(op0 == 0){
        out->group = AD_G_Reserved;

        U32 op1 = bits(i->opcode, 16, 24);

        if(op1 != 0)
            return 1;

        U32 imm16 = bits(i->opcode, 0, 15);

        ADD_FIELD(out, op0);
        ADD_FIELD(out, op1);
        ADD_FIELD(out, imm16);

        ADD_IMM_OPERAND(out, AD_IMM_UINT, *(U32 *)&imm16);

        concat(&DECODE_STR(out), "udf #%#x", imm16);

        SET_INSTR_ID(out, AD_INSTR_UDF);

        return 0;
    }
    else if(op0 > 0 && op0 <= 3){
        return 1;
    }
    else if((op0 >> 1) == 4){
        out->group = AD_G_DataProcessingImmediate;
        return DataProcessingImmediateDisassemble(i, out);
    }
    else if((op0 >> 1) == 5){
        out->group = AD_G_BranchExcSys;
        return BranchExcSysDisassemble(i, out);
    }
    else if((op0 & ~10) == 4){
        out->group = AD_G_LoadsAndStores;
        return LoadsAndStoresDisassemble(i, out);
    }
    else if((op0 & ~8) == 5){
        out->group = AD_G_DataProcessingRegister;
        return DataProcessingRegisterDisassemble(i, out);
    }
    else if((op0 & ~8) == 7){
        out->group = AD_G_DataProcessingFloatingPoint;
        return DataProcessingFloatingPointDisassemble(i, out);
    }

    return 0;
}

S32 ArmadilloDisassemble(U32 opcode, U64 PC,
        struct ad_insn **out){
    if(!out || (out && *out))
        return 1;

    *out = (struct ad_insn*)malloc(sizeof(struct ad_insn));

    (*out)->decoded = NULL;

    (*out)->group = AD_NONE;
    (*out)->instr_id = AD_NONE;

    (*out)->fields = NULL;
    (*out)->num_fields = 0;

    (*out)->operands = NULL;
    (*out)->num_operands = 0;

    (*out)->cc = AD_NONE;

    struct instruction *i = instruction_new(opcode, PC);

    S32 result = _ArmadilloDisassemble(i, out);

    if(result){
        free(DECODE_STR(*out));
        DECODE_STR(*out) = NULL;
        concat(&DECODE_STR(*out), ".long %#x", i->opcode);
    }

    free(i);

    return result;
}

S32 ArmadilloDone(struct ad_insn **_insn){
    if(!_insn)
        return 1;

    struct ad_insn *insn = *_insn;

    free(insn->decoded);
    free(insn->fields);
    free(insn->operands);

    free(insn);

    *_insn = NULL;

    return 0;
}
