test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_rdata_policy.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          data_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "01020304" } }
          data_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "01020304" } }
          data_c: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "01020304" } }
          data_d: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "01020304" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "0000000000000000000000000000000000000000000000000000000000000000" }
            relocations:
            {
              a: { type: Addr64, offset: 0, symbol: data_a_symbol }
              b: { type: Addr64, offset: 8, symbol: data_b_symbol }
              c: { type: Addr64, offset: 16, symbol: data_c_symbol }
              d: { type: Addr64, offset: 24, symbol: data_d_symbol }
            }
          }
        }
        symbols:
        {
          data_a_definition: { kind: section_definition, section: data_a, selection: NoDuplicates }
          data_b_definition: { kind: section_definition, section: data_b, selection: NoDuplicates }
          data_c_definition: { kind: section_definition, section: data_c, selection: Any }
          data_d_definition: { kind: section_definition, section: data_d, selection: Any }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          data_a_symbol: { kind: external, name: "data_a", section: data_a, value: 0 }
          data_b_symbol: { kind: external, name: "data_b", section: data_b, value: 0 }
          data_c_symbol: { kind: external, name: "data_c", section: data_c, value: 0 }
          data_d_symbol: { kind: external, name: "data_d", section: data_d, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_rdata_policy.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, other_section: ".data", other_offset: 8, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 16, type: u64, nonzero: true, other_section: ".data", other_offset: 24, relation: equal }
  }
}
