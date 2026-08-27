test:
{

  artifacts:
  {
    ab:
    {
      file_name: "ab.obj"
      coff: { object: { machine: x64, symbols: {
        B: { kind: undefined, name: "B" }
        A: { kind: weak, name: "A", fallback: B, search: alias }
      } } }
    }
    ba:
    {
      file_name: "ba.obj"
      coff: { object: { machine: x64, symbols: {
        A: { kind: undefined, name: "A" }
        B: { kind: weak, name: "B", fallback: A, search: alias }
      } } }
    }
    entry:
    {
      file_name: "entry.obj"
      coff: { object: { machine: x64, sections: { text: {
        name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" }
      } }, symbols: {
        my_entry: { kind: external, name: "my_entry", section: text, value: 0 }
      } } }
    }
  }

  build:
  {
    // give a generous 3 seconds
    link:
    {
      args: "/subsystem:console /entry:my_entry entry.obj ab.obj ba.obj"
      timeout_ms: 3000
      expect_exit: any
    }
  }
  steps: {}
}
