test:
{

  artifacts:
  {
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
                  // C13 signature and symbols subsection header.
                  u32le: 4
                  u32le: 0xf1
                  u32le: 190

                  // S_OBJNAME: signature 0x123, "debug.obj".
                  u16le: 16
                  u16le: 0x1101
                  u32le: 0x123
                  text: "debug.obj"
                  zero: 1

                  // S_GPROC32_ID: zeroed procedure fields, "global_proc".
                  u16le: 49
                  u16le: 0x1147
                  zero: 32
                  u16le: 0
                  hex: "00"
                  text: "global_proc"
                  zero: 1

                  // S_PROC_ID_END.
                  u16le: 2
                  u16le: 0x114f

                  // S_UDT: type 0, "global_typedef".
                  u16le: 21
                  u16le: 0x1108
                  u32le: 0
                  text: "global_typedef"
                  zero: 1

                  // S_OBJNAME: signature 0x123, "debug.obj".
                  u16le: 16
                  u16le: 0x1101
                  u32le: 0x123
                  text: "debug.obj"
                  zero: 1

                  // S_LPROC32_ID: zeroed procedure fields, "local_proc".
                  u16le: 48
                  u16le: 0x1146
                  zero: 32
                  u16le: 0
                  hex: "00"
                  text: "local_proc"
                  zero: 1

                  // S_UDT: type 0, "local_typedef".
                  u16le: 20
                  u16le: 0x1108
                  u32le: 0
                  text: "local_typedef"
                  zero: 1

                  // S_PROC_ID_END.
                  u16le: 2
                  u16le: 0x114f

                  // Align the complete C13 subsection to four bytes.
                  zero: 2
                }
              }
            }
          }
        }
      }
    }

    // Raw helper output for byte-for-byte parity with the typed CodeView records above.
    raw_debug_obj:
    {
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
                hex: "04000000 f1000000 be000000 10000111 23010000 64656275672e6f626a 00 31004711 0000000000000000000000000000000000000000000000000000000000000000 0000 00 676c6f62616c5f70726f63 00 02004f11 15000811 00000000 676c6f62616c5f74797065646566 00 10000111 23010000 64656275672e6f626a 00 30004611 0000000000000000000000000000000000000000000000000000000000000000 0000 00 6c6f63616c5f70726f63 00 14000811 00000000 6c6f63616c5f74797065646566 00 02004f11 0000"
              }
            }
          }
        }
      }
    }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /debug:full /out:a.exe entry.obj debug.obj"
    }
  }

  steps:
  {
    compare: { left: debug_obj, right: raw_debug_obj }
    expect_pdb:
    {
      path: "a.pdb"
      expected:
      {
        pdb:
        {
          gsi:
          {
            @count(3) symbols:
            {
              global_proc: { kind: S_PROCREF, suc_name: 0, sym_off: 24, imod: 2 }
              local_proc: { kind: S_LPROCREF, suc_name: 0, sym_off: 100, imod: 2 }
              global_typedef: { kind: S_UDT }
              @absent local_typedef
            }
            indexed_symbol_count: 3
            kind_counts: { S_PROCREF: 1, S_LPROCREF: 1, S_UDT: 1 }
          }
        }
      }
    }
  }
}
