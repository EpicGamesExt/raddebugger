/// test: {
///   windows: {
///     skip: ""
///     compile: {
///       cc:   "clang"
///       args: "-O0 -g -o main.exe %FILE%"
///     }
///     launch: "main.exe"
///   }
///   linux: {
///     skip: ""
///     compile: "-O0 -g -o main %FILE%"
///     launch:  "./main"
///   }
/// }

int main()
{                                   /// 1: { step_into step_into }
  for (int i = 0; i < 1;            /// 2: step_into
       ++i, ({ goto exit; })) {     /// 4: step_into
    int x = 0;                      /// 3: step_into
  }
  exit:;
}                                   /// 5: at

