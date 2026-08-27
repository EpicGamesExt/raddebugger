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
          text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } }
          debug_info: { name: ".debug_info", permissions: (read, write), content: initialized_data, data: { text: "DEBUG_INFO" } }
          debug_abbrev: { name: ".debug_abbrev", permissions: (read, write), content: initialized_data, data: { text: "DEBUG_ABBREV" } }
        }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build:
  {
    // link test.obj
    link: { args: "/subsystem:console /entry:entry /out:a.exe test.obj", artifact: image }
  }
  steps:
  {
    // load linked exe
    expect_pe: { artifact: image, expected: { pe: { sections: { ".debug_info": {}, ".debug_abbrev": {} } } } }
  }
}
