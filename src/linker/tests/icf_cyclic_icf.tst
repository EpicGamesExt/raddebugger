test:
{
  artifacts:
  {
    object:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          a: { name: ".text", permissions: (read, execute), content: code, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call_b: { type: Rel32, offset: 1, symbol: b_symbol } } }
          b: { name: ".text", permissions: (read, execute), content: code, flags: (link_comdat), data: { hex: "e800000000c3" }, relocations: { call_a: { type: Rel32, offset: 1, symbol: a_symbol } } }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          a_symbol: { kind: external, name: "a", section: a, value: 0 }
          b_symbol: { kind: static, name: "b", section: b, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /out:a.exe /entry:a /opt:icf a.obj", artifact: image } }
  steps:
  {
    // validate output
    expect_pe: { artifact: image, expected: { pe: { sections: { ".text":
    {
      // a and b folded into a self-call
      data: e8fbffffffc3
    } } } } }
  }
}
