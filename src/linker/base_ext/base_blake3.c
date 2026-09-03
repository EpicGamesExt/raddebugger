// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)


#include "third_party/blake3/blake3_portable.c"

#if defined(__aarch64__) || defined(_M_ARM64)
#include "third_party/blake3/blake3_neon.c"
#endif

#include "third_party/blake3/blake3_dispatch.c"
#include "third_party/blake3/blake3.c"

#if defined(_MSC_VER) && defined(_M_AMD64)
#pragma comment (lib, "blake3")
#endif
