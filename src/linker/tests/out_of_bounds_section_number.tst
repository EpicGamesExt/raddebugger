test:
{
  artifacts:
  {
    // bad.obj
    bad_obj:
    {
      file_name: "bad.obj"
      bytes: { data: { concat:
      {
        hex: "64860100000000003f0000000100000000000000"
        hex: "2e666f6f000000000000000000000000030000003c000000000000000000000000000000400000c0"
        text: "foo"
        hex: "666f6f0000000000000000007b0000000200"
        hex: "04000000"
      } } }
    }

    // entry.obj
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
          data: { hex: "48c7c000000000c3" }
          relocations: { foo: { type: Addr32Nb, offset: 0, symbol: foo } }
        } }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          foo: { kind: undefined, name: "foo" }
        }
      } }
    }
  }

  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj", output: none, expect_exit: nonzero } }
  steps: {}
}
