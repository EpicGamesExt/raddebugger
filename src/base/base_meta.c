// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Type Info Lookups

internal Member *
member_from_name(Type *type, String8 name)
{
  Member *member = &member_nil;
  if(type->members != 0 && name.size != 0)
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
serialized_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params)
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
      U64 count;
      U8 *src;
      Type *containing_type;
      U8 *containing_ptr;
      B32 is_post_header;
    };
    Task start_task = {0, type, 1, data.str};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      switch(t->type->kind)
      {
        //- rjf: leaf -> just copy the data directly
        default:
        if(TypeKind_FirstLeaf <= t->type->kind && t->type->kind <= TypeKind_LastLeaf)
        {
          str8_serial_push_string(scratch.arena, &strings, str8(t->src, t->type->size*t->count));
        }break;
        
        //- rjf: pointers -> try to interpret/understand pointer & read/write, otherwise skip
        case TypeKind_Ptr:
        {
          // rjf: unpack info about this pointer
          TypeSerializePtrRefInfo *ptr_ref_info = 0;
          for(U64 idx = 0; idx < params->ptr_ref_infos_count; idx += 1)
          {
            if(params->ptr_ref_infos[idx].type == t->type->direct)
            {
              ptr_ref_info = &params->ptr_ref_infos[idx];
              break;
            }
          }
          
          // rjf: indexification -> subtract base, divide direct size, write index
          if(ptr_ref_info != 0 && ptr_ref_info->indexify_base != 0)
          {
            U64 ptr_value = 0;
            MemoryCopy(&ptr_value, t->src, sizeof(ptr_value));
            U64 ptr_write_value = ((U64)((U8 *)ptr_value - (U8 *)ptr_ref_info->indexify_base)/t->type->direct->size);
            str8_serial_push_struct(scratch.arena, &strings, &ptr_write_value);
          }
          
          // rjf: offsetification -> subtract base, write offsets
          else if(ptr_ref_info != 0 && ptr_ref_info->offsetify_base != 0)
          {
            U64 ptr_value = 0;
            MemoryCopy(&ptr_value, t->src, sizeof(ptr_value));
            U64 ptr_write_value = (U64)((U8 *)ptr_value - (U8 *)ptr_ref_info->offsetify_base);
            str8_serial_push_struct(scratch.arena, &strings, &ptr_write_value);
          }
          
          // rjf: size-by-member (pre-header): still potentially dependent on other members which
          // delimit our size, so push a new post-header task for pointer.
          else if(t->type->count_delimiter_name.size != 0 && !t->is_post_header)
          {
            Task *task = push_array(scratch.arena, Task, 1);
            task->type  = t->type;
            task->count = t->count;
            task->src   = t->src;
            task->containing_type = t->containing_type;
            task->containing_ptr  = t->containing_ptr;
            task->is_post_header = 1;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: size-by-member (post-header): all flat parts of containing struct have been
          // iterated, so now we can read the size, & descend to new task to read pointer
          // destination contents
          else if(t->type->count_delimiter_name.size != 0 && t->is_post_header)
          {
            // rjf: determine count of this pointer
            U64 count = 0;
            {
              Member *count_member = member_from_name(t->containing_type, t->type->count_delimiter_name);
              MemoryCopy(&count, t->containing_ptr + count_member->value, count_member->type->size);
            }
            
            // rjf: push task
            Task *task = push_array(scratch.arena, Task, 1);
            task->type                 = t->type->direct;
            task->count                = count;
            task->src                  = *(void **)t->src;
            task->containing_type      = t->containing_type;
            task->containing_ptr       = t->containing_ptr;
            SLLQueuePush(first_task, last_task, task);
          }
        }break;
        
        //- rjf: arrays -> descend to underlying type, + count
        case TypeKind_Array:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type  = t->type->direct;
          task->count = t->type->count;
          task->src   = t->src;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
        
        //- rjf: struct -> descend to members
        case TypeKind_Struct:
        {
          U64 off = 0;
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
              task->count           = 1;
              task->src             = t->src + t->type->members[member_idx].value;
              task->containing_type = t->type;
              task->containing_ptr  = t->src;
              SLLQueuePush(first_task, last_task, task);
            }
          }
        }break;
        
        //- rjf: enum -> descend to basic type interpretation
        case TypeKind_Enum:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type  = t->type->direct;
          task->count = t->count;
          task->src   = t->src;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
      }
    }
  }
  String8 result = str8_serial_end(arena, &strings);
  scratch_end(scratch);
  return result;
}

