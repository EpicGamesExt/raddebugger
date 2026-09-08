test:
{
  artifacts:
  {
    empty_obj:
    {
      file_name: "empty_section.obj"
      coff: { object:
      {
        machine: x64
        // TEST is defined in a zero-sized code section and cannot satisfy a live relocation.
        sections: { test: { name: ".test", permissions: (read, execute), content: code, data: { zero: 0 } } }
        symbols: { test: { kind: external, name: "TEST", section: test, value: 0 } }
      } }
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
          data: { hex: "48c7c000000000c3" }
          relocations: { test: { type: Addr32Nb, offset: 3, symbol: test } }
        } }
        symbols:
        {
          test: { kind: undefined, name: "TEST" }
          entry: { kind: external, name: "entry", section: text, value: 0 }
        }
      } }
    }
  }
  // A relocation cannot resolve to a definition in an empty section.
  build: { link: { args: "/subsystem:console /entry:entry /out:empty_section.exe empty_section.obj entry.obj", expect_exit: nonzero } }
  steps: {}
}
