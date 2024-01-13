// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

@table(name, name_lower, type, default_init)
D_StackTable:
{
  {Tex2DSampleKind        tex2d_sample_kind      R_Tex2DSampleKind   `R_Tex2DSampleKind_Nearest`                   }
  {XForm2D                xform2d                Mat3x3F32           `{1, 0, 0, 0, 1, 0, 0, 0, 1}`                 }
  {Clip                   clip                   Rng2F32             `{0}`                                         }
  {Transparency           transparency           F32                 `0`                                           }
}

@table_gen
{
  @expand(D_StackTable a) `typedef struct D_$(a.name)Node D_$(a.name)Node; struct D_$(a.name)Node {D_$(a.name)Node *next; $(a.type) v;};`;
}

@table_gen
{
  `#define D_BucketStackDecls struct{\\`;
  @expand(D_StackTable a) `D_$(a.name)Node *top_$(a.name_lower);\\`;
  `}`;
}

@table_gen
{
  @expand(D_StackTable a) `read_only global D_$(a.name)Node d_nil_$(a.name_lower) = {0, $(a.default_init)};`;
}

@table_gen
{
  `#define D_BucketStackInits(b) do{\\`;
  @expand(D_StackTable a) `(b)->top_$(a.name_lower) = &d_nil_$(a.name_lower);\\`;
  `}while(0)`;
}

@table_gen
{
  `#if 0`;
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_push_$(a.name_lower)($(a.type) v);`;
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_pop_$(a.name_lower)(void);`;
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_top_$(a.name_lower)(void);`;
  `#endif`;
}

@table_gen @c_file
{
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_push_$(a.name_lower)($(a.type) v) {D_StackPushImpl($(a.name), $(a.name_lower), $(a.type), v);}`;
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_pop_$(a.name_lower)(void) {D_StackPopImpl($(a.name), $(a.name_lower), $(a.type));}`;
  @expand(D_StackTable a) `internal $(a.type) $(=>35) d_top_$(a.name_lower)(void) {D_StackTopImpl($(a.name), $(a.name_lower), $(a.type));}`;
}

@table_gen
{
  `#if 0`;
  @expand(D_StackTable a) `#define D_$(a.name)Scope(v) $(=>35) DeferLoop(d_push_$(a.name_lower)(v), d_pop_$(a.name_lower)())`;
  `#endif`;
}
