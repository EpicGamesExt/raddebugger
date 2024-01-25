// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

int Foo(int x, int y)
{
  return x + y;
}

int main(void)
{
  {
    int x = 23;
    int y = 45;
    Foo(x, y);
  }
  return 0;
}
