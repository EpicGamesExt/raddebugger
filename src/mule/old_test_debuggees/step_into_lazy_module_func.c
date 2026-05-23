// TODO: step into alg needs to detect that it is about to step into a thunk and place breakpoint, respective to OS,
// where thunk calls into resolved function

/// test: {
///   windows: {
///     skip: ""
///     compile: "/Od /Z7 /c main.c module.c"
///     link:    "/fixed /debug:full /dll module.obj"
///     link:    "/fixed /debug:full /entry:main kernel32.lib module.lib delayimp.lib main.obj /out:main.exe /delayload:module.dll /incremental:no"
///     launch:  "main.exe"
///   }
///   linux: {
///     skip: ""
///     compile: "-O0 -g module.c -fPIC -shared -o libmodule.so"
///     compile: "-O0 -g main.c -L. -lmodule -o main -Wl,-rpath,%CWD%"
///     launch:  "./main"
///   }
/// }
///

/// file: "module.c"

#if _WIN32
__declspec(dllexport)
#endif
int foo(int a)
{ /// 6: { at, run }
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

