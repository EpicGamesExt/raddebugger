test:
{
  artifacts:
  {
    communal_obj: { file_name: "communal.obj", coff: { object: { machine: x64, symbols: { test: { kind: common, name: "TEST", size: 1 } } } } }
    defn_obj:
    {
      file_name: "defn.obj"
      coff: { object: { machine: x64,
        sections: { data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "test" } } }
        symbols: { test: { kind: external, name: "TEST", section: data, value: 0 } }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        data: { hex: "48c7c000000000c3" }
        // ret
        relocations: { test_ref: { type: Addr32Nb, offset: 0, symbol: test } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        test: { kind: undefined, name: "TEST" }
      } } }
    }
    a_image: { file_name: "a.exe", pe: {} }
    b_image: { file_name: "b.exe", pe: {} }
  }
  build:
  {
    // linker should replace communal TEST with .data TEST
    link: { args: "/subsystem:console /entry:entry /out:a.exe communal.obj defn.obj entry.obj", artifact: a_image }
    link: { args: "/subsystem:console /entry:entry /out:b.exe defn.obj communal.obj entry.obj", artifact: b_image }
  }
  steps:
  {
    expect_pe: { artifact: a_image, expected: { pe: { sections: { ".data": { data: 74657374 } } } } }
    expect_pe: { artifact: b_image, expected: { pe: { sections: { ".data": { data: 74657374 } } } } }
  }
}
