// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <stdlib.h>

int main(int argument_count, char **arguments)
{
  int *arr = malloc(sizeof(int)*1000);
  for(int i = 0; i < 1001; i += 1)
  {
    arr[i] = i;
  }
  return 0;
}
