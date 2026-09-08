test:
{
  artifacts:
  {
    source:
    {
      file_name: "icf_cpp_fold.cpp"
      text: { data: { concat:
      {
        text: "extern "
        hex: "22"
        text: "C"
        hex: "22"
        text: " __declspec(noinline) int a(void) { return 42; }"
        hex: "0a"
        text: "extern "
        hex: "22"
        text: "C"
        hex: "22"
        text: " __declspec(noinline) int b(void) { return 42; }"
        hex: "0a"
        text: "extern "
        hex: "22"
        text: "C"
        hex: "22"
        text: " int (* volatile pa)(void) = a;"
        hex: "0a"
        text: "extern "
        hex: "22"
        text: "C"
        hex: "22"
        text: " int (* volatile pb)(void) = b;"
        hex: "0a"
        text: "extern "
        hex: "22"
        text: "C"
        hex: "22"
        text: " int entry(void) { return pa == pb ? 0 : 1; }"
        hex: "0a"
      } } }
    }
  }
  build:
  {
    compile: { tool: cl, output: none, args: "/nologo /c /O2 /Gy /Zc:preprocessor /Fo:icf_cpp_fold.obj icf_cpp_fold.cpp" }
    link: { output: none, args: "/nodefaultlib /subsystem:console /entry:entry /out:icf_cpp_fold.exe /opt:ref,icf /include:pa /include:pb icf_cpp_fold.obj" }
  }
  steps: { run: { path: "icf_cpp_fold.exe" } }
}
