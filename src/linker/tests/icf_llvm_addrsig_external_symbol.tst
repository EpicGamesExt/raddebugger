// .llvm_addrsig can name an undefined external whose definition is in another
// object; ICF must parse and mark the resolved symbol's object, not the referrer.
test:
{
  artifacts:
  {
    ref_source:
    {
      file_name: "ref.c"
      text: { data: { concat:
      {
        text: "extern int ext_sig();"
        hex: "0a"
        text: "int (*ext_sig_addr)() = &ext_sig;"
        hex: "0a"
        text: "int entry() { return ext_sig_addr(); }"
        hex: "0a"
      } } }
    }
    def_source:
    {
      file_name: "def.c"
      text: { data: { concat:
      {
        text: "int dummy0() { return 0; }"
        hex: "0a"
        text: "int dummy1() { return 1; }"
        hex: "0a"
        text: "int dummy2() { return 2; }"
        hex: "0a"
        text: "int dummy3() { return 3; }"
        hex: "0a"
        text: "int dummy4() { return 4; }"
        hex: "0a"
        text: "int ext_sig() { return 0; }"
        hex: "0a"
      } } }
    }
  }
  build:
  {
    compile: { tool: clang, output: "ref.obj", args: "ref.c -ffunction-sections -target x86_64-pc-windows-msvc" }
    compile: { tool: clang, output: "def.obj", args: "def.c -ffunction-sections -target x86_64-pc-windows-msvc" }
    link: { output: none, args: "ref.obj def.obj /subsystem:console /entry:entry /opt:icf /out:addrsig_ext.exe libcmt.lib" }
  }
}
