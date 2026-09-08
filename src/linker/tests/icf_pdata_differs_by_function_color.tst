test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_pdata_diff.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          fn_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "b801000000c3" } }
          fn_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "b802000000c3" } }
          xdata_a: { name: ".xdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "01000000" } }
          xdata_b: { name: ".xdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "01000000" } }
          pdata_a: { name: ".pdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "000000000000000000000000" }, relocations: { first: { type: Addr32Nb, offset: 0, symbol: fn_a_symbol }, last: { type: Addr32Nb, offset: 4, symbol: fn_a_symbol }, unwind: { type: Addr32Nb, offset: 8, symbol: unwind_a } } }
          pdata_b: { name: ".pdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "000000000000000000000000" }, relocations: { first: { type: Addr32Nb, offset: 0, symbol: fn_b_symbol }, last: { type: Addr32Nb, offset: 4, symbol: fn_b_symbol }, unwind: { type: Addr32Nb, offset: 8, symbol: unwind_b } } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "000000000000000000000000000000000000000000000000" }
            relocations:
            {
              pdata_a: { type: Addr64, offset: 0, symbol: pdata_a_symbol }
              pdata_b: { type: Addr64, offset: 8, symbol: pdata_b_symbol }
              unwind_a: { type: Addr64, offset: 16, symbol: unwind_a }
            }
          }
        }
        symbols:
        {
          fn_a_definition: { kind: section_definition, section: fn_a, selection: NoDuplicates }
          fn_b_definition: { kind: section_definition, section: fn_b, selection: NoDuplicates }
          xdata_a_definition: { kind: section_definition, section: xdata_a, selection: NoDuplicates }
          xdata_b_definition: { kind: section_definition, section: xdata_b, selection: NoDuplicates }
          pdata_a_definition: { kind: section_definition, section: pdata_a, selection: NoDuplicates }
          pdata_b_definition: { kind: section_definition, section: pdata_b, selection: NoDuplicates }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          fn_a_symbol: { kind: external_function, name: "fn_a", section: fn_a, value: 0 }
          fn_b_symbol: { kind: external_function, name: "fn_b", section: fn_b, value: 0 }
          unwind_a: { kind: external, name: "$unwind$a", section: xdata_a, value: 0 }
          unwind_b: { kind: external, name: "$unwind$b", section: xdata_b, value: 0 }
          pdata_a_symbol: { kind: external, name: "$pdata$a", section: pdata_a, value: 0 }
          pdata_b_symbol: { kind: external, name: "$pdata$b", section: pdata_b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_pdata_diff.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, other_section: ".data", other_offset: 8, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true }
  }
}
