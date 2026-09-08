test:
{
  artifacts:
  {
    // main.obj
    main_obj:
    {
      file_name: "main.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data:
            {
              concat:
              {
                hex: "48b80000000000000000" // mov rax, func_name
                hex: "ffd0"                 // call rax
                hex: "4831c0"               // xor rax, rax
                hex: "b800000000"           // mov eax, func_name
                hex: "ffd0"                 // call rax
                hex: "c3"                   // ret
              }
            }
            relocations:
            {
              foo_64: { type: Addr64, offset: 2, symbol: foo }
              foo_32: { type: Addr32, offset: 16, symbol: foo }
            }
          }
          data:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            data: { zero: 4 }
            relocations: { absolute: { type: Addr32, offset: 0, symbol: abs } }
          }
        }
        symbols:
        {
          foo: { kind: undefined, name: "foo" }
          abs: { kind: absolute, name: "abs", value: 305419896, storage: static }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }

    // func.obj
    func_obj:
    {
      file_name: "func.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { foo: { kind: external, name: "foo", section: text, value: 0 } }
      } }
    }
  }

  build:
  {
    // linker must not produce base relocations for absolute symbol
    link: { args: "/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe main.obj func.obj", output: none }

    // it is illegal to merge .reloc with other sections
    link: { args: "/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.reloc=.rdata main.obj func.obj", output: none, expect_exit: 26 }

    // the other way around is illegal too
    link: { args: "/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.rdata=.reloc main.obj func.obj", output: none, expect_exit: 26 }
  }
  steps: {}
}
