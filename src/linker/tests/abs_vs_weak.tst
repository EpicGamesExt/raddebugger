test:
{
  artifacts:
  {
    abs_obj: { file_name: "abs.obj", coff: { object: { machine: x64, symbols: { foo: { kind: absolute, name: "foo", value: 291, storage: external } } } } }
    text_obj:
    {
      file_name: "text.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          mydata: { name: ".mydata", permissions: (read, execute), content: code, alignment: 1, data: { text: "mydata" } }
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { hex: "48b80000000000000000c3" }
            relocations: { foo_ref: { type: Addr64, offset: 2, symbol: foo } }
          }
        }
        symbols:
        {
          mydata: { kind: external, name: "mydata", section: mydata, value: 0 }
          foo: { kind: weak, name: "foo", fallback: mydata, search: no_library }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "text_first.exe", pe: {} }
  }
  build:
  {
    link: { args: "/subsystem:console /entry:my_entry /out:abs_first.exe abs.obj text.obj" }
    link: { args: "/subsystem:console /entry:my_entry /out:text_first.exe text.obj abs.obj", artifact: image }
  }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { ".text": { data: 48b82301000000000000c3 } } } } }
  }
}
