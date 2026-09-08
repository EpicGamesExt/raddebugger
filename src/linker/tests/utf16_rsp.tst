test:
{
  artifacts:
  {
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
    rsp_file:
    {
      file_name: "args.rsp"
      bytes: { data: { hex: "fffe2f00730075006200730079007300740065006d003a0063006f006e0073006f006c00650020002f0065006e007400720079003a0065006e0074007200790020002f006f00750074003a0061002e00650078006500200065006e007400720079002e006f0062006a000a00" } }
    }
  }

  build: { link: { args: "@args.rsp", output: none } }
  steps: {}
}
