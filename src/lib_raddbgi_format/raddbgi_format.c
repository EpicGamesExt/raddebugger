// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Hasher

RDI_PROC void
rdi_hasher_begin(RDI_Hasher *hasher, RDI_U64 seed){
  if (seed == 0){
    hasher->v = 5381;
  } else{
    hasher->v = seed;
  }
}

RDI_PROC void
rdi_hasher_update(RDI_Hasher *hasher, void *ptr, RDI_U64 size){
  RDI_U8 *cur = (RDI_U8 *)ptr;
  RDI_U8 *opl = (RDI_U8 *)ptr + size;
  for (; cur < opl; cur += 1){
    hasher->v = ((hasher->v << 5) + hasher->v) + *cur;
  }
}

RDI_PROC RDI_U64
rdi_hasher_end(RDI_Hasher *hasher){
  return hasher->v;
}

RDI_PROC RDI_U64
rdi_hash(void *ptr, RDI_U64 size){
  RDI_Hasher hasher = {0};
  rdi_hasher_begin(&hasher, 0);
  rdi_hasher_update(&hasher, ptr, size);
  RDI_U64 result = rdi_hasher_end(&hasher);
  return result;
}

////////////////////////////////
// Functions

RDI_PROC RDI_U32
rdi_size_from_basic_type_kind(RDI_TypeKind kind){
  RDI_U32 result = 0;
  switch (kind){
#define X(N,C)
#define XZ(N,C,Z) case C: result = Z; break;
#define Y(A,N)
    RDI_TypeKindXList(X,XZ,Y)
#undef X
#undef XZ
#undef Y
  }
  return(result);
}

RDI_PROC RDI_U32
rdi_addr_size_from_arch(RDI_Arch arch){
  RDI_U32 result = 0;
  switch (arch){
#define X(N,C,Z) case C: result = Z; break;
    RDI_ArchXList(X)
#undef X
  }
  return(result);
}

//- eval helpers

RDI_PROC RDI_EvalConversionKind
rdi_eval_conversion_rule(RDI_EvalTypeGroup in, RDI_EvalTypeGroup out){
  RDI_EvalConversionKind result = 0;
  switch (in + (out << 8)){
#define Y(i,o) case ((RDI_EvalTypeGroup_##i) + ((RDI_EvalTypeGroup_##o) << 8)):
#define Xb(c)
#define Xe(c) result = RDI_EvalConversionKind_##c; break;
    RDI_EvalConversionKindFromTypeGroupPairMap(Y,Xb,Xe)
#undef Xe
#undef Xb
#undef Y
  }
  return(result);
}

RDI_PROC RDI_U8*
rdi_eval_conversion_message(RDI_EvalConversionKind conversion_kind, RDI_U64 *lenout){
  RDI_U8 *result = 0;
  switch (conversion_kind){
#define X(N,msg) \
case RDI_EvalConversionKind_##N: result = (RDI_U8*)msg; *lenout = sizeof(msg) - 1; break;
    RDI_EvalConversionKindXList(X)
#undef X
  }
  return(result);
}

RDI_PROC RDI_S32
rdi_eval_opcode_type_compatible(RDI_EvalOp op, RDI_EvalTypeGroup group){
  RDI_S32 result = 0;
  switch (op){
    case RDI_EvalOp_Neg: case RDI_EvalOp_Add: case RDI_EvalOp_Sub:
    case RDI_EvalOp_Mul: case RDI_EvalOp_Div:
    case RDI_EvalOp_EqEq:case RDI_EvalOp_NtEq:
    case RDI_EvalOp_LsEq:case RDI_EvalOp_GrEq:
    case RDI_EvalOp_Less:case RDI_EvalOp_Grtr:
    {
      if (group != RDI_EvalTypeGroup_Other){
        result = 1;
      }
    }break;
    case RDI_EvalOp_Mod:case RDI_EvalOp_LShift:case RDI_EvalOp_RShift:
    case RDI_EvalOp_BitNot:case RDI_EvalOp_BitAnd:case RDI_EvalOp_BitXor:
    case RDI_EvalOp_BitOr:case RDI_EvalOp_LogNot:case RDI_EvalOp_LogAnd:
    case RDI_EvalOp_LogOr: 
    {
      if (group == RDI_EvalTypeGroup_S || group == RDI_EvalTypeGroup_U){
        result = 1;
      }
    }break;
  }
  return(result);
}
