test:
{
  artifacts:
  {
    object:
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
            flags: (link_comdat)
            // call $
            data: { hex: "e800000000c3" }
            relocations: { call_c: { type: Rel32, offset: 1, symbol: c_symbol } }
          }
          b:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            // call $
            data: { hex: "e800000000c3" }
            relocations: { call_d: { type: Rel32, offset: 1, symbol: d_symbol } }
          }
          c:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            // mov rax, 1
            // ret
            data: { hex: "48c7c001000000c3" }
          }
          d:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            // mov rax, 2
            // ret
            data: { hex: "48c7c002000000c3" }
          }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          c_definition: { kind: section_definition, section: c, selection: NoDuplicates }
          d_definition: { kind: section_definition, section: d, selection: NoDuplicates }
          entry_symbol: { kind: external, name: "entry", section: entry, value: 0 }
          a_symbol: { kind: external, name: "a", section: a, value: 0 }
          b_symbol: { kind: external, name: "b", section: b, value: 0 }
          c_symbol: { kind: external, name: "c", section: c, value: 0 }
          d_symbol: { kind: external, name: "d", section: d, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:icf a.obj", artifact: image } }
  steps:
  {
    // validate output
    expect_pe: { artifact: image, expected: { pe: { sections: { ".text":
    {
      // call a
      // call b
      // call c
      // call d
      // mov rax, 1
      // mov rax, 2
      data: e80b000000e816000000c3cccccccccce81b000000c3cccccccccccccccccccce81b000000c3cccccccccccccccccccc48c7c001000000c3cccccccccccccccc48c7c002000000c3
    } } } } }
  }
}
