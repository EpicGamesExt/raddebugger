test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_color_spaces.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          text: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          vftable: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000" }
            relocations:
            {
              text: { type: Addr64, offset: 0, symbol: text_symbol }
              vftable: { type: Addr64, offset: 8, symbol: vftable_symbol }
            }
          }
        }
        symbols:
        {
          text_definition: { kind: section_definition, section: text, selection: NoDuplicates }
          vftable_definition: { kind: section_definition, section: vftable, selection: NoDuplicates }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          text_symbol: { kind: external_function, name: "text", section: text, value: 0 }
          vftable_symbol: { kind: external, name: "??_7type@@6B@", section: vftable, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_color_spaces.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true, other_section: ".data", other_offset: 0, relation: not_equal }
  }
}
