#include <fstream>

namespace SomeWeirdNamespace
{
  int X = 0;
  int Y = 0;
  void Foo(void)
  {
    X = 123;
    Y = 456;
    int x = 0;
  }
}

int main()
{
  std::fstream temp;
  SomeWeirdNamespace::Foo();
  return 0;
}
