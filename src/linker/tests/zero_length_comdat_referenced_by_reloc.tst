// A referenced zero-sized COMDAT symbol is meaningful enough for
// relocations, even though the COMDAT contributes no bytes to the image.
test:
{
  artifacts:
  {
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
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
          pad: { name: ".rdata$a", permissions: (read), content: initialized_data, alignment: 1, data: { text: "xy" } }
          empty: { name: ".rdata$b", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { zero: 0 } }
          data:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            data: { zero: 8 }
            relocations: { empty_ref: { type: Addr64, offset: 0, symbol: empty } }
          }
        }
        symbols:
        {
          empty_def: { kind: section_definition, section: empty, selection: Any }
          empty: { kind: external, name: "EMPTY", section: empty, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref entry.obj ref.obj", artifact: image } }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".rdata": {}, ".data": {} } } } }
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, target_section: ".rdata", target_offset: 2 }
    expect_pe_bytes: { artifact: image, section: ".rdata", offset: 0, hex: "7879" }
  }
}
