test:
{
  // xor rax, rax
  // ret
  // ret
  artifacts:
  {
    object:
    {
      file_name: "ms_icf.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          entry:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { hex: "c3" }
          }
          a:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            data: { hex: "4831c0c3" }
          }
          b:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            data: { hex: "4831c0c3" }
          }
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { zero: 16 }
            relocations:
            {
              a_ref: { type: Addr64, offset: 0, symbol: a_symbol }
              b_ref: { type: Addr64, offset: 8, symbol: b_symbol }
            }
          }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: Any }
          b_definition: { kind: section_definition, section: b, selection: Any }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          a_symbol: { kind: external_function, name: "a", section: a, value: 0 }
          b_symbol: { kind: external_function, name: "b", section: b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "ms_icf.exe", pe: {} }
  }

  build:
  {
    compile_link:
    {
      tool: msvc
      artifact: image
      args: "ms_icf.obj /link /nologo /nodefaultlib /subsystem:console /entry:entry /out:ms_icf.exe /opt:ref,icf /include:a /include:b /include:addresses"
    }
  }

  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, other_section: ".data", other_offset: 8, relation: equal } // COMDAT are folded
  }
}
