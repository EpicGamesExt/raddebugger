test:
{
  artifacts:
  {
    bar_lib:
    {
      file_name: "bar.lib"
      coff: { library: { members: {
        import_descriptor: { dll_import: { name: "bar.dll", machine: x64, timestamp: 0 } }
        import_symbol: { import: { dll: "bar.dll", name: "bar", machine: x64, timestamp: 0, type: code, lookup: name, hint: 0 } }
        implementation:
        {
          path: "member_2.obj"
          object: { machine: x64,
            sections: { text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "ff2500000000" } } }
            symbols: {
              imp_bar: { kind: undefined, name: "__imp_bar" }
              qwe: { kind: external_function, name: "qwe", section: text, value: 0 }
            }
          }
        }
      } } }
    }
    foo_lib:
    {
      file_name: "foo.lib"
      coff: { library: { members: {
        import_descriptor: { dll_import: { name: "foo.dll", machine: x64, timestamp: 0 } }
        import_symbol: { import: { dll: "foo.dll", name: "bar", machine: x64, timestamp: 0, type: code, lookup: name, hint: 0 } }
        thunk:
        {
          path: "member_2.obj"
          object: { machine: x64, sections: { text: {
            name: ".text", permissions: (read, execute), content: code, data: { hex: "ff2500000000" }
            relocations: { bar_ref: { type: Rel32, offset: 2, symbol: bar } }
          } }, symbols: {
            bar: { kind: undefined, name: "bar" }
            qwe: { kind: undefined, name: "qwe" }
            thunk: { kind: external_function, name: "thunk", section: text, value: 0 }
          } }
        }
      } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64,
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe bar.lib foo.lib entry.obj /include:thunk" } }
  steps: {}
}
