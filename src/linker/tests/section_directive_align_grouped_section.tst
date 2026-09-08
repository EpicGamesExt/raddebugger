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
        sections: { prot_mem: { name: "prot$mem", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "mem" } } }
        directives: { directive: "/SECTION:prot,R,ALIGN=8192" }
      } }
    }
    image: { file_name: "grouped_section_align.exe", pe: {} }
  }
  build:
  {
    // Apply read-only section flags and 8192-byte alignment to the grouped prot output section.
    link: { args: "/subsystem:console /entry:entry /out:grouped_section_align.exe entry.obj prot.obj", artifact: image }
  }
  steps:
  {
    expect_pe: { artifact: image, expected: { pe: { sections: { "prot": { alignment: 8192, @bits_set(1088421952) raw_flags: 1088421952 } } } } }
  }
}
