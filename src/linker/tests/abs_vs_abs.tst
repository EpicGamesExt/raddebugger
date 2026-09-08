test:
{
  artifacts:
  {
    a_obj: { file_name: "a.obj", coff: { object: { machine: x64, symbols: { foo: { kind: absolute, name: "foo", value: 97, storage: external } } } } }
    b_obj: { file_name: "b.obj", coff: { object: { machine: x64, symbols: { foo: { kind: absolute, name: "foo", value: 98, storage: external } } } } }
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
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:error.exe a.obj b.obj entry.obj", expect_exit: 24 } }
  steps: {}
}
