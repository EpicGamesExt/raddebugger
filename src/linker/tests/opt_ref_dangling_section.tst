test:
{
  artifacts:
  {
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code
        // mov rax, $imm
        data: { hex: "48c7c000000000c3" }
        // ret
        relocations: { f_ref: { type: Addr32Nb, offset: 0, symbol: f } }
      } }, symbols: {
        f: { kind: undefined, name: "f" }
        entry: { kind: external, name: "entry", section: text, value: 0 }
      } } }
    }
    a_obj:
    {
      file_name: "a.obj"
      coff: { object: { machine: x64, sections: { data: {
        name: ".data", permissions: (read, write), content: initialized_data, flags: (link_comdat)
        data: { concat: { text: "A0000", zero: 1 } }
        relocations: { q_ref: { type: Addr32Nb, offset: 0, symbol: q } }
      } }, symbols: {
        q: { kind: undefined, name: "q" }
        data_def: { kind: section_definition, section: data, selection: Largest }
        f: { kind: external, name: "f", section: data, value: 0 }
      } } }
    }
    b_lib:
    {
      file_name: "b.lib"
      coff: { library: { second_linker_member: true, members: { b_member: {
        path: "b.obj"
        object: { machine: x64, sections: {
          q: { name: ".q", permissions: (read, write), content: initialized_data, data: { hex: "01020304" } }
          data: {
            name: ".data", permissions: (read, write), content: initialized_data, flags: (link_comdat)
            data: { concat: { text: "BBBBBBBBBBBBBBB", zero: 1 } }
          }
        }, symbols: {
          q: { kind: external, name: "q", section: q, value: 0 }
          data_def: { kind: section_definition, section: data, selection: Largest }
          f: { kind: external, name: "f", section: data, value: 0 }
        } }
      } } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.lib", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".q": {} } } } } }
}
