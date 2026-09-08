test:
{
  artifacts:
  {
    one_obj:
    {
      file_name: "1.obj"
      coff: { object:
      {
        machine: x64
        sections: { test: { name: ".test$mn", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "1" } } }
        symbols:
        {
          test_def: { kind: section_definition, section: test, selection: Any }
          test: { kind: external_function, name: "TEST", section: test, value: 0 }
        }
      } }
    }
    two_obj:
    {
      file_name: "2.obj"
      coff: { object:
      {
        machine: x64
        sections: { test: { name: ".test$mn", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "2" } } }
        symbols:
        {
          test_def: { kind: section_definition, section: test, selection: Any }
          test: { kind: external, name: "TEST", section: test, value: 0 }
        }
      } }
    }
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
            relocations: { test_ref: { type: Addr32Nb, offset: 0, symbol: test } }
          }
        }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          test: { kind: undefined, name: "TEST" }
        }
      } }
    }
    one_image: { file_name: "1.exe", pe: {} }
    two_image: { file_name: "2.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:1.exe 1.obj 2.obj entry.obj", artifact: one_image }
    link: { args: "/subsystem:console /entry:entry /out:2.exe 2.obj 1.obj entry.obj", artifact: two_image }
  }
  steps:
  {
    expect_pe: { artifact: one_image, expected: { pe: { sections: { ".test": { data: 31 } } } } }
    expect_pe: { artifact: two_image, expected: { pe: { sections: { ".test": { data: 32 } } } } }
  }
}
