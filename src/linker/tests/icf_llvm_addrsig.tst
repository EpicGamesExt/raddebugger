test:
{
  artifacts:
  {
    source:
    {
      file_name: "main.c"
      text: { data: { concat:
      {
        text: "int foo() { return 123; }"
        hex: "0a"
        text: "int bar() { return 123; }"
        hex: "0a"
        text: "int main() {"
        hex: "0a"
        text: "int (*fn)() = &foo;"
        hex: "0a"
        text: "return fn != bar;"
        hex: "0a"
        text: "}"
        hex: "0a"
      } } }
    }
  }
  build:
  {
    compile: { tool: clang, output: "main.obj", args: "main.c -ffunction-sections -target x86_64-pc-windows-msvc" }
    link: { output: none, args: "main.obj /opt:icf /out:a.exe libcmt.lib" }
    link: { output: none, args: "main.obj /opt:icf /out:a_no_addrsig.exe libcmt.lib /llvm_addrsig:no" }
  }
  steps:
  {
    run: { path: "a.exe", expect_exit: 1 }
    run: { path: "a_no_addrsig.exe" }
  }
}
