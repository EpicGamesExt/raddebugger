test:
{
  artifacts:
  {
    undef_obj: { file_name: "undef.obj", coff: { object: { machine: x64, symbols: { undef: { kind: undefined, name: "undef" } } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" }
      } }, symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } } } }
    }
  }
  build:
  {
    // try linking unreferenced unresolved symbol, this must link
    link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj", expect_exit: nonzero }
  }
  steps: {}
}
