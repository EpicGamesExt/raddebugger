test:
{
  artifacts:
  {
    source:
    {
      file_name: "main.c"
      text: { data: { concat:
      {
        text: "#include <stdlib.h>"
        hex: "0a"
        text: " int main(void) {"
        hex: "0a"
        text: "int *foo = malloc(sizeof(*foo));"
        hex: "0a"
        text: "free(foo);"
        hex: "0a"
        text: "*foo = 1;"
        hex: "0a"
        text: "}"
        hex: "0a"
      } } }
    }
  }

  build:
  {
    // /MD
    compile: { tool: cl, output: none, args: "/MD /fsanitize=address /Z7 /c /Fo:main_md.obj main.c" }
    link: { output: none, args: "main_md.obj /debug:full" }

    // /MDd
    compile: { tool: cl, output: none, args: "/MDd /fsanitize=address /Z7 /c /Fo:main_mdd.obj main.c" }
    link: { output: none, args: "main_mdd.obj /debug:full" }

    // /MT
    compile: { tool: cl, output: none, args: "/MT /fsanitize=address /Z7 /c /Fo:main_mt.obj main.c" }
    link: { output: none, args: "main_mt.obj /debug:full" }

    // /MTd
    compile: { tool: cl, output: none, args: "/MT /fsanitize=address /Z7 /c /Fo:main_mtd.obj main.c" }
    link: { output: none, args: "main_mtd.obj /debug:full" }
  }

  steps:
  {
    run: { path: "main_md.exe", expect_exit: nonzero, stderr_matches: "=================================================================*AddressSanitizer: heap-use-after-free on address*" }
    run: { path: "main_mdd.exe", expect_exit: nonzero, stderr_matches: "=================================================================*AddressSanitizer: heap-use-after-free on address*" }
    run: { path: "main_mt.exe", expect_exit: nonzero, stderr_matches: "=================================================================*AddressSanitizer: heap-use-after-free on address*" }
    run: { path: "main_mtd.exe", expect_exit: nonzero, stderr_matches: "=================================================================*AddressSanitizer: heap-use-after-free on address*" }
  }
}
