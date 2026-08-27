test:
{

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
            data: { hex: "7465737400" }
          }
          text:
          {
            name: ".text"
            permissions: (read, execute)
            content: code
            // sub  rsp,68h                        ; alloc space on stack
            // mov  dword ptr [rsp+48h],18h        ; SECURITY_ATTRIBUTES.nLength
            // mov  qword ptr [rsp+50h],0          ; SECURITY_ATTRIBUTES.lpSecurityDescriptor
            // mov  dword ptr [rsp+58h],0          ; SECURITY_ATTRIBUTES.bInheritHandle
            // mov  qword ptr [rsp+30h],0          ; hTemplateFile
            // mov  dword ptr [rsp+28h],80h        ; dwFlagsAndAttributes
            // mov  dword ptr [rsp+20h],2          ; dwCreationDisposition
            // lea  r9,[rsp+48h]                   ; lpSecurityAttributes
            // xor  r8d,r8d                        ; dwShareMode
            // mov  edx,40000000h                  ; dwDesiredAccess
            // lea  rcx,[test]                     ; lpFileName
            // call qword ptr [__imp_CreateFileA]  ; call CreateFileA
            // mov  rcx,rax                        ; hObject
            // call qword ptr [__imp_CloseHandle]  ; call CloseHandle
            // xor  eax,eax                        ; clear result
            // add  rsp,68h                        ; dealloc stack
            // ret                                 ; return
            data: { hex: "4883ec68c74424481800000048c744245000000000c74424580000000048c744243000000000c744242880000000c7442420020000004c8d4c24484533c0ba00000040488d0d00000000ff15000000004889c1ff150000000033c04883c468c3" }
            relocations:
            {
              test_ref: { type: Rel32, offset: 70, symbol: test }
              create_file_ref: { type: Rel32, offset: 76, symbol: create_file }
              close_handle_ref: { type: Rel32, offset: 85, symbol: close_handle }
            }
          }
          dead:
          {
            name: ".text$dead"
            permissions: (read, execute)
            content: code
            alignment: 1
            flags: (link_comdat)
            data: { zero: 4 }
            relocations:
            {
              compare_string_ref: { type: Addr32Nb, offset: 0, symbol: compare_string }
            }
          }
        }
        symbols:
        {
          dead_definition: { kind: section_definition, section: dead, selection: Any }
          test: { kind: external, name: "test", section: data, value: 0 }
          entry: { kind: external, name: "entry", section: text, value: 0 }
          dead: { kind: external_function, name: "dead", section: dead, value: 0 }
          create_file: { kind: undefined, name: "__imp_CreateFileA" }
          close_handle: { kind: undefined, name: "__imp_CloseHandle" }
          compare_string: { kind: undefined, name: "__imp_CompareStringW" }
        }
      } }
    }
    image: { file_name: "a.exe", pe: {} }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /entry:entry /out:a.exe /fixed /opt:ref import.obj kernel32.lib"
      artifact: image
    }
  }

  steps:
  {
    expect_pe:
    {
      artifact: image
      // The two exact entries prove CreateFileA and CloseHandle are present and CompareStringW is absent.
      expected:
      {
        pe:
        {
          imports:
          {
            count: 1
            dll_0:
            {
              count: 2
              entries:
              {
                import_0: { type: name, name: "CloseHandle" }
                import_1: { type: name, name: "CreateFileA" }
              }
            }
          }
        }
      }
    }
    run: { path: "a.exe" }
    expect_file: { path: "test", nonempty: false }
  }
}
