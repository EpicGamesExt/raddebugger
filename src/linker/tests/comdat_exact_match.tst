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
          a_def: { kind: section_definition, section: a, selection: ExactMatch }
          test: { kind: external, name: "TEST", section: a, value: 0 }
        }
      } }
    }
    a2_obj:
    {
      file_name: "a2.obj"
      coff: { object:
      {
        machine: x64
        sections: { a2: { name: ".a2", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "a" } } }
        symbols:
        {
          a2_def: { kind: section_definition, section: a2, selection: ExactMatch }
          test: { kind: external, name: "TEST", section: a2, value: 0 }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections: { b: { name: ".b", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "b" } } }
        symbols:
        {
          b_def: { kind: section_definition, section: b, selection: ExactMatch }
          test: { kind: external, name: "TEST", section: b, value: 0 }
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
    image: { file_name: "b.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj", expect_exit: nonzero }
    link: { args: "/subsystem:console /entry:entry /out:b.exe entry.obj a2.obj a.obj", artifact: image }
  }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".a2": { data: 61 } } } } } }
}
