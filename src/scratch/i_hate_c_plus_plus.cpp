
static int Static = 5;
namespace NS
{
  static int staticDataInNS = 99;
  struct A
  {
    static int staticData;
    int a = 20;
    int b = 30;
    void Foo()
    {
      Static += 1;
      staticDataInNS += 1;
      staticData++;
      a++;
      b++;
    }
  };
  int A::staticData = 123;
}

struct Resource
{
  int resourceType;
};

struct Stack
{
  Resource *resource;
};

struct StackNode
{
  StackNode *next;
  Stack v;
};

struct Context
{
  StackNode *entry_stack_first;
};

int main(void)
{
  Resource r_ = {0};
  Resource *r = &r_;
  Stack s = {r};
  StackNode n_ = {0, s};
  StackNode *n = &n_;
  Context c_ = {n};
  Context *context = &c_;
  
  // evaluate `context.entry_stack_first.v.resource.resourceType == 0xd8`
  int x = 0;
  
  NS::A a = {0};
  a.Foo();
  return 0;
}
