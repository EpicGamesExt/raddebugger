/// test: {
///   windows: {
///     compile: "/Od /Z7 /c main.c /Fo:main.obj"
///     compile: "/Od /Z7 /c foo.c /Fo:foo.obj"
///     link: "/fixed /debug:full /dll foo.obj /out:foo.dll /implib:foo.lib"
///     link: "/fixed /debug:full main.obj foo.lib /out:main.exe"
///     launch: "main.exe"
///   }
///   linux: {
///     compile: "-O0 -g main.c foo.c -o main"
///     launch: "./main"
///   }
/// }
///

// TODO: linux: -no-pie crashes the tracer
//
// TODO: crashes
//     compile: "-O0 -g foo.c -fPIC -shared -o libfoo.so"
//     compile: "-O0 -g main.c -L. -lfoo -o main -Wl,-rpath,%CWD%"
// ...
// dw_compute_cfa dwarf_unwind.c:261:5
// d_establish_frame_unwind_context__dwarf dbg_engine_ctrl.c:1946:25
// d_unwind_from_thread dbg_engine_ctrl.c:2913:30
// ...

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

