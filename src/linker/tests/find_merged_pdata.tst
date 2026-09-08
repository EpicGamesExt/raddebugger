test:
{
  artifacts:
  {
    main_obj:
    {
      file_name: "main.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          xdata: { name: ".xdata", permissions: (read), content: initialized_data, alignment: 4, data: { hex: "191b0300090140000270000000000000f00100000104010004420000" } }
          pdata:
          {
            name: ".pdata"
            permissions: (read)
            content: initialized_data
            alignment: 4
            data: { zero: 12 }
            relocations:
            {
              unwind: { type: Addr32Nb, offset: 8, symbol: unwind_foobar }
              first: { type: Addr32Nb, offset: 0, symbol: foobar }
              one_past_last: { type: Addr32Nb, offset: 4, symbol: foobar }
            }
          }
          foobar:
          {
            name: ".foobar"
            permissions: (read, execute)
            content: code
            alignment: 1
            data: { hex: "40574881ec00020000488b05000000004833c448898424f0010000488d0424488bf833c0b9ec010000f3aab804000000486bc0028b0404488b8c24f00100004833cce8000000004881c4000200005fc3cccccccccccccccccccccccccccccccc4883ec28e8000000004883c428c3" }
          }
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols:
        {
          foobar: { kind: static, name: "foobar", section: foobar, value: 0 }
          xdata_definition: { kind: section_definition, section: xdata, selection: Null }
          unwind_foobar: { kind: static, name: "$unwind$foobar", section: xdata, value: 0 }
          pdata_definition: { kind: section_definition, section: pdata, selection: Null }
          pdata_foobar: { kind: static, name: "$pdata$foobar", section: pdata, value: 0 }
          entry: { kind: external, name: "my_entry", section: text, value: 0 }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build: { link: { args: "/subsystem:console /entry:my_entry /out:a.exe main.obj /merge:.pdata=.rdata", artifact: image } }
  steps: { expect_pe: { artifact: image, expected: { pe: { data_directories: { exceptions: { file_size: 12 } } } } } }
}
