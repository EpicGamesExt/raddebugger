// Duplicate COMDAT sections can have identical bytes while their symbol tables
// disagree about where a same-named public symbol points inside the section.
// This mirrors MSVC vftable COMDATs: the selected copy may have leading RTTI data
// at offset 0 and the vftable symbol at offset 8, while a discarded copy's
// vftable symbol is at offset 0. Relocations against the discarded symbol must
// use the selected symbol's value, not just the selected section contribution
// plus the discarded symbol's original offset.
test:
{
  artifacts:
  {
    leader_obj:
    {
      file_name: "leader.obj"
      coff: { object:
      {
        machine: x64
        sections: { vftable: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } } }
        symbols:
        {
          vftable_def: { kind: section_definition, section: vftable, selection: Any }
          vftable: { kind: external, name: "??_7X@@6B@", section: vftable, value: 8 }
        }
      } }
    }
    ref_obj:
    {
      file_name: "ref.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          vftable: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } }
          ptr:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 8
            data: { zero: 8 }
            relocations: { vftable_ref: { type: Addr64, offset: 0, symbol: vftable } }
          }
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { concat: { hex: "488d0500000000" // lea rax, [rip + ??_7X]
                              hex: "c3"
            } }
            relocations: { vftable_ref: { type: Rel32, offset: 3, symbol: vftable } }
          }
        }
        symbols:
        {
          vftable_def: { kind: section_definition, section: vftable, selection: Any }
          vftable: { kind: external, name: "??_7X@@6B@", section: vftable, value: 0 }
          entry: { kind: external_function, name: "entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/nodefaultlib /subsystem:console /entry:entry /out:a.exe leader.obj ref.obj", artifact: image } }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".rdata": {}, ".data": {}, ".text": {} } } } }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, target_section: ".rdata", target_offset: 8 }
    expect_pe_word: { artifact: image, section: ".text", offset: 3, type: rel32, target_section: ".rdata", target_offset: 8 }
  }
}
