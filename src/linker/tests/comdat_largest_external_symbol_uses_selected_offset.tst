// Chromium has duplicate vftable COMDATs where the referencing copy is an
// IMAGE_COMDAT_SELECT_ANY section with the public vftable at offset 0, while
// the selected IMAGE_COMDAT_SELECT_LARGEST copy has the same public symbol at
// offset 8. Relocations in the discarded object must resolve to the selected
// public symbol, not to the discarded section.
test:
{
  artifacts:
  {
    discarded_obj:
    {
      file_name: "discarded.obj"
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
            data: { zero: 16 }
            relocations:
            {
              vftable_ref: { type: Addr64, offset: 0, symbol: vftable }
              force_selected_ref: { type: Addr64, offset: 8, symbol: force_selected }
            }
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
          force_selected: { kind: undefined, name: "force_selected" }
          entry: { kind: external_function, name: "entry", section: text, value: 0 }
        }
      } }
    }
    selected_lib:
    {
      file_name: "selected.lib"
      coff: { library:
      {
        second_linker_member: true
        members:
        {
          selected:
          {
            path: "selected.obj"
            object:
            {
              machine: x64
              sections:
              {
                vftable: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 24 } }
                force_text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
              }
              symbols:
              {
                vftable_def: { kind: section_definition, section: vftable, selection: Largest }
                vftable: { kind: external, name: "??_7X@@6B@", section: vftable, value: 8 }
                force_selected: { kind: external_function, name: "force_selected", section: force_text, value: 0 }
              }
            }
          }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/nodefaultlib /subsystem:console /entry:entry /out:a.exe /opt:ref,noicf discarded.obj selected.lib", artifact: image } }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".rdata": {}, ".data": {}, ".text": {} } } } }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, target_section: ".rdata", target_offset: 8 }
    expect_pe_word: { artifact: image, section: ".text", offset: 3, type: rel32, target_section: ".rdata", target_offset: 8 }
  }
}
