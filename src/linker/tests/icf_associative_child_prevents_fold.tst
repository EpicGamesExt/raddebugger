test:
{
  artifacts:
  {
    object:
    {
      file_name: "icf_associative_child.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          fn_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          fn_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          handler_a: { name: ".xdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "01020304" } }
          handler_b: { name: ".xdata", permissions: (read), content: initialized_data, alignment: 4, flags: (link_comdat), data: { hex: "04030201" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000" }
            relocations:
            {
              a: { type: Addr64, offset: 0, symbol: fn_a_symbol }
              b: { type: Addr64, offset: 8, symbol: fn_b_symbol }
            }
          }
        }
        symbols:
        {
          fn_a_definition: { kind: section_definition, section: fn_a, selection: NoDuplicates }
          fn_b_definition: { kind: section_definition, section: fn_b, selection: NoDuplicates }
          handler_a_definition: { kind: section_definition, section: handler_a, selection: Associative, associate: fn_a }
          handler_b_definition: { kind: section_definition, section: handler_b, selection: Associative, associate: fn_b }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          fn_a_symbol: { kind: external_function, name: "fn_a", section: fn_a, value: 0 }
          fn_b_symbol: { kind: external_function, name: "fn_b", section: fn_b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_associative_child.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true, other_section: ".data", other_offset: 0, relation: not_equal }
  }
}
