/// test: {
///   windows: {
///     compile: "/Od /Z7 /c main.c module.c"
///     link:    "/fixed /debug:full /dll module.obj"
///     link:    "/fixed /debug:full module.lib main.obj /out:main.exe /incremental:no"
///     launch:  "main.exe"
///   }
///   linux: {
///     compile: "-O0 -g module.c -fPIC -shared -o libmodule.so"
///     compile: "-O0 -g main.c -L. -lmodule -o main -Wl,-rpath,%CWD%,-z,now -fno-plt"
///     launch:  "./main"
///   }
/// }
///

/// file: "module.c"

#if _WIN32
__declspec(dllexport)
#endif
int foo(int a)
{ /// 3: at
  return a + 1;
}

/// file: "main.c"

#if _WIN32
__declspec(dllimport)
#endif
int foo();

int main()
{           /// 1: { step_into, at, step_over }
  foo(123); /// 2: { at, step_into }
}

