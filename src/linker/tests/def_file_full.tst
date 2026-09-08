test:
{

  artifacts:
  {
    def_full_obj:
    {
      file_name: "def_full.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            data:
            {
              name: ".rdata"
              permissions: (read, write)
              content: initialized_data
              data: { text: "test" }
            }
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              data: { hex: "c3" }
            }
          }
          symbols:
          {
            entry: { kind: external, name: "entry", section: text, value: 0 }
            foo: { kind: external, name: "foo", section: data, value: 0 }
          }
        }
      }
    }

    full_def:
    {
      file_name: "full.def"
      text: { data: { concat:
      {
        text: "; leading comment"
        hex: "0a"
        text: "NAME "
        hex: "22"
        text: "def full.exe"
        hex: "22"
        text: " BASE=0x140020000"
        hex: "0a"
        text: "VERSION 7.8"
        hex: "0a"
        text: "HEAPSIZE 0x30000, 0x4000"
        hex: "0a"
        text: "STACKSIZE 0x50000,0x6000"
        hex: "0a"
        text: "SECTIONS .rdata READ"
        hex: "0a"
        text: "EXPORTS foo @ 2 DATA"
        hex: "0a"
      } } }
    }
    bad_base_space_def:
    {
      file_name: "bad_base_space.def"
      text: { data: { concat:
      {
        text: "NAME bad_base_space BASE = 0x140020000"
        hex: "0a"
        text: "EXPORTS foo @2 DATA"
        hex: "0a"
      } } }
    }
    bad_base_colon_def:
    {
      file_name: "bad_base_colon.def"
      text: { data: { concat:
      {
        text: "NAME bad_base_colon BASE:0x140020000"
        hex: "0a"
        text: "EXPORTS foo @2 DATA"
        hex: "0a"
      } } }
    }
    bad_section_align_def:
    {
      file_name: "bad_section_align.def"
      text: { data: { concat:
      {
        text: "NAME bad_section_align"
        hex: "0a"
        text: "SECTIONS .rdata READ ALIGN=8192"
        hex: "0a"
        text: "EXPORTS foo @2 DATA"
        hex: "0a"
      } } }
    }
    exe_image: { file_name: "def full.exe", pe: {} }

    def_full_dll_obj:
    {
      file_name: "def_full_dll.obj"
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
              data: { text: "test" }
            }
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              data: { hex: "c3" }
            }
          }
          symbols:
          {
            dll_entry: { kind: external, name: "_DllMainCRTStartup", section: text, value: 0 }
            dll_foo: { kind: external, name: "dll_foo", section: data, value: 0 }
          }
        }
      }
    }
    full_dll_def:
    {
      file_name: "full_dll.def"
      text: { data: { concat:
      {
        text: "LIBRARY folded BASE=0x180020000"
        hex: "0a"
        text: "EXPORTS"
        hex: "0a"
        text: "  dll_foo DATA"
        hex: "0a"
      } } }
    }
    dll_image: { file_name: "folded.dll", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /def:full.def def_full.obj"
      artifact: exe_image
    }

    link:
    {
      args: "/subsystem:console /entry:entry /def:bad_base_space.def /out:bad_base_space.exe def_full.obj"
      expect_exit: nonzero
    }
    link:
    {
      args: "/subsystem:console /entry:entry /def:bad_base_colon.def /out:bad_base_colon.exe def_full.obj"
      expect_exit: nonzero
    }
    link:
    {
      args: "/subsystem:console /entry:entry /def:bad_section_align.def /out:bad_section_align.exe def_full.obj"
      expect_exit: nonzero
    }

    link:
    {
      args: "/dll /subsystem:console /def:full_dll.def def_full_dll.obj"
      artifact: dll_image
    }
  }

  steps:
  {
    expect_pe:
    {
      artifact: exe_image
      expected:
      {
        pe:
        {
          optional:
          {
            image_base: 5368840192
            major_image_version: 7
            minor_image_version: 8
            sizeof_heap_reserve: 196608
            sizeof_heap_commit: 16384
            sizeof_stack_reserve: 327680
            sizeof_stack_commit: 24576
          }
          sections:
          {
            ".rdata": { raw_flags: 1073741888 }
          }
          exports:
          {
            count: 1
            entries:
            {
              export_0: { name: "foo", ordinal: 2 }
            }
          }
        }
      }
    }
    expect_pe:
    {
      artifact: dll_image
      expected:
      {
        pe:
        {
          optional: { image_base: 6442582016 }
          exports:
          {
            count: 1
            entries:
            {
              export_0: { name: "dll_foo" }
            }
          }
        }
      }
    }
  }
}
