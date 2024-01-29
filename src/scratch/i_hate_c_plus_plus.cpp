
struct Bar
{
  int x;
  int y;
  int z;
};

struct BarContainer
{
  Bar *bar;
  int bar_count;
  int bar_cap;
};

struct Foo
{
  BarContainer bar;
  int a;
  int b;
  int c;
};

int main(void)
{
  Bar bar[100] = {0};
  Foo foo_ = {bar, 50, sizeof(bar)/sizeof(Bar)};
  Foo &foo = foo_;
  return 0;
}
