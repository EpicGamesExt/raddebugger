test:
{

  artifacts:
  {
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
            debug_p:
            {
              name: ".debug$P"
              permissions: (read)
              content: initialized_data
              alignment: 1
              raw_flags: 0x02000000
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
              raw_flags: 0x02000000
              data:
              {
                align4:
                {
                  concat:
                  {
                    // C13 signature and symbols subsection kind.
                    hex: "04000000f1000000"
                    size32le:
                    {
                      concat:
                      {
                        // S_OBJNAME, signature 0xcafebabe, and the terminated a.obj work path.
                        size16le:
                        {
                          concat:
                          {
                            hex: "0111bebafeca"
                            work_path: "a.obj"
                            zero: 1
                          }
                        }

                        // S_COMPILE3: C/x64, front-end 0.0.0.0, compiler 14.36.32537.0, TORTURE.
                        size16le: { hex: "3c1100000000d00000000000000000000e002400197f0000544f525455524500" }
                      }
                    }
                  }
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
              raw_flags: 0x02000000
              data:
              {
                concat:
                {
                  // signature
                  hex: "04000000"

                  // PCH starter
                  hex: "280009150010000002000000bebafeca"
                  text: "corrupt-pch-file-path.obj"
                  zero: 1

                  // duplicate pointer type
                  hex: "0a0002100300000000000000"

                  // unique procedure type
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
              raw_flags: 0x02000000
              data:
              {
                align4:
                {
                  concat:
                  {
                    // C13 signature and symbols subsection kind.
                    hex: "04000000f1000000"
                    size32le:
                    {
                      concat:
                      {
                        // Preserve the original use of a.obj in b.obj's S_OBJNAME record.
                        size16le:
                        {
                          concat:
                          {
                            hex: "0111bebafeca"
                            work_path: "a.obj"
                            zero: 1
                          }
                        }

                        // S_COMPILE3: C/x64, front-end 0.0.0.0, compiler 14.36.32537.0, TORTURE.
                        size16le: { hex: "3c1100000000d00000000000000000000e002400197f0000544f525455524500" }
                      }
                    }
                  }
                }
              }
            }
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
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              alignment: 1
              data: { hex: "c3" }
            }
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
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full /rad_type_server:foo.rrt a.obj b.obj entry.obj"
    }
    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /debug:ghash a.obj foo.rrt b.obj entry.obj"
    }
  }

  steps: {}
}
