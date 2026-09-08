test:
{
  artifacts:
  {
    weak_obj:
    {
      file_name: "weak.obj"
      coff: { object: { machine: x64,
        sections: { a: { name: ".a", permissions: (read, write), content: initialized_data, data: { text: "a" } } }
        symbols: {
          a: { kind: static, name: "_a", section: a, value: 0 }
          w: { kind: weak, name: "w", fallback: a, search: search_library }
        }
      } }
    }
    common_obj: { file_name: "common.obj", coff: { object: { machine: x64, symbols: { w: { kind: common, name: "w", size: 2 } } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        data: { hex: "48c7c000000000c3" }
        // ret
        relocations: { w_ref: { type: Addr32Nb, offset: 0, symbol: w } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        w: { kind: undefined, name: "w" }
      } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:entry /out:a.exe common.obj weak.obj entry.obj" }
    link: { args: "/subsystem:console /entry:entry /out:a.exe weak.obj common.obj entry.obj", artifact: image }
  }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".bss": { virtual_size: 2, file_size: 0 } } } } } }
}
