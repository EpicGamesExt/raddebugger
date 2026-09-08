test:
{
  artifacts:
  {
    leader_object:
    {
      file_name: "icf_chain_leader.obj"
      coff: { object:
      {
        machine: x64
        sections: { leader: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c390" } } }
        symbols:
        {
          leader_definition: { kind: section_definition, section: leader, selection: NoDuplicates }
          leader: { kind: external_function, name: "leader", section: leader, value: 0 }
        }
      } }
    }
    duplicate_object:
    {
      file_name: "icf_chain_duplicate.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          duplicate: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000" }
            relocations:
            {
              duplicate: { type: Addr64, offset: 0, symbol: local_duplicate }
              leader: { type: Addr64, offset: 8, symbol: leader }
            }
          }
        }
        symbols:
        {
          duplicate_definition: { kind: section_definition, section: duplicate, selection: Largest }
          duplicate: { kind: external_function, name: "dup", section: duplicate, value: 0 }
          local_duplicate: { kind: static, name: "local_dup", section: duplicate, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
          leader: { kind: undefined_function, name: "leader" }
        }
      } }
    }
    selected_object:
    {
      file_name: "icf_chain_selected.obj"
      coff: { object:
      {
        machine: x64
        sections: { duplicate: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c390" } } }
        symbols:
        {
          duplicate_definition: { kind: section_definition, section: duplicate, selection: Largest }
          duplicate: { kind: external_function, name: "dup", section: duplicate, value: 0 }
        }
      } }
    }
    entry_object:
    {
      file_name: "icf_chain_entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external_function, name: "entry", section: entry, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_chain_leader.obj icf_chain_duplicate.obj icf_chain_selected.obj icf_chain_entry.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, other_section: ".data", other_offset: 8, relation: equal }
  }
}
