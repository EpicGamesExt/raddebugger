// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct Rng1U64Node
{
  struct Rng1U64Node *next;
  Rng1U64             v;
} Rng1U64Node;

typedef struct Rng1U64List
{
  U64          count;
  Rng1U64Node *first;
  Rng1U64Node *last;
} Rng1U64List;

////////////////////////////////

internal void          rng_1u64_list_push_node(Rng1U64List *list, Rng1U64Node *node);
internal Rng1U64Node * rng_1u64_list_push(Arena *arena, Rng1U64List *list, Rng1U64 range);

