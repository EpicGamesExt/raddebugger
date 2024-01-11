// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Functions

RADDBG_PROC RADDBG_U64
raddbg_hash(RADDBG_U8 *ptr, RADDBG_U64 size){
  RADDBG_U64 result = 5381;
  RADDBG_U8 *opl = ptr + size;
  for (; ptr < opl; ptr += 1){
    result = ((result << 5) + result) + *ptr;
  }
  return(result);
}

RADDBG_PROC RADDBG_U32
raddbg_size_from_basic_type_kind(RADDBG_TypeKind kind){
  RADDBG_U32 result = 0;
  switch (kind){
#define X(N,C)
#define XZ(N,C,Z) case C: result = Z; break;
#define Y(A,N)
    RADDBG_TypeKindXList(X,XZ,Y)
#undef X
#undef XZ
#undef Y
  }
  return(result);
}

RADDBG_PROC RADDBG_U32
raddbg_addr_size_from_arch(RADDBG_Arch arch){
  RADDBG_U32 result = 0;
  switch (arch){
#define X(N,C,Z) case C: result = Z; break;
    RADDBG_ArchXList(X)
#undef X
  }
  return(result);
}

//- eval helpers

RADDBG_PROC RADDBG_EvalConversionKind
raddbg_eval_conversion_rule(RADDBG_EvalTypeGroup in, RADDBG_EvalTypeGroup out){
  RADDBG_EvalConversionKind result = 0;
  switch (in + (out << 8)){
#define Y(i,o) case ((RADDBG_EvalTypeGroup_##i) + ((RADDBG_EvalTypeGroup_##o) << 8)):
#define Xb(c)
#define Xe(c) result = RADDBG_EvalConversionKind_##c; break;
    RADDBG_EvalConversionKindFromTypeGroupPairMap(Y,Xb,Xe)
#undef Xe
#undef Xb
#undef Y
  }
  return(result);
}

RADDBG_PROC RADDBG_U8*
raddbg_eval_conversion_message(RADDBG_EvalConversionKind conversion_kind, RADDBG_U64 *lenout){
  RADDBG_U8 *result = 0;
  switch (conversion_kind){
#define X(N,msg) \
case RADDBG_EvalConversionKind_##N: result = (RADDBG_U8*)msg; *lenout = sizeof(msg) - 1; break;
    RADDBG_EvalConversionKindXList(X)
#undef X
  }
  return(result);
}

RADDBG_PROC RADDBG_S32
raddbg_eval_opcode_type_compatible(RADDBG_EvalOp op, RADDBG_EvalTypeGroup group){
  RADDBG_S32 result = 0;
  switch (op){
    case RADDBG_EvalOp_Neg: case RADDBG_EvalOp_Add: case RADDBG_EvalOp_Sub:
    case RADDBG_EvalOp_Mul: case RADDBG_EvalOp_Div:
    case RADDBG_EvalOp_EqEq:case RADDBG_EvalOp_NtEq:
    case RADDBG_EvalOp_LsEq:case RADDBG_EvalOp_GrEq:
    case RADDBG_EvalOp_Less:case RADDBG_EvalOp_Grtr:
    {
      if (group != RADDBG_EvalTypeGroup_Other){
        result = 1;
      }
    }break;
    case RADDBG_EvalOp_Mod:case RADDBG_EvalOp_LShift:case RADDBG_EvalOp_RShift:
    case RADDBG_EvalOp_BitNot:case RADDBG_EvalOp_BitAnd:case RADDBG_EvalOp_BitXor:
    case RADDBG_EvalOp_BitOr:case RADDBG_EvalOp_LogNot:case RADDBG_EvalOp_LogAnd:
    case RADDBG_EvalOp_LogOr: 
    {
      if (group == RADDBG_EvalTypeGroup_S || group == RADDBG_EvalTypeGroup_U){
        result = 1;
      }
    }break;
  }
  return(result);
}
