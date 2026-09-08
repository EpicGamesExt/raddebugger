test:
{
  artifacts:
  {
    sec_defn_obj:
    {
      file_name: "sec_defn.obj"
      coff: { object:
      {
        machine: x64
        sections: { mysect: { name: ".mysect", permissions: (read), content: initialized_data, alignment: 1, data: { hex: "010203" } } }
        symbols: { mysect_definition: { kind: section_definition, section: mysect, selection: Null } }
      } }
    }
    main_obj:
    {
      file_name: "main.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          data:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            data: { zero: 4 }
            relocations: { mysect_voff: { type: Addr32Nb, offset: 0, symbol: mysect } }
          }
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols:
        {
          mysect: { kind: undefined_section, name: ".mysect", value: 1073741888 }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "undef_section.exe", pe: {} }
  }
  build:
  {
    // The section symbol's expected flags select .mysect and Addr32Nb stores its image-relative offset.
    link: { args: "/subsystem:console /entry:my_entry /out:undef_section.exe main.obj sec_defn.obj", artifact: image }
  }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u32, target_section: ".mysect", target_offset: 0, target_address: rva }
    expect_pe_bytes: { artifact: image, section: ".mysect", offset: 0, hex: "010203" }
  }
}
