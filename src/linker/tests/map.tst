test:
{

  artifacts:
  {
    map_input:
    {
      file_name: "map.obj"
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
              data: { hex: "c3" }
            }
          }
          symbols:
          {
            entry: { kind: external, name: "map_entry", section: text, value: 0 }
            local: { kind: static, name: "map_local", section: text, value: 0 }
          }
        }
      }
    }
  }

  build:
  {
    link:
    {
      args: "/entry:map_entry /subsystem:console /out:map_test_explicit.exe /map:map_test_explicit.map map.obj"
    }

    link:
    {
      args: "/entry:map_entry /subsystem:console /out:map_test_default.exe /map map.obj"
    }

    link:
    {
      args: "/entry:map_entry /subsystem:console /out:map_test_collision.exe /map:map_test_collision.exe map.obj"
      expect_exit: nonzero
    }
  }

  steps:
  {
    expect_file: { path: "map_test_explicit.map", contains: " Timestamp is " }
    expect_file: { path: "map_test_explicit.map", contains: " Preferred load address is " }
    expect_file: { path: "map_test_explicit.map", contains: " Start         Length     Name                   Class" }
    expect_file: { path: "map_test_explicit.map", contains: " Publics by Value" }
    expect_file: { path: "map_test_explicit.map", contains: "map_entry" }
    expect_file: { path: "map_test_explicit.map", contains: " entry point at" }
    expect_file: { path: "map_test_explicit.map", contains: " Static symbols" }
    expect_file: { path: "map_test_explicit.map", contains: "map_local" }
    expect_file: { path: "map_test_default.map", nonempty: true }
  }
}