internal String8
deserialized_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params)
{
  String8 result = {0};
  result.size = type->size;
  result.str  = push_array(arena, U8, result.size);
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      Type *type;
      U64 count;
      U8 *dst;
      Type *containing_type;
      U8 *containing_ptr;
      B32 is_post_header;
    };
    U64 read_off = 0;
    Task start_task = {0, type, 1, result.str};
    Task *first_task = &start_task;
    Task *last_task = first_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      U8 *t_src = data.str + read_off;
      switch(t->type->kind)
      {
        //- rjf: leaf -> copy the data directly
        default:
        if(TypeKind_FirstLeaf <= t->type->kind && t->type->kind <= TypeKind_LastLeaf)
        {
          MemoryCopy(t->dst, t_src, t->type->size*t->count);
          read_off += t->type->size*t->count;
        }break;
        
        //- rjf: pointers -> try to interpret/understand pointer & read/write, otherwise skip
        case TypeKind_Ptr:
        {
          // rjf: unpack info about this pointer
          TypeSerializePtrRefInfo *ptr_ref_info = 0;
          for(U64 idx = 0; idx < params->ptr_ref_infos_count; idx += 1)
          {
            if(params->ptr_ref_infos[idx].type == t->type->direct)
            {
              ptr_ref_info = &params->ptr_ref_infos[idx];
              break;
            }
          }
          
          // rjf: indexification -> add base, multiply direct size
          if(ptr_ref_info != 0 && ptr_ref_info->indexify_base != 0)
          {
            U64 ptr_value = 0;
            MemoryCopy(&ptr_value, t_src, sizeof(ptr_value));
            U64 ptr_write_value = (ptr_value + (U64)ptr_ref_info->indexify_base) * t->type->direct->size;
            MemoryCopy(t->dst, &ptr_write_value, sizeof(ptr_write_value));
            read_off += sizeof(ptr_value);
          }
          
          // rjf: offsetification -> subtract base, write offsets
          else if(ptr_ref_info != 0 && ptr_ref_info->offsetify_base != 0)
          {
            U64 ptr_value = 0;
            MemoryCopy(&ptr_value, t_src, sizeof(ptr_value));
            U64 ptr_write_value = ptr_value + (U64)ptr_ref_info->offsetify_base;
            MemoryCopy(t->dst, &ptr_write_value, sizeof(ptr_write_value));
            read_off += sizeof(ptr_value);
          }
          
          // rjf: size-by-member (pre-header): still potentially dependent on other members which
          // delimit our size, so push a new post-header task for pointer.
          else if(t->type->count_delimiter_name.size != 0 && !t->is_post_header)
          {
            Task *task = push_array(scratch.arena, Task, 1);
            task->type  = t->type;
            task->count = t->count;
            task->dst   = t->dst;
            task->containing_type = t->containing_type;
            task->containing_ptr = t->containing_ptr;
            task->is_post_header = 1;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: size-by-member (post-header): all flat parts of containing struct have been
          // iterated, so now we can read the size, & descend to new task to read pointer
          // destination contents
          else if(t->type->count_delimiter_name.size != 0 && t->is_post_header)
          {
            // rjf: determine count of this pointer
            U64 count = 0;
            {
              Member *count_member = member_from_name(t->containing_type, t->type->count_delimiter_name);
              MemoryCopy(&count, t->containing_ptr + count_member->value, count_member->type->size);
            }
            
            // rjf: allocate buffer for pointer destination; write address into pointer value slot
            U64 ptr_dest_buffer_size = count*t->type->direct->size;
            U8 *ptr_dest_buffer = push_array(arena, U8, ptr_dest_buffer_size);
            MemoryCopy(t->dst, &ptr_dest_buffer, sizeof(ptr_dest_buffer));
            
            // rjf: push task
            Task *task = push_array(scratch.arena, Task, 1);
            task->type                 = t->type->direct;
            task->count                = count;
            task->dst                  = ptr_dest_buffer;
            task->containing_type      = t->containing_type;
            task->containing_ptr       = t->containing_ptr;
            SLLQueuePush(first_task, last_task, task);
          }
        }break;
        
        //- rjf: arrays -> descend to underlying type, + count
        case TypeKind_Array:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type  = t->type->direct;
          task->count = t->type->count;
          task->dst   = t->dst;
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
              task->count           = 1;
              task->dst             = t->dst + t->type->members[member_idx].value;
              task->containing_type = t->type;
              task->containing_ptr  = t->dst;
              SLLQueuePush(first_task, last_task, task);
            }
          }
        }break;
        
        //- rjf: enum -> descend to basic type interpretation
        case TypeKind_Enum:
        {
          Task *task = push_array(scratch.arena, Task, 1);
          task->type  = t->type->direct;
          task->count = t->count;
          task->dst   = t->dst;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
      }
    }
    if(params->advance_out != 0)
    {
      params->advance_out[0] = read_off;
    }
    scratch_end(scratch);
  }
  return result;
}

internal String8
deep_copy_from_typed_data(Arena *arena, Type *type, String8 data, TypeSerializeParams *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 data_srlz = serialized_from_typed_data(scratch.arena, type, data, params);
  String8 data_copy = deserialized_from_typed_data(arena, type, data_srlz, params);
  scratch_end(scratch);
  return data_copy;
}
