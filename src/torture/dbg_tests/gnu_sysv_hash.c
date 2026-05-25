/// test: {
///   linux: {
///     skip: ""
///     compile: "-O0 -g sysv_module.c -fPIC -shared -Wl,--hash-style=sysv -o libsysv_module.so"
///     compile: "-O0 -g gnu_module.c -fPIC -shared -Wl,--hash-style=gnu -o libgnu_module.so"
///     compile: "-O0 -g main.c -L. -lsysv_module -lgnu_module -o main -Wl,-rpath,%CWD%,-z,now -fno-plt"
///     launch: "./main"
///   }
/// }

/// file: "sysv_module.c"

__attribute__((visibility("default")))          int sysv_default_object   = 10;
__attribute__((visibility("protected")))        int sysv_protected_object = 11;
__attribute__((visibility("hidden")))           int sysv_hidden_object    = 12;
__attribute__((visibility("internal")))         int sysv_internal_object  = 13;
__attribute__((weak, visibility("default")))    int sysv_weak_object      = 14;

__thread __attribute__((visibility("default")))       int sysv_tls_object           = 15;
__thread __attribute__((visibility("protected")))     int sysv_protected_tls_object = 16;
__thread __attribute__((weak, visibility("default"))) int sysv_weak_tls_object      = 17;
static __thread int sysv_static_tls_object = 18;
static          int sysv_static_object     = 19;

/*
extern long write(int, const void *, unsigned long);
extern char sysv_notype_symbol;
extern char sysv_hidden_notype_symbol;
extern char sysv_internal_notype_symbol;

__asm__(".globl sysv_notype_symbol\n"
        "sysv_notype_symbol:\n"
        "  .byte 0\n"
        ".globl sysv_hidden_notype_symbol\n"
        ".hidden sysv_hidden_notype_symbol\n"
        "sysv_hidden_notype_symbol:\n"
        "  .byte 0\n"
        ".globl sysv_internal_notype_symbol\n"
        ".internal sysv_internal_notype_symbol\n"
        "sysv_internal_notype_symbol:\n"
        "  .byte 0\n");
*/

__attribute__((visibility("default"))) int
sysv_default_func(int x)
{
  return x + 101; /// 1: { run_to_line at }
}

__attribute__((weak, visibility("default"))) int
sysv_weak_func(int x)
{
  return x + 102; /// 2: { run_to_line at }
}

__attribute__((visibility("protected"))) int
sysv_protected_func(int x)
{
  return x + 103; /// 3: { run_to_line at }
}

__attribute__((visibility("internal"))) int
sysv_internal_func(int x)
{
  return x + 104; /// 4: { run_to_line at }
}

static int
sysv_ifunc_impl(int x)
{
  return x + 105; /// 5: { run_to_line at }
}

static void *
sysv_ifunc_resolver(void)
{
  return sysv_ifunc_impl;
}

__attribute__((ifunc("sysv_ifunc_resolver"), visibility("default"))) int sysv_ifunc_func(int x);

__attribute__((visibility("hidden"))) int
sysv_hidden_func(int x)
{
  return x + sysv_hidden_object; /// 6: { run_to_line at }
}

static int
sysv_static_func(int x)
{
  return x + sysv_static_object; /// 7: { run_to_line at }
}

__attribute__((visibility("default"))) int
sysv_run_all(int x)
{
  int result = 0;
  result += sysv_default_func(x);
  result += sysv_weak_func(x);
  result += sysv_protected_func(x);
  result += sysv_internal_func(x);
  result += sysv_ifunc_func(x);
  result += sysv_hidden_func(x);
  result += sysv_static_func(x);
  result += sysv_default_object;
  result += sysv_weak_object;
  result += sysv_protected_object;
  result += sysv_hidden_object;
  result += sysv_tls_object;
  result += sysv_weak_tls_object;
  result += sysv_protected_tls_object;
  result += sysv_static_tls_object;
  result += sysv_static_object;
  result += sysv_internal_object;
  //result += (int)((unsigned long)&sysv_notype_symbol & 1);
  //result += (int)((unsigned long)&sysv_hidden_notype_symbol & 1);
  //result += (int)((unsigned long)&sysv_internal_notype_symbol & 1);
  //result += (int)write(-1, 0, 0);
  return result; /// 8: { run_to_line at }
}

