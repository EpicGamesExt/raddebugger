// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_EVAL_H
#define RADDBG_EVAL_H

////////////////////////////////
//~ rjf: `commands` Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(commands);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(commands);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(commands);

////////////////////////////////
//~ rjf: `themes` Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(themes);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(themes);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(themes);

////////////////////////////////
//~ rjf: `locals` Type Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(locals);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(locals);

////////////////////////////////
//~ rjf: `registers` Type Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(registers);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(registers);

////////////////////////////////
//~ rjf: Schema Type Hooks

E_TYPE_IREXT_FUNCTION_DEF(schema);
E_TYPE_ACCESS_FUNCTION_DEF(schema);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(schema);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(schema);

////////////////////////////////
//~ rjf: Config Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(cfgs);

////////////////////////////////
//~ rjf: Control Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(control);

////////////////////////////////
//~ rjf: Config Slice Type Hooks

E_TYPE_IREXT_FUNCTION_DEF(cfgs_slice);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(cfgs_query);
E_TYPE_ACCESS_FUNCTION_DEF(cfgs_slice);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(cfgs_slice);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(cfgs_slice);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(cfgs_slice);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(cfgs_slice);

////////////////////////////////
//~ rjf: `call_stack` Type Hooks

E_TYPE_IREXT_FUNCTION_DEF(call_stack);
E_TYPE_ACCESS_FUNCTION_DEF(call_stack);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(call_stack);

////////////////////////////////
//~ rjf: `environment` Type Hooks

E_TYPE_IREXT_FUNCTION_DEF(environment);
E_TYPE_ACCESS_FUNCTION_DEF(environment);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(environment);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(environment);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(environment);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(environment);

////////////////////////////////
//~ rjf: `watches` Type Hooks

E_TYPE_IREXT_FUNCTION_DEF(watches);
E_TYPE_ACCESS_FUNCTION_DEF(watches);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(watches);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(watches);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(watches);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(watches);

////////////////////////////////
//~ rjf: `unattached_processes` Type Hooks

E_TYPE_ACCESS_FUNCTION_DEF(unattached_processes);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(unattached_processes);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(unattached_processes);

////////////////////////////////
//~ rjf: Control Entity List Type Hooks (`processes`, `threads`, etc.)

E_TYPE_ACCESS_FUNCTION_DEF(ctrl_entities);
E_TYPE_EXPAND_INFO_FUNCTION_DEF(ctrl_entities);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(ctrl_entities);

////////////////////////////////
//~ rjf: Debug Info Tables Eval Hooks

E_TYPE_EXPAND_INFO_FUNCTION_DEF(debug_info_table);
E_TYPE_EXPAND_RANGE_FUNCTION_DEF(debug_info_table);
E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_DEF(debug_info_table);
E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_DEF(debug_info_table);

#endif // RADDBG_EVAL_H
