test:
{
  artifacts:
  {
    cycle_obj:
    {
      file_name: "cycle.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          debug_t:
          {
            name: ".debug$T"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { hex: "040000000a00021001100000000000000a0002100010000000000000" }
          }
          debug_s:
          {
            name: ".debug$S"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { hex: "04000000f10000002b000000290010110000000000000000000000000000000000000000000000000110000000000000000000666f6f0000" }
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
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full /rad_ignore:-43 cycle.obj entry.obj"
      output: none
      stderr_matches: "*Error(*): *: LF_POINTER(type_index: *) forward refs member type index * (leaf struct offset: *)*"
    }
  }
  steps: {}
}
