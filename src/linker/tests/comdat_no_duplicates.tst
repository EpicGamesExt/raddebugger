test:
{
  artifacts:
  {
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            data:
            {
              concat:
              {
                hex: "48c7c000000000" // mov rax, $imm
                hex: "c3" // ret
              }
            }
            relocations: { a_ref: { type: Addr32Nb, offset: 0, symbol: a } }
          }
        }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          a: { kind: undefined, name: "a" }
        }
      } }
    }
    a_obj:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections: { test: { name: ".test", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "a" } } }
        symbols:
        {
          test_def: { kind: section_definition, section: test, selection: NoDuplicates }
          a: { kind: external, name: "a", section: test, value: 0 }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections: { test: { name: ".test", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "a" } } }
        symbols:
        {
          test_def: { kind: section_definition, section: test, selection: NoDuplicates }
          a: { kind: external, name: "a", section: test, value: 0 }
        }
      } }
    }
    image: { file_name: "b.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj", expect_exit: 24 }
    link: { args: "/subsystem:console /entry:entry /out:b.exe a.obj entry.obj", artifact: image }
  }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".test": { data: 61 } } } } } }
}
