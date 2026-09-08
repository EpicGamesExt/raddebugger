test:
{
  artifacts:
  {
    weak_obj: { file_name: "weak.obj", coff: { object: { machine: x64, symbols: {
      b: { kind: absolute, name: "b", value: 3271557120, storage: external }
      a: { kind: weak, name: "a", fallback: b, search: search_library }
    } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, data: { hex: "00000000" }
        relocations: { a_ref: { type: Addr32Nb, offset: 0, symbol: a } }
      } }, symbols: {
        a: { kind: undefined, name: "a" }
        entry: { kind: external, name: "entry", section: text, value: 0 }
      } } }
    }
  }
  build:
  {
    // undefined symbol must always replace weak symbol with search library
    link: { args: "/subsystem:console /out:a.exe /entry:entry entry.obj weak.obj", expect_exit: 47 }
  }
  steps: {}
}
