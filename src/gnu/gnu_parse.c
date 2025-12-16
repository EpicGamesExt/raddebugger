// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal GNU_RDebugInfoList
gnu_parse_rdebug(Arena *arena, B32 is_64bit, U64 first_rdebug_vaddr, MachineOp_MemRead *mem_read_func, void *mem_read_ud)
{
  GNU_RDebugInfoList result = {0};

  for (U64 rdebug_vaddr = first_rdebug_vaddr, rdebug_next = 0; rdebug_vaddr != 0; rdebug_vaddr = rdebug_next) {
    GNU_RDebugInfo64 rdebug;
    if (is_64bit) {
      U64 rdebug_read_size = mem_read_func(rdebug_vaddr, &rdebug, sizeof(rdebug),mem_read_ud);
      if (rdebug_read_size != sizeof(rdebug)) { goto exit; }
    } else {
      GNU_RDebugInfo32 rdebug32;
      U64 rdebug_read_size = mem_read_func(rdebug_vaddr, &rdebug32, sizeof(rdebug32), mem_read_ud);
      if (rdebug_read_size != sizeof(rdebug32)) { goto exit; }
    }
    if (rdebug.r_version < 1) { goto exit; }

    if (rdebug.r_version > 1) {
      U64 r_next_size = mem_read_func(rdebug_vaddr + sizeof(rdebug), &rdebug_next, sizeof(rdebug_next), mem_read_ud);
      if (r_next_size != sizeof(rdebug_next)) { goto exit; }
    }

    GNU_RDebugInfoNode *rdebug_n = push_array(arena, GNU_RDebugInfoNode, 1);
    rdebug_n->v = rdebug;
    SLLQueuePush(result.first, result.last, rdebug_n);
    result.count += 1;
  }

  exit:;
  return result;
}

internal GNU_LinkMapList
gnu_parse_link_map_list(Arena *arena, B32 is_64bit, U64 first_link_map_vaddr, MachineOp_MemRead *mem_read_func, void *mem_read_ud)
{
  GNU_LinkMapList result = {0};

  GNU_LinkMap64 link_map = {0};
  for (U64 link_map_vaddr = first_link_map_vaddr; link_map_vaddr != 0; link_map_vaddr = link_map.next_vaddr) {
    if (is_64bit) {
      U64 link_map_size = mem_read_func(link_map_vaddr, &link_map, sizeof(link_map), mem_read_ud);
      if (link_map_size != sizeof(link_map)) { goto exit; }
    } else {
      GNU_LinkMap32 link_map32 = {0};
      U64 link_map_size = mem_read_func(link_map_vaddr, &link_map32, sizeof(link_map32), mem_read_ud);
      if (link_map_size != sizeof(link_map32)) { goto exit; }
      gnu_linkmap64_from_linkmap32(link_map32);
    }

    GNU_LinkMapNode *n = push_array(arena, GNU_LinkMapNode, 1);
    n->v = link_map;
    SLLQueuePush(result.first, result.last, n);
    result.count += 1;
  }

  exit:;
  return result;
}


