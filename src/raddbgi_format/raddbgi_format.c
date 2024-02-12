// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Functions

RADDBGI_PROC RADDBGI_U64
raddbgi_hash(RADDBGI_U8 *ptr, RADDBGI_U64 size){
  RADDBGI_U64 result = 5381;
  RADDBGI_U8 *opl = ptr + size;
  for (; ptr < opl; ptr += 1){
    result = ((result << 5) + result) + *ptr;
  }
  return(result);
}

RADDBGI_PROC RADDBGI_U32
raddbgi_size_from_basic_type_kind(RADDBGI_TypeKind kind){
  RADDBGI_U32 result = 0;
  switch (kind){
#define X(N,C)
#define XZ(N,C,Z) case C: result = Z; break;
#define Y(A,N)
    RADDBGI_TypeKindXList(X,XZ,Y)
#undef X
#undef XZ
#undef Y
  }
  return(result);
}

RADDBGI_PROC RADDBGI_U32
raddbgi_addr_size_from_arch(RADDBGI_Arch arch){
  RADDBGI_U32 result = 0;
  switch (arch){
#define X(N,C,Z) case C: result = Z; break;
    RADDBGI_ArchXList(X)
#undef X
  }
  return(result);
}

//- eval helpers

RADDBGI_PROC RADDBGI_EvalConversionKind
raddbgi_eval_conversion_rule(RADDBGI_EvalTypeGroup in, RADDBGI_EvalTypeGroup out){
  RADDBGI_EvalConversionKind result = 0;
  switch (in + (out << 8)){
#define Y(i,o) case ((RADDBGI_EvalTypeGroup_##i) + ((RADDBGI_EvalTypeGroup_##o) << 8)):
#define Xb(c)
#define Xe(c) result = RADDBGI_EvalConversionKind_##c; break;
    RADDBGI_EvalConversionKindFromTypeGroupPairMap(Y,Xb,Xe)
#undef Xe
#undef Xb
#undef Y
  }
  return(result);
}

RADDBGI_PROC RADDBGI_U8*
raddbgi_eval_conversion_message(RADDBGI_EvalConversionKind conversion_kind, RADDBGI_U64 *lenout){
  RADDBGI_U8 *result = 0;
  switch (conversion_kind){
#define X(N,msg) \
case RADDBGI_EvalConversionKind_##N: result = (RADDBGI_U8*)msg; *lenout = sizeof(msg) - 1; break;
    RADDBGI_EvalConversionKindXList(X)
#undef X
  }
  return(result);
}

RADDBGI_PROC RADDBGI_S32
raddbgi_eval_opcode_type_compatible(RADDBGI_EvalOp op, RADDBGI_EvalTypeGroup group){
  RADDBGI_S32 result = 0;
  switch (op){
    case RADDBGI_EvalOp_Neg: case RADDBGI_EvalOp_Add: case RADDBGI_EvalOp_Sub:
    case RADDBGI_EvalOp_Mul: case RADDBGI_EvalOp_Div:
    case RADDBGI_EvalOp_EqEq:case RADDBGI_EvalOp_NtEq:
    case RADDBGI_EvalOp_LsEq:case RADDBGI_EvalOp_GrEq:
    case RADDBGI_EvalOp_Less:case RADDBGI_EvalOp_Grtr:
    {
      if (group != RADDBGI_EvalTypeGroup_Other){
        result = 1;
      }
    }break;
    case RADDBGI_EvalOp_Mod:case RADDBGI_EvalOp_LShift:case RADDBGI_EvalOp_RShift:
    case RADDBGI_EvalOp_BitNot:case RADDBGI_EvalOp_BitAnd:case RADDBGI_EvalOp_BitXor:
    case RADDBGI_EvalOp_BitOr:case RADDBGI_EvalOp_LogNot:case RADDBGI_EvalOp_LogAnd:
    case RADDBGI_EvalOp_LogOr: 
    {
      if (group == RADDBGI_EvalTypeGroup_S || group == RADDBGI_EvalTypeGroup_U){
        result = 1;
      }
    }break;
  }
  return(result);
}
