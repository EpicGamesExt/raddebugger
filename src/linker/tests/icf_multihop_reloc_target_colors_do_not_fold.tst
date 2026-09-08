test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_multihop.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          top_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call: { type: Rel32, offset: 1, symbol: mid_a_symbol } } }
          top_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call: { type: Rel32, offset: 1, symbol: mid_b_symbol } } }
          mid_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call: { type: Rel32, offset: 1, symbol: leaf_a_symbol } } }
          mid_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call: { type: Rel32, offset: 1, symbol: leaf_b_symbol } } }
          leaf_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "48c7c001000000c3" } }
          leaf_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "48c7c002000000c3" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }
            relocations:
            {
              top_a: { type: Addr64, offset: 0, symbol: top_a_symbol }
              top_b: { type: Addr64, offset: 8, symbol: top_b_symbol }
              mid_a: { type: Addr64, offset: 16, symbol: mid_a_symbol }
              mid_b: { type: Addr64, offset: 24, symbol: mid_b_symbol }
              leaf_a: { type: Addr64, offset: 32, symbol: leaf_a_symbol }
              leaf_b: { type: Addr64, offset: 40, symbol: leaf_b_symbol }
            }
          }
        }
        symbols:
        {
          top_a_definition: { kind: section_definition, section: top_a, selection: NoDuplicates }
          top_b_definition: { kind: section_definition, section: top_b, selection: NoDuplicates }
          mid_a_definition: { kind: section_definition, section: mid_a, selection: NoDuplicates }
          mid_b_definition: { kind: section_definition, section: mid_b, selection: NoDuplicates }
          leaf_a_definition: { kind: section_definition, section: leaf_a, selection: NoDuplicates }
          leaf_b_definition: { kind: section_definition, section: leaf_b, selection: NoDuplicates }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          top_a_symbol: { kind: external_function, name: "top_a", section: top_a, value: 0 }
          top_b_symbol: { kind: external_function, name: "top_b", section: top_b, value: 0 }
          mid_a_symbol: { kind: external_function, name: "mid_a", section: mid_a, value: 0 }
          mid_b_symbol: { kind: external_function, name: "mid_b", section: mid_b, value: 0 }
          leaf_a_symbol: { kind: external_function, name: "leaf_a", section: leaf_a, value: 0 }
          leaf_b_symbol: { kind: external_function, name: "leaf_b", section: leaf_b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_multihop.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, other_section: ".data", other_offset: 8, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 16, type: u64, nonzero: true, other_section: ".data", other_offset: 24, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 24, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 32, type: u64, nonzero: true, other_section: ".data", other_offset: 40, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 40, type: u64, nonzero: true }
  }
}
