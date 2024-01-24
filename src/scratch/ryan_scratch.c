// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <stdio.h>
#include <stdint.h>
#include <windows.h>

int main(void) {
  printf("1\n");
  
  VOID *code = VirtualAlloc(0, 0x1000, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  *((uint32_t*)code) = 0xCCCCCCCC;
  
  ((void (__fastcall *)()) code)();
  
  printf("2\n");
  return 0;
}
