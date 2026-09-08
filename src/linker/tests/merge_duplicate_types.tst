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

    pch_obj:
    {
      file_name: "pch.obj"
      coff:
      {
        object:
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
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  // signature
                  hex: "04000000"
                  // duplicate in a.obj
                  hex: "0a0002100300000000000000"
                  // unique procedure type
                  hex: "0e000810001000000200000000000000"
                  // PCH ender
                  hex: "06001400bebafeca"
                }
              }
            }
            debug_s:
            {
              name: ".debug$S"
              permissions: (read)
              content: initialized_data
              alignment: 1
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000f1000000320000000e000111bebafeca7063682e6f626a00"
                  // ver_fe_major
                  // ver_fe_minor
                  // ver_fe_build
                  // ver_feqfe
                  // ver_major
                  // ver_minor
                  // ver_build
                  // ver_qfe
                  hex: "20003c1100000000d00000000000000000000e002400197f0000544f5254555245000000"
                }
              }
            }
          }
        }
      }
    }

    a_obj:
    {
      file_name: "a.obj"
      coff:
      {
        object:
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
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000160009150010000002000000bebafeca7063682e6f626a00"
                  hex: "0a0002100300000000000000"
                  hex: "0e000810001000000000000000000000"
                }
              }
            }
            debug_s:
            {
              name: ".debug$S"
              permissions: (read)
              content: initialized_data
              alignment: 1
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000f1000000300000000c000111bebafeca612e6f626a00"
                  // ver_fe_major
                  // ver_fe_minor
                  // ver_fe_build
                  // ver_feqfe
                  // ver_major
                  // ver_minor
                  // ver_build
                  // ver_qfe
                  hex: "20003c1100000000d00000000000000000000e002400197f0000544f525455524500"
                }
              }
            }
          }
        }
      }
    }

    b_obj:
    {
      file_name: "b.obj"
      coff:
      {
        object:
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
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000160009150010000002000000bebafeca7063682e6f626a00"
                  hex: "0a0002100300000000000000"
                  hex: "0e000810001000000000000000000000"
                }
              }
            }
          }
        }
      }
    }

    c_obj:
    {
      file_name: "c.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              data: { zero: 0 }
            }
            debug_t:
            {
              name: ".debug$T"
              permissions: (read)
              content: initialized_data
              alignment: 1
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000"
                  hex: "0a0002101100000000000000"
                  hex: "0e000810001000000000000000000000"
                }
              }
            }
            debug_s:
            {
              name: ".debug$S"
              permissions: (read)
              content: initialized_data
              alignment: 1
              raw_flags: 33554432
              data:
              {
                concat:
                {
                  hex: "04000000f10000005f000000"
                  // S_OBJNAME
                  hex: "0c000111bebafeca632e6f626a00"
                  // S_COMPILE3
                  // ver_fe_major
                  // ver_fe_minor
                  // ver_fe_build
                  // ver_feqfe
                  // ver_major
                  // ver_minor
                  // ver_build
                  // ver_qfe
                  hex: "20003c1100000000d00000000000000000000e002400197f0000544f525455524500"
                  // S_LPROC32
                  hex: "29000f110000000000000000000000000100000000000000000000000110000000000000010000666f6f00"
                  // S_PROC_ID_END
                  hex: "02000600"
                  // $$Symbols header
                  hex: "00"
                }
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
      args: "/subsystem:console /entry:entry /debug:full /out:a.exe pch.obj a.obj b.obj c.obj entry.obj"
    }
  }

  steps:
  {
    // load msf
    // find named streams
    // find string table
    // find TPI
    expect_pdb:
    {
      path: "a.pdb"
      expected:
      {
        pdb:
        {
          tpi:
          {
            leaf_count: 5
            @count(5) leaves:
            {
              leaf_0: { kind: LF_POINTER, data_size: 8, type: 3, attributes: 0 }
              leaf_1: { kind: LF_PROCEDURE, data_size: 12, return_type: 4096, call_kind: 2 }
              leaf_2: { kind: LF_PROCEDURE, data_size: 12, return_type: 4096, call_kind: 0 }
              leaf_3: { kind: LF_POINTER, data_size: 8, type: 17, attributes: 0 }
              leaf_4: { kind: LF_PROCEDURE, data_size: 12, return_type: 4099, call_kind: 0 }
            }
          }
        }
      }
    }
  }
}
