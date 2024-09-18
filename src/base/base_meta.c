// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Type Info Lookups

internal Member *
member_from_name(Type *type, String8 name)
{
  Member *member = &member_nil;
  if(type->members != 0)
  {
    for(U64 idx = 0; idx < type->count; idx += 1)
    {
      if(str8_match(type->members[idx].name, name, 0))
      {
        member = &type->members[idx];
        break;
      }
    }
  }
  return member;
}

////////////////////////////////
//~ rjf: Type Info * Instance Operations

internal String8
serialized_from_typed_data(Arena *arena, Type *type, void *ptr, TypeSerializeParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strings = {0};
  str8_serial_begin(scratch.arena, &strings);
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      Type *type;
      void *ptr;
      U64 count;
      Type *containing_type;
      void *containing_ptr;
    };
    Task start_task = {0, type, ptr, 1};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      switch(t->type->kind)
      {
        //- rjf: leaf serialiation -> just write the data directly
        default:
        if(TypeKind_FirstLeaf <= t->type->kind && t->type->kind <= TypeKind_LastLeaf)
        {
          str8_serial_push_string(scratch.arena, &strings, str8((U8 *)t->ptr, type_leaves[t->type->kind].size*t->count));
        }break;
        
        //- rjf: pointers -> try to interpret/understand pointer & write, otherwise skip
        case TypeKind_Ptr:
        {
          // rjf: gather info about pointer references of this type
          TypeSerializePtrRefInfo *ptr_ref_info = 0;
          for(U64 idx = 0; idx < params->ptr_ref_infos_count; idx += 1)
          {
            if(params->ptr_ref_infos[idx].type == t->type->direct)
            {
              ptr_ref_info = &params->ptr_ref_infos[idx];
              break;
            }
          }
          
          // rjf: read ptr value
          void *ptr_value = 0;
          MemoryCopy(&ptr_value, t->ptr, sizeof(ptr_value));
          
          // rjf: indexification -> subtract base, divide direct size, write index
          if(ptr_ref_info != 0 && ptr_ref_info->indexify_base != 0)
          {
            U64 ptr_offsetified = (U8 *)ptr_value - (U8 *)ptr_ref_info->indexify_base;
            U64 ptr_indexified  = ptr_offsetified / t->type->direct->size;
            str8_serial_push_struct(scratch.arena, &strings, &ptr_indexified);
          }
          
          // rjf: explicit identification -> descend to ID member at destination, write that
          else if(ptr_ref_info != 0 && ptr_ref_info->id_member.size != 0)
          {
            Member *member = member_from_name(t->type->direct, ptr_ref_info->id_member);
            if(member != &member_nil)
            {
              Task *task = push_array(scratch.arena, Task, 1);
              task->type      = member->type;
              task->ptr       = ((U8 *)ptr_value) + member->value;
              task->count     = 1;
              task->containing_type = t->type->direct;
              task->containing_ptr  = ptr_value;
              SLLQueuePush(first_task, last_task, task);
            }
          }
          
          // rjf: count-delimited pointers -> read count from member in containing type,
          // descend & write destination that way
          else if(t->type->count_delimiter_name.size != 0 && t->containing_type != 0)
          {
            Member *count_member = member_from_name(t->containing_type, t->type->count_delimiter_name);
            if(count_member != &member_nil)
            {
              U64 count = 0;
              MemoryCopy(&count, (U8 *)t->containing_ptr + count_member->value, count_member->type->size);
              Task *task = push_array(scratch.arena, Task, 1);
              task->type      = t->type->direct;
              task->ptr       = ptr_value;
              task->count     = count;
              task->containing_type = t->containing_type;
              task->containing_ptr  = t->containing_ptr;
              SLLQueuePush(first_task, last_task, task);
            }
          }
          
          // rjf: any other nonzero pointer -> descend to pointer destination. trust usage code
          else if(ptr_value != 0)
          {
            Task *task = push_array(scratch.arena, Task, 1);
            task->type      = t->type->direct;
            task->ptr       = ptr_value;
            task->count     = 1;
            SLLQueuePush(first_task, last_task, task);
          }
        }break;
        
        //- rjf: arrays -> descend to underlying type, + count
        case TypeKind_Array:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type            = t->type->direct;
          task->ptr             = t->ptr;
          task->count           = t->type->count;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
        
        //- rjf: struct -> descend to members
        case TypeKind_Struct:
        {
          for(U64 idx = 0; idx < t->count; idx += 1)
          {
            for(U64 member_idx = 0; member_idx < t->type->count; member_idx += 1)
            {
              if(t->type->members[member_idx].flags & MemberFlag_DoNotSerialize)
              {
                continue;
              }
              Task *task = push_array(scratch.arena, Task, 1);
              task->type            = t->type->members[member_idx].type;
              task->ptr             = (U8 *)t->ptr + t->type->size*idx + t->type->members[member_idx].value;
              task->count           = 1;
              task->containing_type = t->type;
              task->containing_ptr  = t->ptr;
              SLLQueuePush(first_task, last_task, task);
            }
          }
        }break;
        
        //- rjf: enum -> descend to basic type interpretation
        case TypeKind_Enum:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type      = t->type->direct;
          task->ptr       = t->ptr;
          task->count     = t->count;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
      }
    }
  }
  String8 result = str8_serial_end(scratch.arena, &strings);
  scratch_end(scratch);
  return result;
}

internal void *
data_from_typed_serialized(Arena *arena, Type *type, String8 string, TypeSerializeParams *params)
{
  
}
