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
          text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } }
          data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "qwe" } }
          zero: { name: ".zero", permissions: (read, write), content: uninitialized_data, data: { zero: 5 } }
        }
        symbols: { my_entry: { kind: external, name: "my_entry", section: text, value: 0 } }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/entry:my_entry /subsystem:console /fixed /filealign:512 /align:4096 /out:a.exe main.obj"
      artifact: image
    }
  }

  steps:
  {
    expect_coff:
    {
      artifact: main_obj
      expected: { coff: { object:
      {
        machine: Amd64
        section_count: 3
        sections:
        {
          section_1: { name: ".text", data: c3 }
          section_2: { name: ".data", data: 717765 }
          section_3: { name: ".zero", data: "" }
        }
        symbol_count: 1
        symbols: { symbol_0: { name: "my_entry", value: 0, section_number: 1 } }
      } } }
    }
    expect_pe:
    {
      artifact: image
      expected: { pe:
      {
        is_pe32: false
        arch: x64
        subsystem: console
        section_count: 3
        section_alignment: 4096
        file_alignment: 512
        symbol_count: 0
        data_directory_count: 16
        entry_point: 4096
        // check section alignment
        @count(3) sections:
        {
          ".text": { virtual_size: 1, virtual_offset: 4096, @aligned(512) file_size, file_offset: 512, data: c3 }
          ".data": { virtual_size: 3, @aligned(4096) virtual_offset, @aligned(512) file_size, data: 717765 }
          ".zero": { virtual_size: 5, @aligned(4096) virtual_offset, @aligned(512) file_size }
        }
        optional:
        {
          sizeof_code: 512
          sizeof_initialized_data: 512
          sizeof_uninitialized_data: 512
          code_base: 4096
          image_base: 5368709120
          major_os_version: 6
          minor_os_version: 0
          major_image_version: 0
          minor_image_version: 0
          major_subsystem_version: 6
          minor_subsystem_version: 0
          win32_version: 0
          sizeof_image: 16384
          sizeof_headers: 512
          dll_characteristics: 33056
          loader_flags: 0
        }
      } }
    }
  }
}
