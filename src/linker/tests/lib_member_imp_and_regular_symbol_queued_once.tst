test:
{
  artifacts:
  {
    rust_style_lib:
    {
      file_name: "rust_style.rlib"
      coff: { library: { second_linker_member: true, members: { rust_member: {
        path: "core-9f9efb2036858c45.core.78298229696da45f-cgu.0.rcgu.o"
        object: { machine: x64, sections: {
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          idata: { name: ".idata", permissions: (read), content: initialized_data, alignment: 8, data: { zero: 8 } }
        }, symbols: {
          foo: { kind: external_function, name: "foo", section: text, value: 0 }
          imp_foo: { kind: external, name: "__imp_foo", section: idata, value: 0 }
        } }
      } } } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1
        data: { hex: "48c7c00000000048c7c100000000c3" }
        relocations: {
          foo_ref: { type: Addr32Nb, offset: 3, symbol: foo }
          imp_foo_ref: { type: Addr32Nb, offset: 10, symbol: imp_foo }
        }
      } }, symbols: {
        entry: { kind: external, name: "entry", section: text, value: 0 }
        foo: { kind: undefined, name: "foo" }
        imp_foo: { kind: undefined, name: "__imp_foo" }
      } } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj rust_style.rlib" } }
  steps: {}
}
