test:
{
  artifacts:
  {
    test_obj:
    {
      file_name: "test.obj"
      coff: { object:
      {
        machine: x64
        // TEST is defined in a section explicitly removed by the object flags.
        sections: { test: { name: ".test", permissions: (read, execute), content: code, flags: (link_remove), data: { hex: "c3" } } }
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
          alignment: 1
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
  build:
  {
    // A relocation cannot resolve to a definition in a removed section.
    link: { args: "/subsystem:console /entry:entry /out:removed_section.exe test.obj entry.obj", expect_exit: nonzero }
  }
  steps: {}
}
