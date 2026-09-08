test:
{
  artifacts:
  {
    a_obj: { file_name: "a.obj", coff: { object: { machine: x64, symbols: {
      q: { kind: absolute, name: "q", value: 273, storage: external }
      w: { kind: weak, name: "w", fallback: q, search: search_library }
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
    a_first_image: { file_name: "a_first.exe", pe: {} }
    entry_first_image: { file_name: "entry_first.exe", pe: {} }
  }
  build:
  {
    // linker must pick weak symbol from a.obj
    link: { args: "/subsystem:console /entry:entry /out:a_first.exe a.obj entry.obj", artifact: a_first_image }
    // linker must pick weak symbol from entry.obj
    link: { args: "/subsystem:console /entry:entry /out:entry_first.exe entry.obj a.obj", artifact: entry_first_image }
  }
  steps:
  {
    expect_pe: { artifact: a_first_image, expected: { pe: { sections: { ".text": { data: 48c7c011010000c3 } } } } }
    expect_pe: { artifact: entry_first_image, expected: { pe: { sections: { ".text": { data: 48c7c022020000c3 } } } } }
  }
}
