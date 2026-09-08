test:
{
  artifacts:
  {
    weak_obj:
    {
      file_name: "weak.obj"
      coff: { object: { machine: x64, sections: { data: {
        name: ".data", permissions: (read, write), content: initialized_data, data: { hex: "deadbeef" }
      } }, symbols: {
        ptr: { kind: undefined, name: "ptr" }
        foo: { kind: weak, name: "foo", fallback: ptr, search: alias }
      } } }
    }
    ptr_obj: { file_name: "ptr.obj", coff: { object: { machine: x64, symbols: {
      entry: { kind: undefined, name: "entry" }
      ptr: { kind: weak, name: "ptr", fallback: entry, search: alias }
    } } } }
    undef_obj:
    {
      file_name: "undef.obj"
      coff: { object: { machine: x64, sections: { data: {
        name: ".data", permissions: (read, write), content: initialized_data, data: { hex: "00000000" }
        relocations: { foo_ref: { type: Addr32Nb, offset: 0, symbol: foo } }
      } }, symbols: { foo: { kind: undefined, name: "foo" } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" }
      } }, symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } } } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe weak.obj entry.obj ptr.obj undef.obj" } }
  steps: {}
}
