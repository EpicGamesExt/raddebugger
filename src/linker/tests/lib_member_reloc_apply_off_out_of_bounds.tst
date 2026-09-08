test:
{
  artifacts:
  {
    bad_lib:
    {
      file_name: "bad.lib"
      coff: { library:
      {
        second_linker_member: true
        members: { malformed:
        {
          path: "bad_member.obj"
          object:
          {
            machine: x64
            sections: { data:
            {
              name: ".data"
              permissions: (read, write)
              content: initialized_data
              alignment: 1
              data: { hex: "00000000" }
              relocations: { target: { type: Addr32, offset: 4294967295, symbol: target } }
            } }
            symbols:
            {
              target: { kind: absolute, name: "target", value: 0, storage: static }
              bad: { kind: external, name: "bad", section: data, value: 0 }
            }
          }
        } }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
  }
  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:archive_relocation_offset_out_of_bounds.exe /include:bad entry.obj bad.lib"
      expect_exit: nonzero
    }
  }
  steps: {}
}
