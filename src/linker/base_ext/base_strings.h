// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

// TODO: remove
internal String8Node * str8_list_pop_front(String8List *list);

internal B32 str8_starts_with(String8 string, String8 expected_prefix);

internal U64 str8_array_bsearch(String8Array arr, String8 value);
