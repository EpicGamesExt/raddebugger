test:
{

  artifacts:
  {
    a_obj:
    {
      file_name: "a.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          return_0:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 0
            // ret
            data: { hex: "48c7c000000000c3" }
          }
          return_1:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 1
            // ret
            data: { hex: "48c7c001000000c3" }
          }
          return_2:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 2
            // ret
            data: { hex: "48c7c002000000c3" }
          }
        }
        symbols:
        {
          return_1: { kind: external_function, name: "return_1", section: return_1, value: 0 }
          return_2: { kind: external_function, name: "return_2", section: return_2, value: 0 }
        }
      } }
    }

    b_obj:
    {
      file_name: "b.obj"
      coff: { object:
      {
        machine: x64
        sections:
        {
          return_0:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 0
            // ret
            data: { hex: "48c7c000000000c3" }
          }
          return_123:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 123
            // ret
            data: { hex: "48c7c07b000000c3" }
          }
          return_321:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            alignment: 1
            // mov rax, 321
            // ret
            data: { hex: "48c7c041010000c3" }
          }
        }
        symbols:
        {
          return_123: { kind: external_function, name: "return_123", section: return_123, value: 0 }
          return_321: { kind: external_function, name: "return_321", section: return_321, value: 0 }
        }
      } }
    }

    main_obj:
    {
      file_name: "main.obj"
      coff: { object:
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
            // push  rsi
            // push  rdi
            // sub   rsp,28h
            // call  return_1
            // mov   esi,eax
            // call  return_2
            // mov   edi,eax
            // add   edi,esi
            // call  return_123
            // mov   esi,eax
            // call  return_321
            // add   eax,esi
            // add   eax,edi
            // add   rsp,28h
            // pop   rdi
            // pop   rsi
            // ret
            data: { hex: "56574883ec28e80000000089c6e80000000089c701f7e80000000089c6e80000000001f001f84883c4285f5ec3" }
            relocations:
            {
              return_1_ref: { type: Rel32, offset: 7, symbol: return_1 }
              return_2_ref: { type: Rel32, offset: 14, symbol: return_2 }
              return_123_ref: { type: Rel32, offset: 23, symbol: return_123 }
              return_321_ref: { type: Rel32, offset: 30, symbol: return_321 }
            }
          }
        }
        symbols:
        {
          entry: { kind: external, name: "entry", section: text, value: 0 }
          return_1: { kind: undefined, name: "return_1" }
          return_2: { kind: undefined, name: "return_2" }
          return_123: { kind: undefined, name: "return_123" }
          return_321: { kind: undefined, name: "return_321" }
        }
      } }
    }

    a_dll: { file_name: "a.dll", pe: {} }
    b_dll: { file_name: "b.dll", pe: {} }
    image: { file_name: "a.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/dll /implib:a.lib /export:return_1 /export:return_2 /rad_time_stamp:0x69EB0E28 a.obj libcmt.lib"
      artifact: a_dll
    }

    link:
    {
      args: "/dll /implib:b.lib /export:return_123 /export:return_321 /rad_time_stamp:0x69EB0E28 b.obj libcmt.lib"
      artifact: b_dll
    }

    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /fixed /debug:full /rad_time_stamp:0x69EB0E28 main.obj a.lib b.lib kernel32.lib delayimp.lib libcmt.lib /delayload:a.dll /delayload:b.dll"
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
          delay_imports:
          {
            count: 2
            dll_0:
            {
              name: "a.dll"
              attributes: 1
              @range(1, 18446744073709551615) module_handle
              @range(1, 18446744073709551615) import_name_table
              @range(1, 18446744073709551615) bound_table
              @range(1, 18446744073709551615) unload_table
              timestamp: 0
              bound_count: 2
              unload_count: 2
              count: 2
              entries:
              {
                import_0: { type: name, name: "return_1", hint: 0 }
                import_1: { type: name, name: "return_2", hint: 1 }
              }
            }
            dll_1:
            {
              name: "b.dll"
              attributes: 1
              @range(1, 18446744073709551615) module_handle
              @range(1, 18446744073709551615) import_name_table
              @range(1, 18446744073709551615) bound_table
              @range(1, 18446744073709551615) unload_table
              timestamp: 0
              bound_count: 2
              unload_count: 2
              count: 2
              entries:
              {
                import_0: { type: name, name: "return_123", hint: 0 }
                import_1: { type: name, name: "return_321", hint: 1 }
              }
            }
          }
        }
      }
    }
  }
}
