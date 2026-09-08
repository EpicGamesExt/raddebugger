test:
{

  artifacts:
  {
    bar_lib:
    {
      file_name: "defer_duplicate_bar.lib"
      coff: { library:
      {
        members:
        {
          bar_scaffold: { dll_import: { name: "bar.dll", machine: x64, timestamp: 0 } }
          bar_import: { import: { dll: "bar.dll", name: "bar", machine: x64, timestamp: 0, type: code, lookup: name, hint: 0 } }
          qwe_member:
          {
            path: "member_2.obj"
            object:
            {
              machine: x64
              sections:
              {
                text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "ff2500000000" } }
              }
              symbols:
              {
                imp_bar: { kind: undefined, name: "__imp_bar" }
                qwe: { kind: external_function, name: "qwe", section: text, value: 0 }
              }
            }
          }
        }
      } }
    }
    foo_lib:
    {
      file_name: "defer_duplicate_foo.lib"
      coff: { library:
      {
        members:
        {
          foo_scaffold: { dll_import: { name: "foo.dll", machine: x64, timestamp: 0 } }
          bar_import: { import: { dll: "foo.dll", name: "bar", machine: x64, timestamp: 0, type: code, lookup: name, hint: 0 } }
          thunk_member:
          {
            path: "member_2.obj"
            object:
            {
              machine: x64
              sections:
              {
                text:
                {
                  name: ".text"
                  permissions: (read, execute)
                  content: code
                  data: { hex: "ff2500000000" }
                  relocations: { bar_ref: { type: Rel32, offset: 2, symbol: bar } }
                }
              }
              symbols:
              {
                bar: { kind: undefined, name: "bar" }
                qwe: { kind: undefined, name: "qwe" }
                thunk: { kind: external_function, name: "thunk", section: text, value: 0 }
              }
            }
          }
        }
      } }
    }
    entry_obj:
    {
      file_name: "defer_duplicate_entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "defer_duplicate_imp_link.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:defer_duplicate_imp_link.exe defer_duplicate_bar.lib defer_duplicate_foo.lib defer_duplicate_entry.obj /include:thunk"
      artifact: image
    }
  }

  steps:
  {
    // Exactly one import descriptor remains, and it names foo.dll.
    expect_pe_word: { artifact: image, section: ".data", offset: 16, type: u32, target_section: ".data", target_offset: 56, target_address: rva }
    expect_pe_word: { artifact: image, section: ".data", offset: 28, type: u32, target_section: ".data", target_offset: 78, target_address: rva }
    expect_pe_word: { artifact: image, section: ".data", offset: 32, type: u32, target_section: ".data", target_offset: 0, target_address: rva }
    expect_pe_bytes: { artifact: image, section: ".data", offset: 36, hex: "0000000000000000000000000000000000000000" }

    // The sole name import is bar with hint zero. Its nonzero IAT and ILT words
    // are equal and point inside .data at the same hint/name record.
    expect_pe_word:
    {
      artifact: image
      section: ".data"
      offset: 0
      type: u64
      nonzero: true
      target_section: ".data"
      target_offset: 72
      target_address: rva
      other_section: ".data"
      other_offset: 56
      other_type: u64
      relation: equal
    }
    expect_pe_bytes: { artifact: image, section: ".data", offset: 8, hex: "0000000000000000" }
    expect_pe_bytes: { artifact: image, section: ".data", offset: 64, hex: "0000000000000000" }
    expect_pe_bytes: { artifact: image, section: ".data", offset: 72, hex: "000062617200666f6f2e646c6c00" }
  }
}
