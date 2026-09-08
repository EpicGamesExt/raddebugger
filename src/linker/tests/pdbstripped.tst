test:
{

  artifacts:
  {
    debug_obj:
    {
      file_name: "debug.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            debug_s:
            {
              name: ".debug$S"
              permissions: (read)
              content: initialized_data
              raw_flags: 0x02000000
              data:
              {
                concat:
                {
                  u32le: 4
                  u32le: 0x000000f1
                  u32le: 172

                  u16le: 16
                  u16le: 0x1101
                  u32le: 0x123
                  text: "debug.obj"
                  zero: 1

                  u16le: 49
                  u16le: 0x1147
                  zero: 35
                  text: "global_proc"
                  zero: 1

                  u16le: 2
                  u16le: 0x114f

                  u16le: 21
                  u16le: 0x1108
                  u32le: 0
                  text: "global_typedef"
                  zero: 1

                  u16le: 48
                  u16le: 0x1146
                  zero: 35
                  text: "local_proc"
                  zero: 1

                  u16le: 20
                  u16le: 0x1108
                  u32le: 0
                  text: "local_typedef"
                  zero: 1

                  u16le: 2
                  u16le: 0x114f
                }
              }
            }
          }
        }
      }
    }

    pub_obj:
    {
      file_name: "pub.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            text: { name: ".text", permissions: (read, execute), content: code, data: { text: "FOOBAR" } }
            data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "QWE" } }
          }
          symbols:
          {
            global_func: { kind: external_function, name: "global_func", section: text, value: 1 }
            global_var: { kind: external, name: "global_var", section: data, value: 1 }
            static_var: { kind: static, name: "static_var", section: data, value: 1 }
          }
        }
      }
    }

    entry_obj:
    {
      file_name: "entry.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
          }
          symbols:
          {
            entry: { kind: external, name: "entry", section: text, value: 0 }
          }
        }
      }
    }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /debug:full /out:a.exe /pdbstripped:a.stripped.pdb entry.obj pub.obj debug.obj"
    }
  }

  steps:
  {
    expect_pdb:
    {
      path: "a.stripped.pdb"
      expected:
      {
        pdb:
        {
          dbi:
          {
            @range(1, 0xffffffffffffffff) module_count

            // modules must contain only stubs for static procs
            @all("*.c11_size", 0)
            @all("*.c13_size", 0)
            @all("*.module_symbols.symbols.*.kind", S_LPROC32, S_END)
            modules:
            {
              module_0: { object_file_name: "entry.obj", symbol_size: 0 }
              module_1: { object_file_name: "pub.obj", symbol_size: 0 }
              module_2: { object_file_name: "debug.obj", @range(1, 0xffffffffffffffff) symbol_size }
            }
          }

          // global symbol stream must have public and references to the static stubs
          @all("symbols.*.kind", S_PUB32, S_LPROCREF)
          global_symbols: {}

          // types must be stripped
          tpi: { header_only: true }
          ipi: { header_only: true }
        }
      }
    }
  }
}