/// file: "gnu_module.c"

__attribute__((visibility("default")))       int gnu_default_object   = 20;
__attribute__((weak, visibility("default"))) int gnu_weak_object      = 21;
__attribute__((visibility("protected")))     int gnu_protected_object = 22;
__attribute__((visibility("hidden")))        int gnu_hidden_object    = 23;
__attribute__((visibility("internal")))      int gnu_internal_object  = 24;

__thread __attribute__((visibility("default")))       int gnu_tls_object           = 25;
__thread __attribute__((weak, visibility("default"))) int gnu_weak_tls_object      = 26;
__thread __attribute__((visibility("protected")))     int gnu_protected_tls_object = 27;
static __thread int gnu_static_tls_object = 28;
static          int gnu_static_object     = 29;

/*
extern long write(int, const void *, unsigned long);
extern char gnu_notype_symbol;
extern char gnu_hidden_notype_symbol;
extern char gnu_internal_notype_symbol;

__asm__(".globl gnu_notype_symbol\n"
        "gnu_notype_symbol:\n"
        "  .byte 0\n"
        ".globl gnu_hidden_notype_symbol\n"
        ".hidden gnu_hidden_notype_symbol\n"
        "gnu_hidden_notype_symbol:\n"
        "  .byte 0\n"
        ".globl gnu_internal_notype_symbol\n"
        ".internal gnu_internal_notype_symbol\n"
        "gnu_internal_notype_symbol:\n"
        "  .byte 0\n");
*/

__attribute__((visibility("default"))) int
gnu_default_func(int x)
{
  return x * 101; /// 9: { run_to_line at }
}

__attribute__((weak, visibility("default"))) int
gnu_weak_func(int x)
{
  return x * 102; /// 10: { run_to_line at }
}

__attribute__((visibility("protected"))) int
gnu_protected_func(int x)
{
  return x * 103; /// 11: { run_to_line at }
}

__attribute__((visibility("internal"))) int
gnu_internal_func(int x)
{
  return x * 104; /// 12: { run_to_line at }
}

static int
gnu_ifunc_impl(int x)
{
  return x * 105; /// 13: { run_to_line at }
}

static void *
gnu_ifunc_resolver(void)
{
  return gnu_ifunc_impl;
}

__attribute__((ifunc("gnu_ifunc_resolver"), visibility("default"))) int gnu_ifunc_func(int x);

__attribute__((visibility("hidden"))) int
gnu_hidden_func(int x)
{
  return x * gnu_hidden_object; /// 14: { run_to_line at }
}

static int
gnu_static_func(int x)
{
  return x * gnu_static_object; /// 15: { run_to_line at }
}

__attribute__((visibility("default"))) int
gnu_run_all(int x)
{
  int result = 0;
  result += gnu_default_func(x);
  result += gnu_weak_func(x);
  result += gnu_protected_func(x);
  result += gnu_internal_func(x);
  result += gnu_ifunc_func(x);
  result += gnu_hidden_func(x);
  result += gnu_static_func(x);
  result += gnu_default_object;
  result += gnu_weak_object;
  result += gnu_protected_object;
  result += gnu_hidden_object;
  result += gnu_tls_object;
  result += gnu_weak_tls_object;
  result += gnu_protected_tls_object;
  result += gnu_static_tls_object;
  result += gnu_static_object;
  result += gnu_internal_object;
  /*
  result += (int)((unsigned long)&gnu_notype_symbol & 1);
  result += (int)((unsigned long)&gnu_hidden_notype_symbol & 1);
  result += (int)((unsigned long)&gnu_internal_notype_symbol & 1);
  result += (int)write(-1, 0, 0);
  */
  return result; /// 16: { run_to_line at }
}

/// file: "main.c"

int sysv_run_all(int x);
int gnu_run_all(int x);

int main()
{
  int result = 0;
  result += sysv_run_all(123); /// 17: { run_to_line at }
  result += gnu_run_all(321);  /// 18: { run_to_line at }
  return result;               /// 19: { run_to_line at run }
}
