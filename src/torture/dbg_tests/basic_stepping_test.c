/// test: {
///   windows: {
///     compile: "/Od /Z7 /c main.c /Fo:main.obj"
///     compile: "/Od /Z7 /c foo.c /Fo:foo.obj"
///     link: "/debug:full /dll foo.obj /out:foo.dll /implib:foo.lib"
///     link: "/debug:full main.obj foo.lib /out:main.exe"
///     launch: "main.exe"
///   }
///   linux: {
///     compile: "-O0 -g main.c foo.c -o main"
///     launch: "./main"
///   }
/// }

/// file: "foo.c"

#if defined(_WIN32)
__declspec(dllexport)
#endif
int foo(int a)
{               /// 6: at
                /// 7: step_over
  return a + 1; /// 8: at
                /// 9: step_out
}

/// file: "main.c"

#include <stdio.h>

#if defined(_WIN32)
__declspec(dllimport)
#endif
int foo();

int main()                     /// 1: step_into
{                              /// 2: at
                               /// 3: step_over
	int x = foo(123);            /// 4: at
                               /// 5: step_into
                               /// 10: at: -2
                               /// 11: step_over
	printf("Hello, world!\n");   /// 12: at
                               /// 13: step_over
                               /// 14: run
}

