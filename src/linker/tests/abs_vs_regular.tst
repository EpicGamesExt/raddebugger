test:
{
  artifacts:
  {
    regular_obj:
    {
      file_name: "regular.obj"
      coff: { object: { machine: x64, sections: { data: { name: ".data", permissions: (read, write), content: initialized_data, data: { hex: "c0ffee" } } }, symbols: { foo: { kind: external, name: "foo", section: data, value: 0 } } } }
    }
    abs_obj: { file_name: "abs.obj", coff: { object: { machine: x64, symbols: { foo: { kind: absolute, name: "foo", value: 4660, storage: external } } } } }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text:
        {
          name: ".text"
          permissions: (read, execute)
          content: code
          alignment: 1
          // mov rax, $imm
          // ret
          data: { hex: "48c7c000000000c3" }
          relocations: { foo_ref: { type: Addr32Nb, offset: 3, symbol: foo } }
        } }
        symbols:
        {
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
          foo: { kind: undefined, name: "foo" }
        }
      } }
    }
  }
  build:
  {
    // TODO: validate that linker issues multiply defined symbol error
    link: { args: "/subsystem:console /entry:my_entry /out:abs_first.exe abs.obj regular.obj entry.obj", expect_exit: nonzero }
    // linker should complain about multiply defined symbol
    link: { args: "/subsystem:console /entry:my_entry /out:regular_first.exe regular.obj abs.obj entry.obj", expect_exit: nonzero }
    // linker should complain even in case regular is before abs
  }
  steps: {}
}
