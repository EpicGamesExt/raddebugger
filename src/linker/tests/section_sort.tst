test:
{
  artifacts:
  {
    data_obj:
    {
      file_name: "data.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          idata_2: { name: ".idata$2", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "last" } }
          idata_5: { name: ".idata$5", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "first" } }
          rdata: { name: ".rdata", permissions: (read), content: initialized_data, alignment: 1, data: { text: "middle" } }
          data_z: { name: ".data$z", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "five" } }
          data_a: { name: ".data$a", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "three" } }
          data_bbbbb: { name: ".data$bbbbb", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "four" } }
          data_empty: { name: ".data$", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "two" } }
          data: { name: ".data", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "one" } }
        }
      } }
    }
    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections: { text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } } }
        symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe data.obj entry.obj", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { sections:
  {
    ".data": { data: 66697273746f6e6574776f7468726565666f7572666976656c617374 }
    ".rdata": { data: 6d6964646c65 }
  } } } } }
}
