test:
{
  artifacts:
  {
    a_obj:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections: { rdata: { name: ".rdata", permissions: (read), content: initialized_data, flags: (link_comdat), data: { concat: { text: "1Hello, World!", zero: 1 } } } }
        symbols:
        {
          rdata_def: { kind: section_definition, section: rdata, selection: Largest }
          test: { kind: external, name: "TEST", section: rdata, value: 1 }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections: { rdata: { name: ".rdata", permissions: (read), content: initialized_data, flags: (link_comdat), data: { concat: { text: "Hello, World!", zero: 1 } } } }
        symbols:
        {
          rdata_def: { kind: section_definition, section: rdata, selection: Largest }
          test: { kind: external, name: "TEST", section: rdata, value: 1 }
        }
      } }
    }
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
          data: { concat: { hex: "48c7c000000000" // mov rax, $imm
                            hex: "c3" // ret
          } }
          relocations: { test_ref: { type: Addr32Nb, offset: 3, symbol: test } }
        } }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          test: { kind: undefined, name: "TEST" }
        }
      } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj" } }
  steps: {}
}
