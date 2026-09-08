test:
{
  // write objs
  artifacts:
  {
    import_obj:
    {
      file_name: "import.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          data:
          {
            name: ".data"
            permissions: (read, write)
            content: initialized_data
            data: { zero: 1024 }
            relocations:
            {
              imp_foo_ref: { type: Addr32Nb, offset: 0, symbol: imp_foo }
              imp_bar_ref: { type: Addr32Nb, offset: 4, symbol: imp_bar }
              imp_baz_ref: { type: Addr32Nb, offset: 8, symbol: imp_baz }
              imp_baf_ref: { type: Addr32Nb, offset: 12, symbol: imp_baf }
              imp_ord_ref: { type: Addr32Nb, offset: 16, symbol: imp_ord }
              bar_ref: { type: Addr32Nb, offset: 20, symbol: bar }
              foo_ref: { type: Addr32Nb, offset: 24, symbol: foo }
              ord_ref: { type: Addr32Nb, offset: 28, symbol: ord }
            }
          }
        }
        symbols:
        {
          imp_foo: { kind: undefined, name: "__imp_foo" }
          imp_bar: { kind: undefined, name: "__imp_bar" }
          imp_baz: { kind: undefined, name: "__imp_baz" }
          imp_baf: { kind: undefined, name: "__imp_baf" }
          imp_ord: { kind: undefined, name: "__imp_ord" }
          bar: { kind: undefined, name: "bar" }
          foo: { kind: undefined, name: "foo" }
          ord: { kind: undefined, name: "ord" }
          //"baf",
          //"baz",
          //"__imp_ord2",
          //"__imp_ord4",
        }
      } }
    }

    export_obj:
    {
      file_name: "export.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          data: { name: ".data", permissions: (read, write), content: initialized_data, data: { text: "test" } }
          text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "b801000000c3" } }
        }
        symbols:
        {
          entry: { kind: external, name: "_DllMainCRTStartup", section: text, value: 0 }
          foo: { kind: external, name: "foo", section: data, value: 0 }
          ord: { kind: external, name: "ord", section: data, value: 1 }
          ord2: { kind: external, name: "ord2", section: data, value: 2 }
          ord3: { kind: external, name: "ord3", section: data, value: 9 }
          ord4: { kind: external, name: "ord4", section: data, value: 10 }
        }
        directives:
        {
          directive: "/export:foo=foo"
          directive: "/export:bar=foo"
          directive: "/export:ord,@5"
          directive: "/export:ord2,@6,DATA"
          directive: "/export:ord3,@7,NONAME,PRIVATE"
          directive: "/export:ord4,@8,NONAME,DATA"
          directive: "/export:baz=BAZ.qwe"
          directive: "/export:baf=BAZ.#1"
        }
      } }
    }

    baz_obj:
    {
      file_name: "baz.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          s1: { name: ".s1", permissions: (read, write), content: initialized_data, data: { text: "s1" } }
          s2: { name: ".s2", permissions: (read, write), content: initialized_data, data: { text: "s2" } }
          text: { name: ".text", permissions: (read, execute), content: code, data: { hex: "c3" } }
        }
        symbols:
        {
          entry: { kind: external, name: "_DllMainCRTStartup", section: text, value: 0 }
          s1: { kind: external, name: "s1", section: s1, value: 0 }
          s2: { kind: external, name: "s2", section: s2, value: 0 }
        }
        directives:
        {
          directive: "/export:baf=s1"
          directive: "/export:baz=s2"
        }
      } }
    }

    entry_obj:
    {
      file_name: "entry.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          text: { name: ".text", permissions: (read, execute), content: code, alignment: 1, data: { hex: "c3" } }
        }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
        }
      } }
    }

    export_dll: { file_name: "export.dll", pe: {} }
    baz_dll: { file_name: "baz.dll", pe: {} }

    loader_source:
    {
      file_name: "import_export_loader.c"
      text: { data: { concat:
      {
        text: "#include <windows.h>"
        hex: "0a"
        text: "int main(void) {"
        hex: "0a"
        text: "  if (!SetDllDirectoryA("
        hex: "22"
        text: "."
        hex: "22"
        text: ")) return 1;"
        hex: "0a"
        text: "  HMODULE export_dll = LoadLibraryA("
        hex: "22"
        text: "export.dll"
        hex: "22"
        text: ");"
        hex: "0a"
        text: "  if (!export_dll) return 2;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, "
        hex: "22"
        text: "bar"
        hex: "22"
        text: ")) return 3;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, "
        hex: "22"
        text: "foo"
        hex: "22"
        text: ")) return 4;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, "
        hex: "22"
        text: "ord"
        hex: "22"
        text: ")) return 5;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, "
        hex: "22"
        text: "ord2"
        hex: "22"
        text: ")) return 6;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(10))) return 7;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(12))) return 8;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(5))) return 9;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(6))) return 10;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(7))) return 11;"
        hex: "0a"
        text: "  if (!GetProcAddress(export_dll, MAKEINTRESOURCEA(8))) return 12;"
        hex: "0a"
        text: "  FreeLibrary(export_dll);"
        hex: "0a"
        text: "  return 0;"
        hex: "0a"
        text: "}"
        hex: "0a"
      } } }
    }
  }

  // link dlls
  build:
  {
    link: { args: "/dll /nodefaultlib /out:export.dll export.obj", artifact: export_dll } // export.dll
    link: { args: "/dll /out:baz.dll /export:s1,@1,NONAME /export:qwe=s2 baz.obj", artifact: baz_dll } // baz.dll
    compile_link: { tool: msvc, output: "import_export_loader.exe", args: "/nologo import_export_loader.c" }
  }

  steps:
  {
    // validate export table in export.dll
    // validate export table in export.dll
    expect_pe:
    {
      artifact: export_dll
      expected:
      {
        pe:
        {
          sections: { ".data": { virtual_offset: 12288 } }
          exports:
          {
            // validate header
            flags: 0
            timestamp: 4294967295
            major_version: 0
            minor_version: 0
            ordinal_base: 5
            count: 8
            entries:
            {
              // validate names
              export_0: { name: "baf", forwarder: "BAZ.#1", ordinal: 9 }
              export_1: { name: "bar", forwarder: "", virtual_offset: 12288, ordinal: 10 }
              export_2: { name: "baz", forwarder: "BAZ.qwe", ordinal: 11 }
              export_3: { name: "foo", forwarder: "", virtual_offset: 12288, ordinal: 12 }
              export_4: { name: "ord", forwarder: "", virtual_offset: 12289, ordinal: 5 }
              export_5: { name: "ord2", forwarder: "", virtual_offset: 12290, ordinal: 6 }
              export_6: { name: "", forwarder: "", virtual_offset: 12297, ordinal: 7 }
              export_7: { name: "", forwarder: "", virtual_offset: 12298, ordinal: 8 }
              // validate forwarders
              // validate voffs
              // validate ordinals
            }
          }
        }
      }
    }

    // validate export table in baz.dll
    expect_pe:
    {
      artifact: baz_dll
      expected:
      {
        pe:
        {
          exports:
          {
            // validate header
            flags: 0
            timestamp: 4294967295
            major_version: 0
            minor_version: 0
            ordinal_base: 1
            count: 4
            entries:
            {
              // validate names
              export_0: { name: "baf", forwarder: "", virtual_offset: 12288, ordinal: 2 }
              export_1: { name: "baz", forwarder: "", virtual_offset: 16384, ordinal: 3 }
              export_2: { name: "qwe", forwarder: "", virtual_offset: 16384, ordinal: 4 }
              export_3: { name: "", forwarder: "", virtual_offset: 12288, ordinal: 1 }
              // validate forwarders
              // validate voffs
              // validate ordinals
            }
          }
        }
      }
    }

    // test query by function name
    //T_Ok(GetProcAddress(export_dll, "baf"));
    //T_Ok(GetProcAddress(export_dll, "baz"));
    // test query by ordinal
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(9)));
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(11)));
    run: { path: "import_export_loader.exe" }

    //T_Ok(t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /delayload:export.dll /export:entry kernel32.Lib delayimp.lib libcmt.lib export.lib import.obj entry.obj") == 0);
    // TODO: check import table
  }
}
