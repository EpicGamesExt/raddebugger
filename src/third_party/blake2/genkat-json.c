/*
   BLAKE2 reference source code package - reference C implementations

   Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
   terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
   your option.  The terms of these licenses can be found at:

   - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
   - OpenSSL license   : https://www.openssl.org/source/license.html
   - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

   More information about the BLAKE2 hash function can be found at
   https://blake2.net.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blake2.h"

#define STR_(x) #x
#define STR(x) STR_(x)

#define LENGTH 256

#define MAKE_KAT(name, size_prefix, first)                                                         \
  do {                                                                                             \
    for (i = 0; i < LENGTH; ++i) {                                                                 \
      printf("%s\n{\n", i == 0 && first ? "" : ",");                                               \
                                                                                                   \
      printf("    \"hash\": \"" #name "\",\n");                                                    \
      printf("    \"in\": \"");                                                                    \
      for (j = 0; j < i; ++j)                                                                      \
        printf("%02x", in[j]);                                                                     \
                                                                                                   \
      printf("\",\n");                                                                             \
      printf("    \"key\": \"\",\n");                                                              \
      printf("    \"out\": \"");                                                                   \
                                                                                                   \
      name(hash, size_prefix##_OUTBYTES, in, i, NULL, 0);                                          \
                                                                                                   \
      for (j = 0; j < size_prefix##_OUTBYTES; ++j)                                                 \
        printf("%02x", hash[j]);                                                                   \
      printf("\"\n");                                                                              \
      printf("}");                                                                                 \
    }                                                                                              \
  } while (0)

#define MAKE_KEYED_KAT(name, size_prefix, first)                                                   \
  do {                                                                                             \
    for (i = 0; i < LENGTH; ++i) {                                                                 \
      printf("%s\n{\n", i == 0 && first ? "" : ",");                                               \
                                                                                                   \
      printf("    \"hash\": \"" #name "\",\n");                                                    \
      printf("    \"in\": \"");                                                                    \
      for (j = 0; j < i; ++j)                                                                      \
        printf("%02x", in[j]);                                                                     \
                                                                                                   \
      printf("\",\n");                                                                             \
      printf("    \"key\": \"");                                                                   \
      for (j = 0; j < size_prefix##_KEYBYTES; ++j)                                                 \
        printf("%02x", key[j]);                                                                    \
      printf("\",\n");                                                                             \
      printf("    \"out\": \"");                                                                   \
                                                                                                   \
      name(hash, size_prefix##_OUTBYTES, in, i, key, size_prefix##_KEYBYTES);                      \
                                                                                                   \
      for (j = 0; j < size_prefix##_OUTBYTES; ++j)                                                 \
        printf("%02x", hash[j]);                                                                   \
      printf("\"\n");                                                                              \
      printf("}");                                                                                 \
    }                                                                                              \
  } while (0)

#define MAKE_XOF_KAT(name, first)                                                                  \
  do {                                                                                             \
    for (i = 1; i <= LENGTH; ++i) {                                                                \
      printf("%s\n{\n", i == 1 && first ? "" : ",");                                               \
                                                                                                   \
      printf("    \"hash\": \"" #name "\",\n");                                                    \
      printf("    \"in\": \"");                                                                    \
      for (j = 0; j < LENGTH; ++j)                                                                 \
        printf("%02x", in[j]);                                                                     \
                                                                                                   \
      printf("\",\n");                                                                             \
      printf("    \"key\": \"\",\n");                                                              \
      printf("    \"out\": \"");                                                                   \
                                                                                                   \
      name(hash, i, in, LENGTH, NULL, 0);                                                          \
                                                                                                   \
      for (j = 0; j < i; ++j)                                                                      \
        printf("%02x", hash[j]);                                                                   \
      printf("\"\n");                                                                              \
      printf("}");                                                                                 \
    }                                                                                              \
  } while (0)

#define MAKE_XOF_KEYED_KAT(name, size_prefix, first)                                               \
  do {                                                                                             \
    for (i = 1; i <= LENGTH; ++i) {                                                                \
      printf("%s\n{\n", i == 1 && first ? "" : ",");                                               \
                                                                                                   \
      printf("    \"hash\": \"" #name "\",\n");                                                    \
      printf("    \"in\": \"");                                                                    \
      for (j = 0; j < LENGTH; ++j)                                                                 \
        printf("%02x", in[j]);                                                                     \
                                                                                                   \
      printf("\",\n");                                                                             \
      printf("    \"key\": \"");                                                                   \
      for (j = 0; j < size_prefix##_KEYBYTES; ++j)                                                 \
        printf("%02x", key[j]);                                                                    \
      printf("\",\n");                                                                             \
      printf("    \"out\": \"");                                                                   \
                                                                                                   \
      name(hash, i, in, LENGTH, key, size_prefix##_KEYBYTES);                                      \
                                                                                                   \
      for (j = 0; j < i; ++j)                                                                      \
        printf("%02x", hash[j]);                                                                   \
      printf("\"\n");                                                                              \
      printf("}");                                                                                 \
    }                                                                                              \
  } while (0)

int main() {
  uint8_t key[64] = {0};
  uint8_t in[LENGTH] = {0};
  uint8_t hash[LENGTH] = {0};
  size_t i, j;

  for (i = 0; i < sizeof(in); ++i)
    in[i] = i;

  for (i = 0; i < sizeof(key); ++i)
    key[i] = i;

  printf("[");
  MAKE_KAT(blake2s, BLAKE2S, 1);
  MAKE_KEYED_KAT(blake2s, BLAKE2S, 0);
  MAKE_KAT(blake2b, BLAKE2B, 0);
  MAKE_KEYED_KAT(blake2b, BLAKE2B, 0);
  MAKE_KAT(blake2sp, BLAKE2S, 0);
  MAKE_KEYED_KAT(blake2sp, BLAKE2S, 0);
  MAKE_KAT(blake2bp, BLAKE2B, 0);
  MAKE_KEYED_KAT(blake2bp, BLAKE2B, 0);
  MAKE_XOF_KAT(blake2xs, 0);
  MAKE_XOF_KEYED_KAT(blake2xs, BLAKE2S, 0);
  MAKE_XOF_KAT(blake2xb, 0);
  MAKE_XOF_KEYED_KAT(blake2xb, BLAKE2B, 0);
  printf("\n]\n");
  fflush(stdout);
  return 0;
}
