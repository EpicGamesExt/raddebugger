test:
{
  artifacts:
  {
    a_object:
    {
      file_name: "icf_comdat_reloc_a.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          fn_a:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            // mov rax, shared
            // ret
            data: { hex: "48b80000000000000000c3" }
            relocations: { shared: { type: Addr64, offset: 2, symbol: shared_local_a } }
          }
          shared: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "00" } }
        }
        symbols:
        {
          fn_a_definition: { kind: section_definition, section: fn_a, selection: NoDuplicates }
          shared_definition: { kind: section_definition, section: shared, selection: Any }
          fn_a_symbol: { kind: external_function, name: "fn_a", section: fn_a, value: 0 }
          shared_symbol: { kind: external, name: "shared", section: shared, value: 0 }
          shared_local_a: { kind: static, name: "shared_local_a", section: shared, value: 0 }
        }
      } }
    }
    b_object:
    {
      file_name: "icf_comdat_reloc_b.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          fn_b:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            // mov rax, shared
            // ret
            data: { hex: "48b80000000000000000c3" }
            relocations: { shared: { type: Addr64, offset: 2, symbol: shared_local_b } }
          }
          shared: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "00" } }
        }
        symbols:
        {
          fn_b_definition: { kind: section_definition, section: fn_b, selection: NoDuplicates }
          shared_definition: { kind: section_definition, section: shared, selection: Any }
          fn_b_symbol: { kind: external_function, name: "fn_b", section: fn_b, value: 0 }
          shared_symbol: { kind: external, name: "shared", section: shared, value: 0 }
          shared_local_b: { kind: static, name: "shared_local_b", section: shared, value: 0 }
        }
      } }
    }
    entry_object:
    {
      file_name: "icf_comdat_reloc_entry.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000" }
            relocations:
            {
              a: { type: Addr64, offset: 0, symbol: fn_a }
              b: { type: Addr64, offset: 8, symbol: fn_b }
            }
          }
        }
        symbols:
        {
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          fn_a: { kind: undefined_function, name: "fn_a" }
          fn_b: { kind: undefined_function, name: "fn_b" }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses icf_comdat_reloc_entry.obj icf_comdat_reloc_a.obj icf_comdat_reloc_b.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, other_section: ".data", other_offset: 8, relation: equal }
  }
}
