// MSVC vftables use COMDAT sections whose public symbol can start past the
// section definition symbol; references to a replaced copy must target the winner.
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
        sections: { data: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } } }
        symbols:
        {
          data_def: { kind: section_definition, section: data, selection: Any }
          foo: { kind: external, name: "foo", section: data, value: 8 }
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
          data: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 8, flags: (link_comdat), data: { zero: 16 } }
          ptr:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            alignment: 8
            data: { zero: 8 }
            relocations: { foo_ref: { type: Addr64, offset: 0, symbol: foo } }
          }
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { concat: { hex: "488d0500000000" // lea rax, [rip + foo]
                              hex: "c3"
            } }
            relocations: { foo_ref: { type: Rel32, offset: 3, symbol: foo } }
          }
        }
        symbols:
        {
          data_def: { kind: section_definition, section: data, selection: Any }
          foo: { kind: external, name: "foo", section: data, value: 8 }
          entry: { kind: external, name: "entry", section: text, value: 0 }
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
