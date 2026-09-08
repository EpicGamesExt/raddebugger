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
        sections:
        {
          a: { name: "a", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "a" } }
          aa: { name: "aa", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "aa" } }
        }
        symbols:
        {
          a_def: { kind: section_definition, section: a, selection: Largest }
          test: { kind: external, name: "TEST", section: a, value: 0 }
          aa_def: { kind: section_definition, section: aa, selection: Associative, associate: a }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          bb: { name: "bb", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "bb" } }
          b: { name: "b", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "b" } }
          bbb: { name: "bbb", permissions: (read, write), content: initialized_data, flags: (link_comdat), data: { text: "bbb" } }
        }
        symbols:
        {
          bb_def: { kind: section_definition, section: bb, selection: Largest }
          b_def: { kind: section_definition, section: b, selection: Associative, associate: bb }
          bbb_def: { kind: section_definition, section: bbb, selection: Associative, associate: bb }
          test: { kind: external, name: "TEST", section: bb, value: 0 }
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
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj", artifact: image } }
  steps:
  {
    expect_pe:
    {
      artifact: image
      expected: { pe: { sections: { @absent "a", @absent "aa", "b": { data: 62 }, "bb": { data: 6262 }, "bbb": { data: 626262 } } } }
    }
  }
}
