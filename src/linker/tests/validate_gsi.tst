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
                  u32le: 102

                  // S_CONSTANT: type 0, immediate value 263, "CV_SymKind_BLOCK16".
                  u16le: 27
                  u16le: 0x1107
                  u32le: 0
                  u16le: 263
                  text: "CV_SymKind_BLOCK16"
                  zero: 1

                  // S_GDATA32: type 0, section 2 + 0x25440, "__newclmap".
                  u16le: 23
                  u16le: 0x110d
                  u32le: 0
                  u32le: 0x25440
                  u16le: 2
                  text: "__newclmap"
                  zero: 1

                  // S_GDATA32: type 0, section 1 + 123, "coffeebabe".
                  u16le: 23
                  u16le: 0x110d
                  u32le: 0
                  u32le: 123
                  u16le: 1
                  text: "coffeebabe"
                  zero: 1

                  // S_GDATA32: type 0, section 1 + 123, "deadbeef".
                  u16le: 21
                  u16le: 0x110d
                  u32le: 0
                  u32le: 123
                  u16le: 1
                  text: "deadbeef"
                  zero: 1

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
                hex: "04000000 f1000000 66000000 1b000711 00000000 0701 43565f53796d4b696e645f424c4f434b3136 00 17000d11 00000000 40540200 0200 5f5f6e6577636c6d6170 00 17000d11 00000000 7b000000 0100 636f6666656562616265 00 15000d11 00000000 7b000000 0100 6465616462656566 00 0000"
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
            @count(4) symbols:
            {
              "CV_SymKind_BLOCK16": { kind: S_CONSTANT }
              "__newclmap": { kind: S_GDATA32 }
              coffeebabe: { kind: S_GDATA32 }
              deadbeef: { kind: S_GDATA32 }
            }
            indexed_symbol_count: 4
            kind_counts: { S_CONSTANT: 1, S_GDATA32: 3 }
          }
        }
      }
    }
  }
}
