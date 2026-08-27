test:
{
  artifacts:
  {
    undef_obj: { file_name: "undef.obj", coff: { object: { machine: x64, symbols: { undef: { kind: undefined, name: "undef" } } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, data: { hex: "48c7c000000000c3" }
        relocations: { undef_ref: { type: Addr32Nb, offset: 0, symbol: undef } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        undef: { kind: undefined, name: "undef" }
      } } }
    }
  }
  build:
  {
    // try linking unresolved symbol and see if linker picks up on that
    link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj", expect_exit: 47 }
  }
  steps: {}
}
