test:
{
  artifacts:
  {
    debug_obj:
    {
      file_name: "debug.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          debug_t: { name: ".debug$T", permissions: (read), content: initialized_data, flags: (discardable), data: { hex: "0400000012000515000080000000000000000000000000000a00061500008000000000000e000715000080000000000000000000" } }
          debug_h: { name: ".debug$H", permissions: (read), content: initialized_data, flags: (discardable), data: { zero: 0 } }
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
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj"
      output: none
      stderr_matches: "*Warning(*): *: .debug$H section is too small to contain the header*"
    }
  }
  steps: {}
}
