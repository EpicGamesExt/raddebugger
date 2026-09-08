test:
{
  artifacts:
  {
    weak_tag_obj:
    {
      file_name: "weak_tag.obj"
      coff: { object: { machine: x64, sections: { data: {
        name: ".data", permissions: (read, write), content: initialized_data, data: { hex: "00000000" }
        relocations: { strong_second_ref: { type: Addr32, offset: 0, symbol: strong_second } }
      } }, symbols: {
        abs: { kind: absolute, name: "abs", value: 305419896, storage: static }
        strong_first: { kind: weak, name: "strong_first", fallback: abs, search: alias }
        strong_second: { kind: weak, name: "strong_second", fallback: strong_first, search: alias }
      } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" }
      } }, symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } } } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe weak_tag.obj entry.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".data": { virtual_size: 4, data: 78563412 } } } } } }
}
