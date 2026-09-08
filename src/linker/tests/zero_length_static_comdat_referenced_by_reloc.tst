test:
{
  artifacts:
  {
    test_obj:
    {
      file_name: "test.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          empty: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 1, flags: (link_comdat), data: { zero: 0 } }
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            data:
            {
              concat:
              {
                hex: "488d0500000000" // lea rax, [rip + EMPTY]
                hex: "c3"
              }
            }
            relocations: { empty_ref: { type: Rel32, offset: 3, symbol: empty } }
          }
        }
        symbols:
        {
          empty_def: { kind: section_definition, section: empty, selection: NoDuplicates }
          empty: { kind: static, name: "EMPTY", section: empty, value: 0 }
          text_def: { kind: section_definition, section: text, selection: NoDuplicates }
          entry: { kind: external, name: "entry", section: text, value: 0 }
        }
      } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:ref,noicf test.obj" } }
  steps: {}
}
