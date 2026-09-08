test:
{
  artifacts:
  {
    a_object:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            data: { hex: "e800000000e800000000c3" }
            relocations:
            {
              call_a: { type: Rel32, offset: 1, symbol: a_symbol }
              call_b: { type: Rel32, offset: 6, symbol: b_symbol }
            }
          }
          a:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 4
            flags: (link_comdat)
            // mov rax, 1
            data: { hex: "48c7c001000000c3" }
          }
          b: { name: ".text", permissions: (read, execute), content: code, alignment: 8, flags: (link_comdat), data: { hex: "48c7c001000000c3" } }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          entry_symbol: { kind: external, name: "entry", section: entry, value: 0 }
          a_symbol: { kind: static, name: "a", section: a, value: 0 }
          b_symbol: { kind: static, name: "b", section: b, value: 0 }
        }
      } }
    }
    // swap sections for a and b
    b_object:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            data: { hex: "e800000000e800000000c3" }
            relocations:
            {
              call_a: { type: Rel32, offset: 1, symbol: a_symbol }
              call_b: { type: Rel32, offset: 6, symbol: b_symbol }
            }
          }
          a: { name: ".text", permissions: (read, execute), content: code, alignment: 8, flags: (link_comdat), data: { hex: "48c7c001000000c3" } }
          b: { name: ".text", permissions: (read, execute), content: code, alignment: 4, flags: (link_comdat), data: { hex: "48c7c001000000c3" } }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          entry_symbol: { kind: external, name: "entry", section: entry, value: 0 }
          a_symbol: { kind: static, name: "a", section: a, value: 0 }
          b_symbol: { kind: static, name: "b", section: b, value: 0 }
        }
      } }
    }
    a_image: { file_name: "a.exe", pe: {} }
    b_image: { file_name: "b.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:icf a.obj", artifact: a_image }
    link: { args: "/subsystem:console /entry:entry /out:b.exe /opt:icf b.obj", artifact: b_image }
  }
  steps:
  {
    // validate output in a.exe
    expect_pe: { artifact: a_image, expected: { pe: { sections: { ".text": { data: e80b000000e806000000c3cccccccccc48c7c001000000c3 } } } } }
    // validate output in b.exe
    expect_pe: { artifact: b_image, expected: { pe: { sections: { ".text": { data: e80b000000e806000000c3cccccccccc48c7c001000000c3 } } } } }
  }
}
