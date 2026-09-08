test:
{
  artifacts:
  {
    source:
    {
      file_name: "icf_cpp_multihop.c"
      text: { data: { concat:
      {
        text: "__declspec(noinline) int leaf_a(void) { return 1; }"
        hex: "0a"
        text: "__declspec(noinline) int leaf_b(void) { return 2; }"
        hex: "0a"
        text: "__declspec(noinline) int mid_a(void) { return leaf_a(); }"
        hex: "0a"
        text: "__declspec(noinline) int mid_b(void) { return leaf_b(); }"
        hex: "0a"
        text: "__declspec(noinline) int top_a(void) { return mid_a(); }"
        hex: "0a"
        text: "__declspec(noinline) int top_b(void) { return mid_b(); }"
        hex: "0a"
        text: "int (* volatile p_top_a)(void) = top_a;"
        hex: "0a"
        text: "int (* volatile p_top_b)(void) = top_b;"
        hex: "0a"
        text: "int (* volatile p_mid_a)(void) = mid_a;"
        hex: "0a"
        text: "int (* volatile p_mid_b)(void) = mid_b;"
        hex: "0a"
        text: "int (* volatile p_leaf_a)(void) = leaf_a;"
        hex: "0a"
        text: "int (* volatile p_leaf_b)(void) = leaf_b;"
        hex: "0a"
        text: "int entry(void) {"
        hex: "0a"
        text: "  if (p_top_a == p_top_b) { return 1; }"
        hex: "0a"
        text: "  if (p_mid_a == p_mid_b) { return 2; }"
        hex: "0a"
        text: "  if (p_leaf_a == p_leaf_b) { return 3; }"
        hex: "0a"
        text: "  return 0;"
        hex: "0a"
        text: "}"
        hex: "0a"
      } } }
    }
  }
  build:
  {
    compile: { tool: cl, output: none, args: "/nologo /c /O2 /Gy /Zc:preprocessor /Fo:icf_cpp_multihop.obj icf_cpp_multihop.c" }
    link: { output: none, args: "/nodefaultlib /subsystem:console /entry:entry /out:icf_cpp_multihop.exe /opt:ref,icf /include:p_top_a /include:p_top_b /include:p_mid_a /include:p_mid_b /include:p_leaf_a /include:p_leaf_b icf_cpp_multihop.obj" }
  }
  steps: { run: { path: "icf_cpp_multihop.exe" } }
}
