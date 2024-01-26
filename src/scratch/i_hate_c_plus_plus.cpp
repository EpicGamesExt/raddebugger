namespace NS
{
  static int X = 123;
  static int Y = 456;
  namespace SubNS
  {
    static int X = 111;
    static int Y = 222;
    int Foo(int x, int y)
    {
      int z = x + y + X + Y + NS::X + NS::Y;
      return z;
    }
  }
}

int main(void)
{
  int result = NS::SubNS::Foo(5, 6);
  return 0;
}
