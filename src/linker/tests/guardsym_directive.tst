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
    guardsym_obj:
    {
      file_name: "guardsym.obj"
      // MSVC link accepts GUARDSYM without treating the named symbol as /INCLUDE.
      coff: { object: { machine: x64, directives: { directive: "/GUARDSYM:missing,S" } } }
    }
  }

  build: { link: { args: "entry.obj guardsym.obj /entry:entry /subsystem:console /out:guardsym.exe", output: none } }
  steps: {}
}
