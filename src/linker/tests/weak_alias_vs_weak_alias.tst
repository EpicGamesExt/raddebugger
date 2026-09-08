test:
{
  artifacts:
  {
    a_obj: { file_name: "a.obj", coff: { object: { machine: x64, symbols: {
      qwe: { kind: absolute, name: "qwe", value: 273, storage: external }
      sym: { kind: weak, name: "sym", fallback: qwe, search: alias }
    } } } }
    b_obj: { file_name: "b.obj", coff: { object: { machine: x64, symbols: {
      ewq: { kind: absolute, name: "ewq", value: 546, storage: external }
      sym: { kind: weak, name: "sym", fallback: ewq, search: alias }
    } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        // ret
        data: { hex: "48c7c000000000c3" }
        relocations: { sym_ref: { type: Addr32, offset: 3, symbol: sym } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        sym: { kind: undefined, name: "sym" }
      } } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj", expect_exit: 24 } }
  steps: {}
}
