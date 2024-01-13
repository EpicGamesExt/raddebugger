// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

@table(name  num_children  op_string)
// num_children  - # of children packed after this node kind
// op_string     - string for quick display of the operator
EVAL_ExprKindTable:
{
  { ArrayIndex          2         "[]"           }
  { MemberAccess        2         "."            }
  { Deref               1         "*"            }
  { Address             1         "&"            }
  
  { Cast                2         "cast"         }
  { Sizeof              1         "sizeof"       }
  
  { Neg                 1         "-"            }
  { LogNot              1         "!"            }
  { BitNot              1         "~"            }
  { Mul                 2         "*"            }
  { Div                 2         "/"            }
  { Mod                 2         "%"            }
  { Add                 2         "+"            }
  { Sub                 2         "-"            }
  { LShift              2         "<<"           }
  { RShift              2         ">>"           }
  { Less                2         "<"            }
  { LsEq                2         "<="           }
  { Grtr                2         ">"            }
  { GrEq                2         ">="           }
  { EqEq                2         "=="           }
  { NtEq                2         "!="           }
  
  { BitAnd              2         "&"            }
  { BitXor              2         "^"            }
  { BitOr               2         "|"            }
  { LogAnd              2         "&&"           }
  { LogOr               2         "||"           }
  
  { Ternary             3         "? "           }
  
  { LeafBytecode 0                "bytecode"     }
  { LeafMember          0         "member"       }
  { LeafU64             0         "U64"          }
  { LeafF64             0         "F64"          }
  { LeafF32             0         "F32"          }
  
  { TypeIdent 0                   "type_ident"   }
  { Ptr                 1         "ptr"          }
  { Array               2         "array"        }
  { Func                1         "function"     }
}

@table_gen
{
  `typedef U32 EVAL_ExprKind;`;
  `enum`;
  `{`;
    @expand(EVAL_ExprKindTable a) `EVAL_ExprKind_$(a.name),`;
    `EVAL_ExprKind_COUNT`;
    `};`;
  ``;
}

@table_gen_data(type:U8, fallback:0)
eval_expr_kind_child_counts:
{
  @expand(EVAL_ExprKindTable a) `$(a.num_children),`;
}

@table_gen_data(type:String8, fallback:`{0}`)
eval_expr_kind_strings:
{
  @expand(EVAL_ExprKindTable a) `str8_lit_comp("$(a.name)"),`;
}

@table_gen_data(type:String8, fallback:`{0}`)
eval_expr_op_strings:
{
  @expand(EVAL_ExprKindTable a) `str8_lit_comp("$(a.op_string)"),`;
}
