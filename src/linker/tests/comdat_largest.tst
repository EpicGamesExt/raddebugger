test:
{
  artifacts:
  {
    a_obj:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections: { a: { name: ".a", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "a" } } }
        symbols:
        {
          a_def: { kind: section_definition, section: a, selection: Largest }
          test: { kind: external, name: "TEST", section: a, value: 0 }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections: { b: { name: ".b", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "bb" } } }
        symbols:
        {
          b_def: { kind: section_definition, section: b, selection: Largest }
          test: { kind: external, name: "TEST", section: b, value: 0 }
        }
      } }
    }
    c_obj:
    {
      file_name: "c.obj"
      coff: { object:
      {
        machine: x64
        sections: { c: { name: ".c", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "c" } } }
        symbols:
        {
          c_def: { kind: section_definition, section: c, selection: Largest }
          test: { kind: external, name: "TEST", section: c, value: 0 }
        }
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
    a_image: { file_name: "a.exe", pe: {} }
    b_image: { file_name: "b.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /out:a.exe /entry:entry entry.obj a.obj b.obj", artifact: a_image }
    link: { args: "/subsystem:console /out:b.exe /entry:entry entry.obj c.obj a.obj", artifact: b_image }
  }
  steps:
  {
    expect_pe: { artifact: a_image, expected: { pe: { sections: { @absent ".a", ".b": { data: 6262 } } } } }
    expect_pe: { artifact: b_image, expected: { pe: { sections: { ".c": { data: 63 } } } } }
  }
}
