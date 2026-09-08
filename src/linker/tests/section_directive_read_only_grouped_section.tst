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
    prot_obj:
    {
      file_name: "prot.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          prot_a: { name: "prot$a", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "A" } }
          prot_mem: { name: "prot$mem", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "mem" } }
          prot_z: { name: "prot$z", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "Z" } }
        }
        directives: { directive: "/SECTION:prot,R" }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:entry /out:a.exe entry.obj prot.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { "prot": { @bits_set(1073741888) @bits_clear(2684354560) raw_flags: 1073741888 } } } } } }
}
