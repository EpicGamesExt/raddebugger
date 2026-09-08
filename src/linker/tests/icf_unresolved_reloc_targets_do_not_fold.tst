// ICF must preserve sections with unresolved target symbols (with /FORCE)
test:
{
  artifacts:
  {
    entry_object:
    {
      file_name: "icf_unresolved_entry.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } // ret
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { hex: "00000000000000000000000000000000" }
            relocations:
            {
              undef: { type: Addr64, offset: 0, symbol: fn_undef }
              weak: { type: Addr64, offset: 8, symbol: fn_weak }
            }
          }
        }
        symbols:
        {
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
          fn_undef: { kind: undefined, name: "fn_undef" }
          fn_weak: { kind: undefined, name: "fn_weak" }
        }
      } }
    }
    undef_object:
    {
      file_name: "icf_unresolved_undef.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          fn:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            // mov rax, target
            // ret
            data: { hex: "48c7c000000000c3" }
            relocations: { target: { type: Rel32, offset: 3, symbol: target } }
          }
        }
        symbols:
        {
          fn_definition: { kind: section_definition, section: fn, selection: NoDuplicates }
          fn: { kind: external_function, name: "fn_undef", section: fn, value: 0 }
          target: { kind: undefined, name: "target_undef" }
        }
      } }
    }
    weak_object:
    {
      file_name: "icf_unresolved_weak.obj"
      coff: { object:
      {
        machine: x64
        sections: { fn: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "48c7c000000000c3" }, relocations: { target: { type: Rel32, offset: 3, symbol: target } } } }
        symbols:
        {
          fn_definition: { kind: section_definition, section: fn, selection: NoDuplicates }
          fn: { kind: external_function, name: "fn_weak", section: fn, value: 0 }
          fallback: { kind: absolute, name: "target_weak_fallback", value: 0, storage: external }
          target: { kind: weak, name: "target_weak", fallback: fallback, search: no_library }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/force /subsystem:console /entry:entry /out:a.exe /opt:ref,icf icf_unresolved_entry.obj icf_unresolved_undef.obj icf_unresolved_weak.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, nonzero: true, other_section: ".data", other_offset: 0, relation: not_equal }
  }
}
