test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_tables.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          vf_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" } }
          vf_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" } }
          vb_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" } }
          vb_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" } }
          rtti_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, data: { hex: "01" } }
          rtti_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, data: { hex: "02" } }
          vf_ref_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" }, relocations: { rtti: { type: Addr64, offset: 0, symbol: rtti_a_symbol } } }
          vf_ref_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" }, relocations: { rtti: { type: Addr64, offset: 0, symbol: rtti_b_symbol } } }
          vb_ref_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" }, relocations: { rtti: { type: Addr64, offset: 0, symbol: rtti_a_symbol } } }
          vb_ref_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0000000000000000" }, relocations: { rtti: { type: Addr64, offset: 0, symbol: rtti_b_symbol } } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }
            relocations:
            {
              vf_a: { type: Addr64, offset: 0, symbol: vf_a_symbol }
              vf_b: { type: Addr64, offset: 8, symbol: vf_b_symbol }
              vb_a: { type: Addr64, offset: 16, symbol: vb_a_symbol }
              vb_b: { type: Addr64, offset: 24, symbol: vb_b_symbol }
              vf_ref_a: { type: Addr64, offset: 32, symbol: vf_ref_a_symbol }
              vf_ref_b: { type: Addr64, offset: 40, symbol: vf_ref_b_symbol }
              vb_ref_a: { type: Addr64, offset: 48, symbol: vb_ref_a_symbol }
              vb_ref_b: { type: Addr64, offset: 56, symbol: vb_ref_b_symbol }
            }
          }
        }
        symbols:
        {
          vf_a_definition: { kind: section_definition, section: vf_a, selection: NoDuplicates }
          vf_b_definition: { kind: section_definition, section: vf_b, selection: NoDuplicates }
          vb_a_definition: { kind: section_definition, section: vb_a, selection: NoDuplicates }
          vb_b_definition: { kind: section_definition, section: vb_b, selection: NoDuplicates }
          vf_ref_a_definition: { kind: section_definition, section: vf_ref_a, selection: NoDuplicates }
          vf_ref_b_definition: { kind: section_definition, section: vf_ref_b, selection: NoDuplicates }
          vb_ref_a_definition: { kind: section_definition, section: vb_ref_a, selection: NoDuplicates }
          vb_ref_b_definition: { kind: section_definition, section: vb_ref_b, selection: NoDuplicates }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          vf_a_symbol: { kind: external, name: "??_7a@@6B@", section: vf_a, value: 0 }
          vf_b_symbol: { kind: external, name: "??_7b@@6B@", section: vf_b, value: 0 }
          vb_a_symbol: { kind: external, name: "??_8a@@7B@", section: vb_a, value: 0 }
          vb_b_symbol: { kind: external, name: "??_8b@@7B@", section: vb_b, value: 0 }
          vf_ref_a_symbol: { kind: external, name: "??_7ra@@6B@", section: vf_ref_a, value: 0 }
          vf_ref_b_symbol: { kind: external, name: "??_7rb@@6B@", section: vf_ref_b, value: 0 }
          vb_ref_a_symbol: { kind: external, name: "??_8ra@@7B@", section: vb_ref_a, value: 0 }
          vb_ref_b_symbol: { kind: external, name: "??_8rb@@7B@", section: vb_ref_b, value: 0 }
          rtti_a_symbol: { kind: external, name: "rtti_a", section: rtti_a, value: 0 }
          rtti_b_symbol: { kind: external, name: "rtti_b", section: rtti_b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_tables.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, other_section: ".data", other_offset: 8, relation: equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 16, type: u64, other_section: ".data", other_offset: 24, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 32, type: u64, other_section: ".data", other_offset: 40, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 48, type: u64, other_section: ".data", other_offset: 56, relation: not_equal }
  }
}
