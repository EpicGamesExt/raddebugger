test:
{
  artifacts:
  {
    test_lib:
    {
      file_name: "test.lib"
      coff: { library: { second_linker_member: true, members: { symbols: {
        path: "obj.obj"
        object: { machine: x64, symbols: {
          eight: { kind: absolute, name: "8", value: 8, storage: external }
          one: { kind: absolute, name: "1", value: 1, storage: external }
          nine: { kind: absolute, name: "9", value: 9, storage: external }
          seven: { kind: absolute, name: "7", value: 7, storage: external }
          four: { kind: absolute, name: "4", value: 4, storage: external }
          five: { kind: absolute, name: "5", value: 5, storage: external }
          two: { kind: absolute, name: "2", value: 2, storage: external }
          three: { kind: absolute, name: "3", value: 3, storage: external }
          six: { kind: absolute, name: "6", value: 6, storage: external }
        } }
      } } } }
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
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe test.lib entry.obj /include:1 /include:2 /include:3 /include:4 /include:5 /include:6 /include:7 /include:8 /include:9" } }
  steps: {}
}
