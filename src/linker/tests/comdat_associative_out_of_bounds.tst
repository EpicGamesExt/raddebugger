test:
{
  artifacts:
  {
    bad_obj:
    {
      file_name: "bad.obj"
      bytes:
      {
        data: { concat:
        {
          hex: "6486020000000000670000000500000000000000"
          hex: "2e6100000000000000000000000000000100000064000000000000000000000000000000401000c0"
          hex: "2e6161000000000000000000000000000200000065000000000000000000000000000000401000c0"
          text: "aaa"
          hex: "2e6100000000000000000000010000000301"
          hex: "010000000000000000000000000002000000"
          hex: "544553540000000000000000010000000200"
          hex: "2e6161000000000000000000020000000301"
          hex: "020000000000000000000000410105000000"
          hex: "04000000"
        } }
      }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text:
        {
          name: ".text"
          permissions: (read, execute)
          content: code
          data: { concat: { hex: "48c7c000000000" // mov rax, $imm
                            hex: "c3" // ret
          } }
          relocations: { test_ref: { type: Addr32Nb, offset: 0, symbol: test } }
        } }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          test: { kind: undefined, name: "TEST" }
        }
      } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj", expect_exit: 7 } }
  steps: {}
}
