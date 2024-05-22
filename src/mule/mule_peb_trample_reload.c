__declspec(dllexport) int
loop_iteration(int it)
{
  int sum = 0;
  for(int i = 0; i < 1000; i += 1)
  {
    sum += it*i;
  }
  return sum;
}
