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
              raw_flags: 33554432
              data: { hex: "0400000012000515000080000000000000000000000000000a00061500008000000000000e000715000080000000000000000000" }
            }
            debug_h:
            {
              name: ".debug$H"
              permissions: (read)
              content: initialized_data
              raw_flags: 33554432
              data: { hex: "c5c9330100000200010000000000000002000000000000000300000000000000" }
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
              raw_flags: 33554432
              data: { hex: "0400000012000515000080000000000000000000000000000a00061500008000000000000e000715000080000000000000000000" }
            }
            debug_h:
            {
              name: ".debug$H"
              permissions: (read)
              content: initialized_data
              raw_flags: 33554432
              data: { hex: "c5c9330100000200040000000000000005000000000000000600000000000000" }
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
      args: "/subsystem:console /entry:entry /out:ghash.exe /debug:ghash entry.obj a.obj b.obj"
    }
    link:
    {
      args: "/subsystem:console /entry:entry /out:full.exe /debug:full entry.obj a.obj b.obj"
    }
  }

  steps:
  {
    expect_pdb:
    {
      path: "ghash.pdb"
      expected: { pdb: { tpi: { leaf_count: 6 } } }
    }
    expect_pdb:
    {
      path: "full.pdb"
      expected: { pdb: { tpi: { leaf_count: 3 } } }
    }
  }
}
