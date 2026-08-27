test:
{
  artifacts:
  {
    a_obj:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          // ver_fe_major
          // ver_fe_minor
          // ver_fe_build
          // ver_feqfe
          // ver_major
          // ver_minor
          // ver_build
          // ver_qfe
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
              hex: "04000000"
              // duplicate in a.obj
              hex: "0a0002100300000000000000"
              // unique procedure type
              hex: "0e000810001000000200000000000000"
              // PCH ender
              hex: "06001400bebafeca"
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
              hex: "04000000f1000000300000000c000111bebafeca"
              text: "a.obj"
              hex: "0020003c1100000000d00000000000000000000e002400197f0000544f525455524500"
            } }
          }
        }
      } }
    }
    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          // ver_fe_major
          // ver_fe_minor
          // ver_fe_build
          // ver_feqfe
          // ver_major
          // ver_minor
          // ver_build
          // ver_qfe
          debug_t:
          {
            name: ".debug$T"
            permissions: (read)
            content: initialized_data
            alignment: 1
            flags: (discardable)
            data: { concat:
            {
              hex: "04000000140009150010000002000000efbeadde"
              text: "a.obj"
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
              hex: "04000000f1000000300000000c000111efbeadde"
              text: "a.obj"
              hex: "0020003c1100000000d00000000000000000000e002400197f0000544f525455524500"
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
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full a.obj b.obj entry.obj"
      output: none
      stderr_matches: "*Error(048): *b.obj: PCH signature mismatch, expected 0xdeadbeef got 0xcafebabe; PCH obj *a.obj*"
    }
  }
  steps: {}
}
