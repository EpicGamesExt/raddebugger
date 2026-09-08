test:
{
  artifacts:
  {
    test_lib:
    {
      file_name: "test.lib"
      coff: { library: { second_linker_member: true, members: { test_member: {
        path: "test.obj"
        object: { machine: unknown,
          sections: { data: {
            name: ".data", permissions: (read, write), content: initialized_data
            data: { concat: { text: "The quick brown fox jumps over the lazy dog", zero: 1 } }
          } }
          symbols: { test: { kind: external, name: "test", section: data, value: 0 } }
        }
      } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1
        data: { hex: "48c7c000000000c3" }
        relocations: { test_ref: { type: Addr32Nb, offset: 3, symbol: test } }
      } }, symbols: {
        test: { kind: undefined, name: "test" }
        entry: { kind: external, name: "my_entry", section: text, value: 7 }
      } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe entry.obj test.lib", artifact: image } }
  steps:
  {
    // was test payload linked?
    expect_pe: { artifact: image, expected: { pe: { sections: { ".data": { data: 54686520717569636b2062726f776e20666f78206a756d7073206f76657220746865206c617a7920646f6700 } } } } }
    // do we have enough bytes to read text?
    expect_pe: { artifact: image, expected: { pe: { sections: { ".text": { virtual_size: 8 } } } } }
    // linker must pull-in test.obj and patch relocation for "test" symbol
    expect_pe_word: { artifact: image, section: ".text", offset: 3, type: u32, target_section: ".data", target_offset: 0, target_address: rva }
  }
}
