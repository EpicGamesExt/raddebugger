test:
{
  artifacts:
  {
    image_base_obj:
    {
      file_name: "image_base.obj"
      coff: { object:
      {
        machine: x64
        sections: { text:
        {
          name: ".text"
          permissions: (read, execute)
          content: code
          data:
          {
            concat:
            {
              hex: "488d0d00000000"     // lea rcx, [__ImageBase]
              hex: "48b80000000000000000" // mov rax, __ImageBase
              hex: "b800000000"         // mov eax, __ImageBase
              hex: "c3"                 // ret
            }
          }
          relocations:
          {
            relative: { type: Rel32, offset: 3, symbol: image_base }
            absolute_64: { type: Addr64, offset: 9, symbol: image_base }
            image_relative_32: { type: Addr32Nb, offset: 18, symbol: image_base }
          }
        } }
        symbols:
        {
          image_base: { kind: undefined, name: "__ImageBase" }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:my_entry /base:0x2000000140000000 /out:a.exe image_base.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections: { ".text": { data: 488d0df9efffff48b80000004001000020b800000000c3 } } } } } }
}
