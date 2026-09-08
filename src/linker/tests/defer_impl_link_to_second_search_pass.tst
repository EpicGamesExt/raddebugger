test:
{
  artifacts:
  {
    imp_ref_obj:
    {
      file_name: "imp_ref.obj"
      coff: { object: { machine: x64,
        sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } } }
        symbols: {
          entry: { kind: external_function, name: "entry", section: text, value: 0 }
          imp_foo: { kind: undefined, name: "__imp_foo" }
          func: { kind: undefined, name: "func" }
        }
      } }
    }
    impl_ref_lib:
    {
      file_name: "impl_ref.lib"
      coff: { library: { second_linker_member: true, members: { implementation: {
        path: "impl_ref.obj"
        object: { machine: x64,
          sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } } }
          symbols: {
            func: { kind: external_function, name: "func", section: text, value: 0 }
            foo: { kind: undefined, name: "foo" }
          }
        }
      } } } }
    }
    foo_lib:
    {
      file_name: "foo.lib"
      coff: { library: { second_linker_member: true, members: { imported: { import: {
        dll: "foo.dll", name: "foo", machine: x64, timestamp: 4294967295, type: code, lookup: name, hint: 0
      } } } } }
    }
    foo2_lib:
    {
      file_name: "foo2.lib"
      coff: { library: { second_linker_member: true, members: { imported: { import: {
        dll: "foo.dll", name: "foo", machine: x64, timestamp: 4294967295, type: code, lookup: name, hint: 0
      } } } } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe foo.lib foo2.lib imp_ref.obj impl_ref.lib" } }
  steps: {}
}
