__declspec(dllexport) int
loop_iteration(int it)
{
  return 111;
#if 0
  int sum = 0;
  for(int i = 0; i < 1000; i += 1)
  {
    sum += it*i;
  }
  return sum;
#endif
}
