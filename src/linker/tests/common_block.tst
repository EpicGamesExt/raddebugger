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
            data:
            {
              name: ".data"
              permissions: (read, write)
              content: initialized_data
              alignment: 1
              data: { zero: 6 }
              relocations: { a_ref: { type: Addr32, offset: 0, symbol: a } }
            }
            // shift common block's initial position
            bss:
            {
              name: ".bss"
              permissions: (read, write)
              content: uninitialized_data
              data: { zero: 1 }
            }
          }
          symbols: { a: { kind: common, name: "A", size: 3 } }
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
            data:
            {
              name: ".data"
              permissions: (read, write)
              content: initialized_data
              alignment: 1
              data: { zero: 9 }
              relocations: { b_ref: { type: Addr64, offset: 0, symbol: b } }
            }
          }
          symbols: { b: { kind: common, name: "B", size: 6 } }
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
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              alignment: 1
              data: { hex: "c3" }
            }
          }
          symbols: { entry: { kind: external, name: "my_entry", section: text, value: 0 } }
        }
      }
    }
    image: { file_name: "common_block.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:my_entry /out:common_block.exe /fixed /largeaddressaware:no /merge:.bss=.comm a.obj b.obj entry.obj"
      artifact: image
    }
  }

  steps:
  {
    expect_pe:
    {
      artifact: image
      expected:
      {
        pe:
        {
          sections:
          {
            // blocks must be sorted in descending order to reduce alignment padding
            ".comm": { virtual_size: 19 }
            ".data": {}
          }
        }
      }
    }
    // ensure linker correctly patched addresses for symbols pointing into common block
    expect_pe_word: { artifact: image, section: ".data", offset: 0, type: u32, target_section: ".comm", target_offset: 16, target_address: va }
    expect_pe_word: { artifact: image, section: ".data", offset: 6, type: u64, target_section: ".comm", target_offset: 8, target_address: va }
  }
}
