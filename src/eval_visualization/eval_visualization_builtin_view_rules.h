// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef EVAL_VISUALIZATION_BUILTIN_VIEW_RULES_H
#define EVAL_VISUALIZATION_BUILTIN_VIEW_RULES_H

////////////////////////////////
//~ rjf: View Rule Tree Info Extraction Helpers

internal U64 ev_base_offset_from_eval(E_Eval eval);
internal E_Value ev_value_from_params(MD_Node *params);
internal E_TypeKey ev_type_key_from_params(MD_Node *params);
internal E_Value ev_value_from_params_key(MD_Node *params, String8 key);
internal Rng1U64 ev_range_from_eval_params(E_Eval eval, MD_Node *params);
internal Arch ev_arch_from_eval_params(E_Eval eval, MD_Node *params);

#endif // EVAL_VISUALIZATION_BUILTIN_VIEW_RULES_H
