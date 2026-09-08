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
        sections:
        {
          a: { name: ".a", permissions: (read, write), content: initialized_data, data: { text: "a" } }
          b: { name: ".b", permissions: (read, write), content: initialized_data, data: { text: "b" } }
        }
        symbols:
        {
          test: { kind: external, name: "TEST", section: a, value: 0 }
          b_def: { kind: section_definition, section: b, selection: Associative, associate: a }
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
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj test.obj", artifact: image } }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".a": { data: 61 }, ".b": { data: 62 } } } } }
  }
}
