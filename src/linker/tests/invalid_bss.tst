test:
{
  artifacts:
  {
    bss_obj:
    {
      file_name: "bss.obj"
      coff: { object:
      {
        machine: x64
        sections: { bss: { name: ".bss", permissions: (read), content: initialized_data, data: { text: "Hello, World" } } }
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
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe bss.obj entry.obj", artifact: image } }
  steps:
  {
    expect_pe:
    {
      artifact: image
      expected:
      {
        pe:
        {
          sections:
          {
            ".bss":
            {
              virtual_size: 12
              raw_flags: 1073741888
              data: 48656c6c6f2c20576f726c64
            }
          }
        }
      }
    }
  }
}
