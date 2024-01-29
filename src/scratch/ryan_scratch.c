// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <Windows.h>

int main(void)
{
  for(int i = 0; i < 1000; i += 1)
  {
    OutputDebugStringA("Hello, this is a long string which is being output in loop #1.\n");
  }
  for(int i = 0; i < 1000; i += 1)
  {
    OutputDebugStringA("Hello, this is a long string which is being output in loop #2.\n");
  }
  for(int i = 0; i < 1000; i += 1)
  {
    OutputDebugStringA("Hello, this is a long string which is being output in loop #3.\n");
  }
  for(int i = 0; i < 1000; i += 1)
  {
    OutputDebugStringA("Hello, this is a long string which is being output in loop #4.\n");
  }
  return 0;
}
