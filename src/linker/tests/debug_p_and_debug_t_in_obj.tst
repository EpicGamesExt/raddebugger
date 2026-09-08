test:
{
  artifacts:
  {
    pch_obj:
    {
      file_name: "pch.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          debug_p:
          {
            name: ".debug$P"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { concat:
            {
              // signature
              hex: "040000000a0002100300000000000000"
              // PCH ender
              hex: "06001400bebafeca"
            } }
          }
          debug_t:
          {
            name: ".debug$T"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { concat:
            {
              hex: "04000000160009150010000001000000bebafeca"
              text: "pch.obj"
              hex: "000a00021003000000000000000e000810001000000000000000000000"
            } }
          }
          debug_s:
          {
            name: ".debug$S"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { concat:
            {
              hex: "04000000f1000000100000000e000111bebafeca"
              text: "pch.obj"
              hex: "00"
            } }
          }
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
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full pch.obj entry.obj"
      output: none
      stderr_matches: "*Warning(067): *pch.obj: multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections*"
    }
  }
  steps: {}
}
