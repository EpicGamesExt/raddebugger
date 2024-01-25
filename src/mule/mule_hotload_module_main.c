__declspec(dllexport) int
get_number(void)
{
  int sum = 0;
  for(int i = 0; i < 100; i += 1)
  {
    sum += i;
    sum += i;
    sum += 1;
  }
  sum = 1000;
  return sum;
}
