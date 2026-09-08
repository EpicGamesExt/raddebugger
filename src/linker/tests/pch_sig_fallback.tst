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
                  u32le: 4

                  // duplicate in a.obj
                  u16le: 10
                  u16le: 0x1002
                  u32le: 3
                  u32le: 0

                  // unique procedure type
                  u16le: 14
                  u16le: 0x1008
                  u32le: 0x1000
                  hex: "02 00"
                  u16le: 0
                  u32le: 0

                  // PCH ender
                  u16le: 6
                  u16le: 0x0014
                  u32le: 0xcafebabe
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
                concat:
                {
                  u32le: 4
                  align4:
                  {
                    concat:
                    {
                      u32le: 0x000000f1
                      size32le:
                      {
                        concat:
                        {
                          size16le:
                          {
                            concat:
                            {
                              u16le: 0x1101
                              u32le: 0xcafebabe
                              work_path: "a.obj"
                              zero: 1
                            }
                          }

                          size16le:
                          {
                            concat:
                            {
                              u16le: 0x113c
                              u32le: 0
                              u16le: 0x00d0
                              u16le: 0
                              u16le: 0
                              u16le: 0
                              u16le: 0
                              u16le: 14
                              u16le: 36
                              u16le: 32537
                              u16le: 0
                              text: "TORTURE"
                              zero: 1
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
                  u32le: 4

                  u16le: 40
                  u16le: 0x1509
                  u32le: 0x1000
                  u32le: 2
                  u32le: 0xcafebabe
                  text: "corrupt-pch-file-path.obj"
                  zero: 1

                  u16le: 10
                  u16le: 0x1002
                  u32le: 3
                  u32le: 0

                  u16le: 14
                  u16le: 0x1008
                  u32le: 0x1000
                  hex: "00 00"
                  u16le: 0
                  u32le: 0
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
                concat:
                {
                  u32le: 4
                  align4:
                  {
                    concat:
                    {
                      u32le: 0x000000f1
                      size32le:
                      {
                        concat:
                        {
                          size16le:
                          {
                            concat:
                            {
                              u16le: 0x1101
                              u32le: 0xcafebabe
                              work_path: "a.obj"
                              zero: 1
                            }
                          }

                          size16le:
                          {
                            concat:
                            {
                              u16le: 0x113c
                              u32le: 0
                              u16le: 0x00d0
                              u16le: 0
                              u16le: 0
                              u16le: 0
                              u16le: 0
                              u16le: 14
                              u16le: 36
                              u16le: 32537
                              u16le: 0
                              text: "TORTURE"
                              zero: 1
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
      args: "/subsystem:console /entry:entry /out:a.exe /debug:full a.obj b.obj entry.obj"
    }
  }

  steps: {}
}
