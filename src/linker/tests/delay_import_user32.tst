test:
{

  artifacts:
  {
    delay_import_obj:
    {
      file_name: "delay_import.obj"
      coff:
      {
        object:
        {
          machine: x64
          sections:
          {
            str:
            {
              name: ".str"
              permissions: (read, write)
              content: initialized_data
              data: { hex: "7465737400666f6f00" }
            }
            text:
            {
              name: ".text"
              permissions: (read, execute)
              content: code
              alignment: 1
              // sub  rsp,28h
              // xor  r9d,r9d
              // lea  r8,[msg]
              // lea  rdx,[caption]
              // xor  ecx,ecx
              // call qword ptr [__imp_MessageBoxA]
              // xor  eax,eax
              // add  rsp,28h
              // ret
              data: { hex: "4883ec284533c94c8d0500000000488d150000000033c9ff150000000033c04883c428c3" }
              relocations:
              {
                msg_ref: { type: Rel32, offset: 10, symbol: msg }
                caption_ref: { type: Rel32, offset: 17, symbol: caption }
                message_box_ref: { type: Rel32, offset: 25, symbol: message_box }
              }
            }
          }
          symbols:
          {
            msg: { kind: external, name: "msg", section: str, value: 0 }
            caption: { kind: external, name: "caption", section: str, value: 5 }
            entry: { kind: external, name: "entry", section: text, value: 0 }
            message_box: { kind: undefined, name: "__imp_MessageBoxA" }
          }
        }
      }
    }
  }

  build:
  {
    link:
    {
      args: "/subsystem:console /out:a.exe /entry:entry /fixed /delayload:user32.dll kernel32.lib user32.lib libcmt.lib delayimp.lib delay_import.obj /debug:full"
    }
  }

  steps: {}
}
