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
          a:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            data: { hex: "e800000000e800000000c3" }
            relocations:
            {
              call_b: { type: Rel32, offset: 1, symbol: b_symbol }
              call_c: { type: Rel32, offset: 6, symbol: c_symbol }
            }
          }
          b: { name: ".text", permissions: (read, execute), content: code, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call_d: { type: Rel32, offset: 1, symbol: d_symbol } } }
          c: { name: ".text", permissions: (read, execute), content: code, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call_d: { type: Rel32, offset: 1, symbol: d_symbol } } }
          d: { name: ".text", permissions: (read, execute), content: code, flags: (link_comdat), data: { hex: "4831c0c3" } }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          c_definition: { kind: section_definition, section: c, selection: NoDuplicates }
          d_definition: { kind: section_definition, section: d, selection: NoDuplicates }
          a_symbol: { kind: external, name: "a", section: a, value: 0 }
          b_symbol: { kind: external, name: "b", section: b, value: 0 }
          c_symbol: { kind: external, name: "c", section: c, value: 0 }
          d_symbol: { kind: external, name: "d", section: d, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:a /out:a.exe /opt:icf a.obj", artifact: image } }
  steps:
  {
    // validate output
    expect_pe: { artifact: image, expected: { pe: { sections: { ".text": { data: e80b000000e806000000c3cccccccccce80b000000c3cccccccccccccccccccc4831c0c3 } } } } }
  }
}
