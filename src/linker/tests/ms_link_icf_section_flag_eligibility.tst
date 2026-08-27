test:
{
  // ret
  artifacts:
  {
    object:
    {
      file_name: "ms_icf_flags.obj"
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
          addresses:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 1
            data: { zero: 96 }
            relocations:
            {
              code_comdat_a_ref: { type: Addr64, offset: 0, symbol: code_comdat_a_symbol }
              code_comdat_b_ref: { type: Addr64, offset: 8, symbol: code_comdat_b_symbol }
              code_plain_a_ref: { type: Addr64, offset: 16, symbol: code_plain_a_symbol }
              code_plain_b_ref: { type: Addr64, offset: 24, symbol: code_plain_b_symbol }
              rdata_comdat_a_ref: { type: Addr64, offset: 32, symbol: rdata_comdat_a_symbol }
              rdata_comdat_b_ref: { type: Addr64, offset: 40, symbol: rdata_comdat_b_symbol }
              wdata_comdat_a_ref: { type: Addr64, offset: 48, symbol: wdata_comdat_a_symbol }
              wdata_comdat_b_ref: { type: Addr64, offset: 56, symbol: wdata_comdat_b_symbol }
              ro_code_comdat_a_ref: { type: Addr64, offset: 64, symbol: ro_code_comdat_a_symbol }
              ro_code_comdat_b_ref: { type: Addr64, offset: 72, symbol: ro_code_comdat_b_symbol }
              rw_code_comdat_a_ref: { type: Addr64, offset: 80, symbol: rw_code_comdat_a_symbol }
              rw_code_comdat_b_ref: { type: Addr64, offset: 88, symbol: rw_code_comdat_b_symbol }
            }
          }
          code_comdat_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          code_comdat_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          code_plain_a: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          code_plain_b: { name: ".text$mn", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          ro_code_comdat_a: { name: ".text$mn", permissions: (read), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          ro_code_comdat_b: { name: ".text$mn", permissions: (read), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          rw_code_comdat_a: { name: ".text$mn", permissions: (read, write), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          rw_code_comdat_b: { name: ".text$mn", permissions: (read, write), content: code, alignment: 1, flags: (link_comdat), data: { hex: "c3" } }
          rdata_comdat_a: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0102030405060708" } }
          rdata_comdat_b: { name: ".rdata$mn", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0102030405060708" } }
          wdata_comdat_a: { name: ".data$mn", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0102030405060708" } }
          wdata_comdat_b: { name: ".data$mn", permissions: (read, write), content: initialized_data, alignment: 1, flags: (link_comdat), data: { hex: "0102030405060708" } }
        }
        symbols:
        {
          code_comdat_a_definition: { kind: section_definition, section: code_comdat_a, selection: Any }
          code_comdat_b_definition: { kind: section_definition, section: code_comdat_b, selection: Any }
          rdata_comdat_a_definition: { kind: section_definition, section: rdata_comdat_a, selection: Any }
          rdata_comdat_b_definition: { kind: section_definition, section: rdata_comdat_b, selection: Any }
          wdata_comdat_a_definition: { kind: section_definition, section: wdata_comdat_a, selection: Any }
          wdata_comdat_b_definition: { kind: section_definition, section: wdata_comdat_b, selection: Any }
          ro_code_comdat_a_definition: { kind: section_definition, section: ro_code_comdat_a, selection: Any }
          ro_code_comdat_b_definition: { kind: section_definition, section: ro_code_comdat_b, selection: Any }
          rw_code_comdat_a_definition: { kind: section_definition, section: rw_code_comdat_a, selection: Any }
          rw_code_comdat_b_definition: { kind: section_definition, section: rw_code_comdat_b, selection: Any }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
          code_comdat_a_symbol: { kind: external_function, name: "code_comdat_a", section: code_comdat_a, value: 0 }
          code_comdat_b_symbol: { kind: external_function, name: "code_comdat_b", section: code_comdat_b, value: 0 }
          code_plain_a_symbol: { kind: external_function, name: "code_plain_a", section: code_plain_a, value: 0 }
          code_plain_b_symbol: { kind: external_function, name: "code_plain_b", section: code_plain_b, value: 0 }
          ro_code_comdat_a_symbol: { kind: external_function, name: "ro_code_comdat_a", section: ro_code_comdat_a, value: 0 }
          ro_code_comdat_b_symbol: { kind: external_function, name: "ro_code_comdat_b", section: ro_code_comdat_b, value: 0 }
          rw_code_comdat_a_symbol: { kind: external_function, name: "rw_code_comdat_a", section: rw_code_comdat_a, value: 0 }
          rw_code_comdat_b_symbol: { kind: external_function, name: "rw_code_comdat_b", section: rw_code_comdat_b, value: 0 }
          rdata_comdat_a_symbol: { kind: external, name: "rdata_comdat_a", section: rdata_comdat_a, value: 0 }
          rdata_comdat_b_symbol: { kind: external, name: "rdata_comdat_b", section: rdata_comdat_b, value: 0 }
          wdata_comdat_a_symbol: { kind: external, name: "wdata_comdat_a", section: wdata_comdat_a, value: 0 }
          wdata_comdat_b_symbol: { kind: external, name: "wdata_comdat_b", section: wdata_comdat_b, value: 0 }
          addresses_symbol: { kind: external, name: "addresses", section: addresses, value: 0 }
        }
      } }
    }
    image: { file_name: "ms_icf_flags.exe", pe: {} }
  }

  build:
  {
    compile_link:
    {
      tool: msvc
      artifact: image
      args: "ms_icf_flags.obj /link /nologo /nodefaultlib /subsystem:console /entry:entry /out:ms_icf_flags.exe /opt:ref,icf /include:addresses"
    }
  }

  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, nonzero: true }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, other_section: ".data", other_offset: 8, relation: equal } // executable code COMDATs fold
    expect_pe_word: { artifact: image, section: ".data", offset: 64, type: u64, other_section: ".data", other_offset: 72, relation: equal } // read-only code COMDATs fold
    expect_pe_word: { artifact: image, section: ".data", offset: 16, type: u64, other_section: ".data", other_offset: 24, relation: not_equal } // non-COMDAT code does not fold
    expect_pe_word: { artifact: image, section: ".data", offset: 48, type: u64, other_section: ".data", other_offset: 56, relation: not_equal } // writable data COMDATs do not fold
    expect_pe_word: { artifact: image, section: ".data", offset: 80, type: u64, other_section: ".data", other_offset: 88, relation: not_equal } // writable code COMDATs do not 
    expect_pe_word: { artifact: image, section: ".data", offset: 32, type: u64, other_section: ".data", other_offset: 40, relation: equal } // read-only data COMDATs fold
  }
}
