test:
{
  artifacts:
  {
    sect_obj:
    {
      file_name: "sect.obj"
      coff: { object:
      {
        machine: x64
        sections: { mysect1: { name: ".mysect$1", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "hello, world" } } }
        directives: { directive: "/merge:.mysect=.data" }
      } }
    }
    main_obj:
    {
      file_name: "main.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          data:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            data: { zero: 8 }
            relocations: { mysect_address: { type: Addr64, offset: 0, symbol: mysect_group } }
          }
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols:
        {
          mysect_group: { kind: undefined_section, name: ".mysect$2222", value: 3221225536 }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe main.obj sect.obj", artifact: image } }
  steps:
  {
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u64, target_section: ".data", target_offset: 8 }
    expect_pe_bytes: { artifact: image, section: ".data", offset: 8, hex: "68656c6c6f2c20776f726c64" }
  }
}
