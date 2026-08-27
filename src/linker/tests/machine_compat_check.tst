test:
{

  artifacts:
  {
    unknown_obj:
    {
      file_name: "unknown.obj"
      coff:
      {
        object:
        {
          machine: unknown
          sections:
          {
            data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "unknown" } }
          }
        }
      }
    }
    x64_obj:
    {
      file_name: "x64.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "x64" } }
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
            text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } }
          }
          symbols:
          {
            my_entry: { kind: external, name: "my_entry", section: text, value: 0 }
          }
        }
      }
    }
    arm64_obj:
    {
      file_name: "arm64.obj"
      coff:
      {
        object:
        {
          machine: arm64
          sections:
          {
            data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "arm64" } }
          }
        }
      }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj"
      artifact: image
    }
    // test objs with conflicting machines
    link:
    {
      args: "/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj arm64.obj"
      expect_exit: nonzero
    }
    // check /MACHINE switch
    link:
    {
      args: "/subsystem:console /entry:my_entry /out:a.exe /machine:amd64 arm64.obj entry.obj"
      expect_exit: nonzero
    }
  }

  steps:
  {
    expect_pe:
    {
      artifact: image
      expected: { pe: { arch: x64 } }
    }
  }
}
