test:
{
  artifacts:
  {
    test_obj:
    {
      file_name: "test.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          shift: { name: ".a", permissions: (read, write), content: initialized_data, data: { text: "q" } }
          none: { name: ".a", permissions: (read, write), content: initialized_data, data: { text: "abc" } }
          a1: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "wr" } }
          a2: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 2, data: { text: "e" } }
          a4: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 4, data: { text: "ttttt" } }
          a8: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 8, data: { text: "g" } }
          a16: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 16, data: { text: "o" } }
          a32: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 32, data: { text: "p" } }
          a64: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 64, data: { text: "f" } }
          a128: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 128, data: { text: "x" } }
          a256: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 256, data: { text: "c" } }
          a512: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 512, data: { text: "v" } }
          a1024: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 1024, data: { text: "b" } }
          a2048: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 2048, data: { text: "n" } }
          a4096: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 4096, data: { text: "m" } }
          a8192: { name: ".a", permissions: (read, write), content: initialized_data, alignment: 8192, data: { text: "z" } }
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }
  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe /align:8192 test.obj", artifact: image } }
  steps:
  {
    expect_pe_bytes: { artifact: image, section: ".a", offset: 0, hex: "71" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 16, hex: "616263" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 19, hex: "7772" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 22, hex: "65" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 24, hex: "7474747474" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 32, hex: "67" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 48, hex: "6f" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 64, hex: "70" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 128, hex: "66" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 256, hex: "78" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 512, hex: "63" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 1024, hex: "76" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 2048, hex: "62" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 4096, hex: "6e" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 8192, hex: "6d" }
    expect_pe_bytes: { artifact: image, section: ".a", offset: 16384, hex: "7a" }
  }
}
