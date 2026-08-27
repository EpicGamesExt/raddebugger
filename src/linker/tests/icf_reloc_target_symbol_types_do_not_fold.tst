// ICF must preserve identical sections with different symbol targets
test:
{
  artifacts:
  {
    entry_object:
    {
      file_name: "icf_interp_entry.obj"
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
            data: { hex: "000000000000000000000000000000000000000000000000" }
            relocations:
            {
              regular: { type: Addr64, offset: 0, symbol: fn_regular }
              common: { type: Addr64, offset: 8, symbol: fn_common }
              absolute: { type: Addr64, offset: 16, symbol: fn_abs }
            }
          }
        }
        symbols:
        {
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
          fn_regular: { kind: undefined, name: "fn_regular" }
          fn_common: { kind: undefined, name: "fn_common" }
          fn_abs: { kind: undefined, name: "fn_abs" }
        }
      } }
    }
    regular_object:
    {
      file_name: "icf_interp_regular.obj"
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
          target: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, data: { hex: "00" } }
        }
        symbols:
        {
          fn_definition: { kind: section_definition, section: fn, selection: NoDuplicates }
          fn: { kind: external_function, name: "fn_regular", section: fn, value: 0 }
          target: { kind: external, name: "target_regular", section: target, value: 0 }
        }
      } }
    }
    common_object:
    {
      file_name: "icf_interp_common.obj"
      coff: { object:
      {
        machine: x64
        sections: { fn: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "48c7c000000000c3" }, relocations: { target: { type: Rel32, offset: 3, symbol: target } } } }
        symbols:
        {
          fn_definition: { kind: section_definition, section: fn, selection: NoDuplicates }
          fn: { kind: external_function, name: "fn_common", section: fn, value: 0 }
          target: { kind: common, name: "target_common", size: 8 }
        }
      } }
    }
    absolute_object:
    {
      file_name: "icf_interp_abs.obj"
      coff: { object:
      {
        machine: x64
        sections: { fn: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "48c7c000000000c3" }, relocations: { target: { type: Rel32, offset: 3, symbol: target } } } }
        symbols:
        {
          fn_definition: { kind: section_definition, section: fn, selection: NoDuplicates }
          fn: { kind: external_function, name: "fn_abs", section: fn, value: 0 }
          target: { kind: absolute, name: "target_abs", value: 4660, storage: external }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,icf icf_interp_entry.obj icf_interp_regular.obj icf_interp_common.obj icf_interp_abs.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, other_section: ".data", other_offset: 8, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, other_section: ".data", other_offset: 16, relation: not_equal }
    expect_pe_word: { artifact: image, section: ".data", offset: 8, type: u64, other_section: ".data", other_offset: 16, relation: not_equal }
  }
}
