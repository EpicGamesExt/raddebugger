test:
{
  artifacts:
  {
    funcs_obj:
    {
      file_name: "funcs.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          a: { name: ".text", permissions: (read, execute), content: code, alignment: 4, data: { hex: "c3" } }
          b: { name: ".text", permissions: (read, execute), content: code, alignment: 4, data: { hex: "c3" } }
          c: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols:
        {
          a: { kind: external_function, name: "A", section: a, value: 0 }
          b: { kind: external_function, name: "B", section: b, value: 0 }
          c: { kind: external_function, name: "C", section: c, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:A /functionpadmin:1 /out:a.exe funcs.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".text": { data: ccccccccc3ccccccc3ccc3 } } } } } }
}
