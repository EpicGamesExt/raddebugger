test:
{
  artifacts:
  {
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "entry", section: text, value: 0 } }
      } }
    }
    a1_obj: { file_name: "a1.obj", coff: { object: { machine: x64, directives: { directive: "/FAILIFMISMATCH:a=1" } } } }
    a2_obj: { file_name: "a2.obj", coff: { object: { machine: x64, directives: { directive: "/FAILIFMISMATCH:a=2" } } } }
    a1_copy_obj: { file_name: "a1_copy.obj", coff: { object: { machine: x64, directives: { directive: "/FAILIFMISMATCH:a=1" } } } }
    conf_dirs_obj: { file_name: "conf_dirs.obj", coff: { object: { machine: x64, directives: { directive: "/FAILIFMISMATCH:a=1 /FAILIFMISMATCH:a=2" } } } }
  }

  build:
  {
    // ------------------------------------------------------------
    // try linking two objs with mismatching directives
    link: { args: "entry.obj a1.obj a2.obj /entry:entry /subsystem:console /out:a2.exe", output: none, expect_exit: 32 }

    // ------------------------------------------------------------
    // happy case
    link: { args: "entry.obj a1.obj a1_copy.obj /entry:entry /subsystem:console /out:a1.exe", output: none }

    // ------------------------------------------------------------
    // test conflicting directives in obj
    link: { args: "entry.obj conf_dirs.obj /entry:entry /subsystem:console /out:conf_dirs.exe", output: none, expect_exit: 32 }

    // ------------------------------------------------------------
    // passing switch on command line
    link: { args: "entry.obj a1.obj /FAILIFMISMATCH:a=2 /out:cmddir.exe", output: none, expect_exit: 32 }
  }
  steps: {}
}
