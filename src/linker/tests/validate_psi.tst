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

    test_obj:
    {
      file_name: "test.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            // Executable bytes containing the externally visible function.
            text: { name: ".text", permissions: (read, execute), content: code, data: { text: "FOOBAR" } }
            // Writable bytes containing the global and static variables.
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
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full entry.obj test.obj"
    }
  }

  steps:
  {
    expect_pdb:
    {
      path: "a.pdb"
      expected:
      {
        pdb:
        {
          psi:
          {
            symbols:
            {
              global_func: { kind: S_PUB32, flags: 2, @range(1, 65535) section }
              global_var: { kind: S_PUB32, flags: 0, @range(1, 65535) section }
              @absent static_var
            }
            // Every indexed PSI record must be PUB32, and at least one must exist.
            @count(1) kind_counts: { @range(1, 4294967295) S_PUB32 }
            @range(1, 4294967295) indexed_symbol_count
          }
        }
      }
    }
  }
}
