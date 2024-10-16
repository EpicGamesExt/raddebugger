// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "../third_party_ext/blake3/blake3_portable.c"

#if defined(__aarch64__) || defined(_M_ARM64)
#include "../third_party_ext/blake3/blake3_neon.c"
#endif

#include "../third_party_ext/blake3/blake3_dispatch.c"
#include "../third_party_ext/blake3/blake3.c"

#pragma comment (lib, "blake3")
