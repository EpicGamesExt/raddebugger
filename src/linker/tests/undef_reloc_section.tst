test:
{
  artifacts:
  {
    main_obj:
    {
      file_name: "main.obj"
      coff: { object: { machine: x64, sections: {
        text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } }
        data: {
          name: ".data", permissions: (read, write), content: initialized_data, data: { hex: "0000000000000000" }
          relocations: { reloc_ref: { type: Addr64, offset: 0, symbol: reloc } }
        }
      }, symbols: {
        entry: { kind: external, name: "my_entry", section: text, value: 0 }
        reloc: { kind: undefined_section, name: ".reloc", value: 1107296320 }
      } } }
    }
    sec_defn_obj:
    {
      file_name: "sec_defn.obj"
      coff: { object: { machine: x64, sections: { mysect: {
        name: ".mysect", permissions: (read), content: initialized_data, alignment: 1, data: { hex: "010203" }
      } }, symbols: { mysect_definition: { kind: section_definition, section: mysect, selection: Null } } } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj", expect_exit: 25 } }
  steps: {}
}
