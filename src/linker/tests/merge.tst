test:
{
  artifacts:
  {
    test_obj:
    {
      file_name: "test.obj"
      coff: { object: { machine: x64, sections: { test: { name: ".test", permissions: (read, write), content: initialized_data, data: { text: "hello, world" } } } } }
    }
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
    mixed_obj:
    {
      file_name: "mixed.obj"
      coff: { object: { machine: x64, sections:
      {
        data: { name: ".data", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "d" } }
        bss: { name: ".bss", permissions: (read, write), content: uninitialized_data, alignment: 1, data: { zero: 513 } }
      } } }
    }
    order_obj:
    {
      file_name: "order.obj"
      coff: { object: { machine: x64, sections:
      {
        a: { name: ".a$m", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "a" } }
        z: { name: ".z$m", permissions: (read, write), content: initialized_data, alignment: 1, data: { text: "z" } }
      } } }
    }
    created_image: { file_name: "merge_created.exe", pe: {} }
    mixed_image: { file_name: "merge_mixed.exe", pe: {} }
    order_image: { file_name: "merge_order.exe", pe: {} }
    chained_image: { file_name: "merge_chained.exe", pe: {} }
  }

  build:
  {
    // circular merge
    link: { args: "/subsystem:console /entry:entry /out:a.exe /merge:.test=.test entry.obj test.obj", output: none, expect_exit: 28 }

    // circular merge with extra link
    link: { args: "/subsystem:console /entry:entry /out:a.exe /merge:.test=.data /merge:.data=.test entry.obj test.obj", output: none, expect_exit: 28 }

    // merge with non-defined section
    link: { args: "/subsystem:console /entry:entry /out:merge_created.exe /merge:.test=.qwe entry.obj test.obj", artifact: created_image }
    // make sure linker created .qwe and merged .test into it

    // illegal merge with .reloc
    link: { args: "/subsystem:console /entry:entry /out:a.exe /merge:.test=.reloc entry.obj test.obj", output: none, expect_exit: 26 }

    // illegal merge with .rsrc
    link: { args: "/subsystem:console /entry:entry /out:a.exe /merge:.test=.rsrc entry.obj test.obj", output: none, expect_exit: 26 }

    // merge non-defined section with defined section
    link: { args: "/subsystem:console /entry:entry /out:a.exe /merge:.qwe=.test entry.obj test.obj", output: none }

    // BSS and initialized data have incompatible flags and remain separate.
    link: { args: "/subsystem:console /entry:entry /out:merge_mixed.exe /merge:.bss=.data entry.obj mixed.obj", artifact: mixed_image }

    // merged contribution groups retain lexical order
    link: { args: "/subsystem:console /entry:entry /out:merge_order.exe /merge:.a=.z entry.obj order.obj", artifact: order_image }

    // merge .test -> .qwe -> .data
    link: { args: "/subsystem:console /entry:entry /out:merge_chained.exe /merge:.test=.qwe /merge:.qwe=.data entry.obj test.obj", artifact: chained_image }
  }

  steps:
  {
    expect_pe: { artifact: created_image, expected: { pe: { sections: { ".qwe": { @bits_set(3221225536) raw_flags: 3221225536, data: 68656c6c6f2c20776f726c64 } } } } }
    expect_pe: { artifact: mixed_image, expected: { pe: { sections: { ".data": { file_size: 512, virtual_size: 1 }, ".bss": { file_size: 0, virtual_size: 513 } }, optional: { sizeof_initialized_data: 512, sizeof_uninitialized_data: 1024 } } } }
    expect_pe: { artifact: order_image, expected: { pe: { sections: { ".z": { data: 617a } } } } }

    // make sure linker merged .test into .data
    expect_pe: { artifact: chained_image, expected: { pe: { sections: { ".data": { @bits_set(3221225536) raw_flags: 3221225536, data: 68656c6c6f2c20776f726c64 } } } } }
  }
}
