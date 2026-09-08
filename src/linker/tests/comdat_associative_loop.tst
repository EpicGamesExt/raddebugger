test:
{
  artifacts:
  {
    loop_obj:
    {
      file_name: "loop.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          aaaa: { name: ".aaaa", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "aaaa" } }
          aa: { name: ".aa", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "aa" } }
          a: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "a" } }
          aaa: { name: ".aaa", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "aaa" } }
        }
        symbols:
        {
          aaa_def: { kind: section_definition, section: aaa, selection: Associative, associate: aa }
          aaaa_def: { kind: section_definition, section: aaaa, selection: Associative, associate: aaa }
          a_def: { kind: section_definition, section: a, selection: Associative, associate: aa }
          aa_def: { kind: section_definition, section: aa, selection: Associative, associate: a }
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
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe loop.obj entry.obj", expect_exit: 29 } }
  steps: {}
}
