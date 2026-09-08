test:
{
  artifacts:
  {
    vftable_obj:
    {
      file_name: "vftable.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { concat: { hex: "488d0500000000" // lea rax, [rip + ??_7B]
                              hex: "c3" // ret
            } }
            relocations: { vftable_b_ref: { type: Rel32, offset: 3, symbol: vftable_b } }
          }
          vftable_a: { name: ".rdata$vt", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } }
          vftable_b: { name: ".rdata$vt", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 8
            data: { zero: 16 }
            relocations:
            {
              vftable_a_ref: { type: Addr64, offset: 0, symbol: vftable_a }
              vftable_b_ref: { type: Addr64, offset: 8, symbol: vftable_b }
            }
          }
        }
        symbols:
        {
          vftable_a_def: { kind: section_definition, section: vftable_a, selection: Any }
          vftable_b_def: { kind: section_definition, section: vftable_b, selection: Any }
          rtti_a: { kind: external, name: "??_R4A@@6B@", section: vftable_a, value: 0 }
          vftable_a: { kind: external, name: "??_7A@@6B@", section: vftable_a, value: 8 }
          rtti_b: { kind: external, name: "??_R4B@@6B@", section: vftable_b, value: 0 }
          vftable_b: { kind: external, name: "??_7B@@6B@", section: vftable_b, value: 8 }
          entry: { kind: external_function, name: "entry", section: entry, value: 0 }
          addresses: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/nodefaultlib /subsystem:console /entry:entry /out:a.exe /opt:ref,icf /include:addresses vftable.obj", artifact: image } }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".rdata": {}, ".data": {}, ".text": {} } } } }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true, modulo: 16, remainder: 8 }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true, modulo: 16, remainder: 8 }
    expect_pe_word: { artifact: image, section: ".text", offset: 3, type: rel32, other_section: ".data", other_offset: 8, other_type: u64, relation: equal }
  }
}
