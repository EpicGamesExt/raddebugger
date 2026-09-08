test:
{
  artifacts:
  {
    weak_obj:
    {
      file_name: "weak.obj"
      coff: { object: { machine: x64,
        sections: { target: { name: ".target", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { text: "target" } } }
        symbols: {
          target_def: { kind: section_definition, section: target, selection: Any }
          target: { kind: external, name: "target", section: target, value: 0 }
          weak_target: { kind: weak, name: "weak_target", fallback: target, search: alias }
        }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "48c7c000000000c3" }
        relocations: { weak_ref: { type: Addr32Nb, offset: 3, symbol: weak_target } }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        weak_target: { kind: undefined, name: "weak_target" }
      } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /opt:ref /out:a.exe entry.obj weak.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".target": {} } } } } }
}
