test:
{
  artifacts:
  {
    bad_obj:
    {
      file_name: "bad.obj"
      coff: { object:
      {
        machine: x64
        sections: { text:
        {
          name: ".text"
          permissions: (read, execute)
          content: code
          alignment: 1
          data: { hex: "00000000" }
          relocations: { target: { type: Addr32, offset: 4294967295, symbol: target } }
        } }
        symbols:
        {
          target: { kind: absolute, name: "target", value: 0, storage: static }
          entry: { kind: external, name: "entry", section: text, value: 0 }
        }
      } }
    }
  }
  build: { link: { args: "/subsystem:console /entry:entry /out:relocation_offset_out_of_bounds.exe bad.obj", expect_exit: nonzero } }
  steps: {}
}
