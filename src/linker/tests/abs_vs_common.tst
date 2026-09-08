test:
{
  artifacts:
  {
    common_obj: { file_name: "common.obj", coff: { object: { machine: x64, symbols: { foo: { kind: common, name: "foo", size: 321 } } } } }
    abs_obj: { file_name: "abs.obj", coff: { object: { machine: x64, symbols: { foo: { kind: absolute, name: "foo", value: 4660, storage: external } } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } }
      } }
    }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:my_entry /out:common_after_absolute_definition_first.exe abs.obj common.obj entry.obj", expect_exit: any }
    // TODO: validate that linker issues multiply defined symbol error
    link: { args: "/subsystem:console /entry:my_entry /out:common_after_absolute_common_first.exe common.obj abs.obj entry.obj", when_previous_exit: 0, expect_exit: 24 }
  }
  steps: {}
}
