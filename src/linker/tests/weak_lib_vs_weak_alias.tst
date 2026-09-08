test:
{
  artifacts:
  {
    a_obj: { file_name: "a.obj", coff: { object: { machine: x64, symbols: {
      q: { kind: absolute, name: "q", value: 273, storage: external }
      w: { kind: weak, name: "w", fallback: q, search: alias }
    } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        // ret
        data: { hex: "48c7c000000000c3" }
        relocations: { w_ref: { type: Addr32, offset: 3, symbol: w } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        e: { kind: absolute, name: "e", value: 546, storage: external }
        w: { kind: weak, name: "w", fallback: e, search: search_library }
      } } }
    }
  }
  build:
  {
    // linker must pick weak symbol from entry.obj
    link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj a.obj", expect_exit: 24 }
  }
  steps: {}
}
