test:
{
  artifacts:
  {
    include_lib:
    {
      file_name: "include.lib"
      coff: { library: { second_linker_member: true, members: { include_member: {
        path: "include.obj"
        object: { machine: x64,
          sections: { data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "foo" } } }
          symbols: { foo: { kind: external, name: "foo", section: data, value: 0 } }
        }
      } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        data: { hex: "48c7c000000000c3" }
        // ret
        relocations: { entry_ref: { type: Addr32Nb, offset: 0, symbol: entry } }
      } }, symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build:
  {
    // simple include test
    link: { args: "/subsystem:console /entry:entry /out:a.exe /include:foo entry.obj include.lib", artifact: image }
    // test unresolved include
    link: { args: "/subsystem:console /entry:entry /out:a.exe /include:ewq entry.obj", expect_exit: 47 }
  }
  steps:
  {
    // validate that linker pulled-in include.obj
    expect_pe: { artifact: image, expected: { pe: { sections: { ".data": { data: 666f6f } } } } }
  }
}
