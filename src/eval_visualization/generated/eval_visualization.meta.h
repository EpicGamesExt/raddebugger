// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef EVAL_VISUALIZATION_META_H
#define EVAL_VISUALIZATION_META_H

typedef enum EV_ViewRuleKind
{
EV_ViewRuleKind_Default,
EV_ViewRuleKind_Array,
EV_ViewRuleKind_Slice,
EV_ViewRuleKind_ByteSwap,
EV_ViewRuleKind_Cast,
EV_ViewRuleKind_Only,
EV_ViewRuleKind_Omit,
EV_ViewRuleKind_Bin,
EV_ViewRuleKind_Oct,
EV_ViewRuleKind_Dec,
EV_ViewRuleKind_Hex,
EV_ViewRuleKind_COUNT,
} EV_ViewRuleKind;

EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(array);
EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(slice);
EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(bswap);
EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(cast);
EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(only);
EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_DEF(omit);
EV_VIEW_RULE_BLOCK_PROD_FUNCTION_DEF(default);
#endif // EVAL_VISUALIZATION_META_H
