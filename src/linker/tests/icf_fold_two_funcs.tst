test:
{
  artifacts:
  {
    object:
    {
      file_name: "ident_funcs.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          a:
          {
            name: ".text$mn"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            // xor rax, rax
            // ret
            data: { hex: "4831c0c3" }
          }
          b:
          {
            name: ".text$mb"
            permissions: (read, execute)
            content: code
            flags: (link_comdat)
            // xor rax, rax
            // ret
            data: { hex: "4831c0c3" }
          }
          entry:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // call a
            // call b
            // ret
            data: { hex: "e800000000e800000000c3" }
            relocations:
            {
              call_a: { type: Rel32, offset: 1, symbol: a_symbol }
              call_b: { type: Rel32, offset: 6, symbol: b_symbol }
            }
          }
        }
        symbols:
        {
          a_definition: { kind: section_definition, section: a, selection: NoDuplicates }
          b_definition: { kind: section_definition, section: b, selection: NoDuplicates }
          a_symbol: { kind: external_function, name: "a", section: a, value: 0 }
          b_symbol: { kind: external_function, name: "b", section: b, value: 0 }
          entry_symbol: { kind: external_function, name: "entry", section: entry, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe /opt:icf ident_funcs.obj", artifact: image } }
  steps:
  {
    expect_pe:
    {
      artifact: image
      expected: { pe: { sections: { ".text":
      {
        // validate .text header
        virtual_offset: 4096
        @range(20, 18446744073709551615) virtual_size
        file_size: 512
        // entry
        // pad
        // a and b folded
        @starts_with(e80b000000e806000000c3cccccccccc4831c0c3) data
      } } } }
    }
  }
}
