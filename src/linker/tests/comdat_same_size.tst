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
        sections: { a: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "a" } } }
        symbols:
        {
          a_def: { kind: section_definition, section: a, selection: SameSize }
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
        sections: { b: { name: ".b", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "b" } } }
        symbols:
        {
          b_def: { kind: section_definition, section: b, selection: SameSize }
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
        sections: { c: { name: ".c", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "cc" } } }
        symbols:
        {
          c_def: { kind: section_definition, section: c, selection: SameSize }
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
    image: { file_name: "a.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj", artifact: image }
    link: { args: "/subsystem:console /entry:entry /out:b.exe a.obj b.obj c.obj entry.obj", expect_exit: 24 }
  }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".a": { data: 61 } } } } } }
}
