test:
{
  artifacts:
  {
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64,
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
    a_lib:
    {
      file_name: "a.lib"
      coff: { library: { second_linker_member: true, members: { a_member: {
        path: "a.obj"
        object: { machine: x64, sections: { a: { name: ".a", permissions: (read, write), content: initialized_data, data: { text: "a" } } } }
      } } } }
    }
    b_lib:
    {
      file_name: "b.lib"
      coff: { library: { second_linker_member: true, members: { b_member: {
        path: "b.obj"
        object: { machine: x64, sections: { b: { name: ".b", permissions: (read, write), content: initialized_data, data: { text: "b" } } } }
      } } } }
    }
    all_image: { file_name: "all_libs.exe", pe: {} }
    a_image: { file_name: "only_a.exe", pe: {} }
    b_image: { file_name: "only_b.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:all_libs.exe entry.obj /wholearchive a.lib b.lib", artifact: all_image }
    link: { args: "/subsystem:console /entry:entry /out:only_a.exe entry.obj /wholearchive:a.lib a.lib b.lib", artifact: a_image }
    link: { args: "/subsystem:console /entry:entry /out:only_b.exe /wholearchive:b.lib a.lib b.lib entry.obj", artifact: b_image }
  }
  steps:
  {
    expect_pe: { artifact: all_image, expected: { pe: { sections: { ".a": {}, ".b": {} } } } }
    expect_pe: { artifact: a_image, expected: { pe: { sections: { ".a": {}, @absent ".b" } } } }
    expect_pe: { artifact: b_image, expected: { pe: { sections: { @absent ".a", ".b": {} } } } }
  }
}
