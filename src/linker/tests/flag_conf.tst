test:
{
  artifacts:
  {
    conf_obj:
    {
      file_name: "conf.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          a: { name: ".mysect", permissions: (read, execute), content: initialized_data, data: { text: "one" } }
          b: { name: ".mysect", permissions: (read, write), content: initialized_data, data: { text: "two" } }
        }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe conf.obj entry.obj", artifact: image } }
  steps:
  {
    expect_pe:
    {
      artifact: image
      expected:
      {
        pe:
        {
          section_count: 3
          @count(3) sections_by_index:
          {
            section_1: { name: ".text" }
            section_2: { name: ".mysect", raw_flags: 1610612800 }
            section_3: { name: ".mysect", raw_flags: 3221225536 }
          }
        }
      }
    }
  }
}
